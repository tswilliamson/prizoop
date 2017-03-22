
#include "platform.h"

#include "debug.h"
#include "cpu.h"
#include "memory.h"
#include "interrupts.h"
#include "keys.h"
#include "gpu.h"
#include "cb.h"
#include "display.h"
#include "main.h"

cpu_type cpu ALIGN(256);

CT_ASSERT(sizeof(cpu.memory) == 0x100);

#if TARGET_WINSIM
#include <Windows.h>
#define DEBUG_ROM 0
#endif

#if DEBUG_ROM
int iter = 0;
#define DebugInstruction(name) { char buffer[256]; sprintf(buffer, "(%d) 0x%04x : %s\n", iter++, cpu.registers.pc-1, name); OutputDebugString(buffer); }
#else
// #define DebugInstruction(...) 
#endif

/*
	References:

	Opcode disassemblies:
	http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
	http://imrannazar.com/Gameboy-Z80-Opcode-Map
	GBE source

	Which instructions modify flags:
	http://gameboy.mongenel.com/dmg/opcodes.html

	Instruction implementation:
	GBE source

	Testing:
	NO$GMB
*/

// number of cpu instructions to batch at once
#define CPU_BATCH 12

void reset(void) {
	memset(sram, 0, sizeof(sram));
	memcpy(&cpu.memory, ioReset, sizeof(cpu.memory));
	memset(vram, 0, sizeof(vram));
	memset(oam, 0, sizeof(oam));
	memset(wram, 0, sizeof(wram));
	
	cpu.registers.a = 0x01;
	cpu.registers.f = 0xb0;
	cpu.registers.b = 0x00;
	cpu.registers.c = 0x13;
	cpu.registers.d = 0x00;
	cpu.registers.e = 0xd8;
	cpu.registers.h = 0x01;
	cpu.registers.l = 0x4d;
	cpu.registers.sp = 0xfffe;
	cpu.registers.pc = 0x100;
	
	cpu.IME = 1;
	
	keys.k1.a = 1;
	keys.k1.b = 1;
	keys.k1.select = 1;
	keys.k1.start = 1;
	keys.k2.right = 1;
	keys.k2.left = 1;
	keys.k2.up = 1;
	keys.k2.down = 1;

	if (tiles == NULL) {
		tiles = (tilestype*)malloc(sizeof(tilestype));
	}

	memset(tiles, 0, sizeof(tilestype));
	
	backgroundPalette[0] = 0;
	backgroundPalette[1] = 1;
	backgroundPalette[2] = 2;
	backgroundPalette[3] = 3;
	
	spritePalette[0][0] = 0;
	spritePalette[0][1] = 3;
	spritePalette[0][2] = 1;
	spritePalette[0][3] = 0;
	
	spritePalette[1][0] = 0;
	spritePalette[1][1] = 2;
	spritePalette[1][2] = 1;
	spritePalette[1][3] = 0;

	gpu.tick = 0;
	gpu.tickBase = 0;
	
	cpu.clocks = 0;
	cpu.stopped = 0;
	cpu.halted = 0;
	
	// TODO check this.. should be same as ioReset? Or does it serve a different purpose
	writeByte(0xFF05, 0);
	writeByte(0xFF06, 0);
	writeByte(0xFF07, 0);
	writeByte(0xFF10, 0x80);
	writeByte(0xFF11, 0xBF);
	writeByte(0xFF12, 0xF3);
	writeByte(0xFF14, 0xBF);
	writeByte(0xFF16, 0x3F);
	writeByte(0xFF17, 0x00);
	writeByte(0xFF19, 0xBF);
	writeByte(0xFF1A, 0x7A);
	writeByte(0xFF1B, 0xFF);
	writeByte(0xFF1C, 0x9F);
	writeByte(0xFF1E, 0xBF);
	writeByte(0xFF20, 0xFF);
	writeByte(0xFF21, 0x00);
	writeByte(0xFF22, 0x00);
	writeByte(0xFF23, 0xBF);
	writeByte(0xFF24, 0x77);
	writeByte(0xFF25, 0xF3);
	writeByte(0xFF26, 0xF1);
	writeByte(0xFF40, 0x91);
	writeByte(0xFF42, 0x00);
	writeByte(0xFF43, 0x00);
	writeByte(0xFF45, 0x00);
	writeByte(0xFF47, 0xFC);
	writeByte(0xFF48, 0xFF);
	writeByte(0xFF49, 0xFF);
	writeByte(0xFF4A, 0x00);
	writeByte(0xFF4B, 0x00);
	writeByte(0xFFFF, 0x00);

	// initialize div timer
	cpu.div = 0xABCC;
	cpu.divBase = 0;

	// initialize timer
	cpu.timer = 0;
	cpu.timerBase = 0;

	// LCD starts out on
	gpuStep = stepLCDOn;
}

inline void undefined(void) {
	cpu.registers.pc--;
	
	unsigned char instruction = readInstrByte(cpu.registers.pc);

	#if TARGET_WINSIM
		char d[100];
		sprintf(d, "Undefined instruction 0x%02x!\n\nCheck stdout for more details.", instruction);
		MessageBox(NULL, d, "Prizoop", MB_OK);
	#else
	#ifndef PS4
		printf("Undefined instruction 0x%02x!\n", instruction);
	#endif
	#endif

#if DEBUG
	int key;
	GetKey(&key);
#endif

	exit(-1);
}

static unsigned char inc(unsigned char value) {
	if((value & 0x0f) == 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	value++;
	
	if(value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_NEGATIVE);
	
	return value;
}

static unsigned char dec(unsigned char value) {
	if(value & 0x0f) FLAGS_CLEAR(FLAGS_HALFCARRY);
	else FLAGS_SET(FLAGS_HALFCARRY);
	
	value--;
	
	if(value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_SET(FLAGS_NEGATIVE);
	
	return value;
}

static inline void add(unsigned char *destination, unsigned char value) {
	unsigned int result = *destination + value;
	
	if(result & 0xff00) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	if (((*destination & 0x0f) + (value & 0x0f)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);

	*destination = (unsigned char)(result & 0xff);
	
	if(*destination) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_NEGATIVE);
}

static inline void add2(unsigned short *destination, unsigned short value) {
	unsigned long result = *destination + value;
	
	if(result & 0xffff0000) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	if (((*destination & 0x0fff) + (value & 0x0fff)) & 0x1000) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	*destination = (unsigned short)(result & 0xffff);
	
	// zero flag left alone
	
	FLAGS_CLEAR(FLAGS_NEGATIVE);
}

static inline void adc(unsigned char value) {
	int result = cpu.registers.a + value + (FLAGS_ISCARRY ? 1 : 0);
	
	if(((value & 0x0f) + (cpu.registers.a & 0x0f) + (FLAGS_ISCARRY ? 1 : 0)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);

	if (result & 0xff00) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	FLAGS_CLEAR(FLAGS_NEGATIVE);
	
	cpu.registers.a = (unsigned char)(result & 0xff);

	if (cpu.registers.a == 0) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);
}

static inline void sbc(unsigned char value) {
	int result = cpu.registers.a - value - (FLAGS_ISCARRY ? 1 : 0);

	if ((value & 0x0f) + (FLAGS_ISCARRY ? 1 : 0) > (cpu.registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);

	if (result & 0xff00) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	FLAGS_SET(FLAGS_NEGATIVE);

	cpu.registers.a = (unsigned char)(result & 0xff);

	if (cpu.registers.a == 0) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);
}

static inline void sub(unsigned char value) {
	FLAGS_SET(FLAGS_NEGATIVE);
	
	if(value > cpu.registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if((value & 0x0f) > (cpu.registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	cpu.registers.a -= value;
	
	if(cpu.registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
}

static inline void and_a(unsigned char value) {
	cpu.registers.a &= value;
	
	if(cpu.registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE);
	FLAGS_SET(FLAGS_HALFCARRY);
}

static inline void or_a(unsigned char value) {
	cpu.registers.a |= value;
	
	if(cpu.registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

static inline void xor_a(unsigned char value) {
	cpu.registers.a ^= value;
	
	if(cpu.registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

static inline void cp(unsigned char value) {
	if (cpu.registers.a == value) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);

	if (value > cpu.registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	if ((value & 0x0f) > (cpu.registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);

	FLAGS_SET(FLAGS_NEGATIVE);
}

// 0x00
inline void nop(void) {  }

// 0x01
inline void ld_bc_nn(unsigned short operand) { cpu.registers.bc = operand; }

// 0x02
inline void ld_bcp_a(void) { writeByte(cpu.registers.bc, cpu.registers.a); }

// 0x03
inline void inc_bc(void) { cpu.registers.bc++; }

// 0x04
inline void inc_b(void) { cpu.registers.b = inc(cpu.registers.b); }

// 0x05
inline void dec_b(void) { cpu.registers.b = dec(cpu.registers.b); }

// 0x06
inline void ld_b_n(unsigned char operand) { cpu.registers.b = operand; }

// 0x07
inline void rlca(void) {
	cpu.registers.a = (cpu.registers.a << 1) | (cpu.registers.a >> 7);
	if(cpu.registers.a & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	FLAGS_CLEAR(FLAGS_ZERO | FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

// 0x08
inline void ld_nnp_sp(unsigned short operand) { writeShort(operand, cpu.registers.sp); }

// 0x09
inline void add_hl_bc(void) { add2(&cpu.registers.hl, cpu.registers.bc); }

// 0x0a
inline void ld_a_bcp(void) { cpu.registers.a = readByte(cpu.registers.bc); }

// 0x0b
inline void dec_bc(void) { cpu.registers.bc--; }

// 0x0c
inline void inc_c(void) { cpu.registers.c = inc(cpu.registers.c); }

// 0x0d
inline void dec_c(void) { cpu.registers.c = dec(cpu.registers.c); }

// 0x0e
inline void ld_c_n(unsigned char operand) { cpu.registers.c = operand; }

// 0x0f
inline void rrca(void) {
	cpu.registers.a = (cpu.registers.a >> 1) | ((cpu.registers.a << 7) & 0x80);

	if (cpu.registers.a & 0x80) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	FLAGS_CLEAR(FLAGS_ZERO | FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

// 0x10
inline void stop(unsigned char operand) { cpu.stopped = 1; }

// 0x11
inline void ld_de_nn(unsigned short operand) { cpu.registers.de = operand; }

// 0x12
inline void ld_dep_a(void) { writeByte(cpu.registers.de, cpu.registers.a); }

// 0x13
inline void inc_de(void) { cpu.registers.de++; }

// 0x14
inline void inc_d(void) { cpu.registers.d = inc(cpu.registers.d); }

// 0x15
inline void dec_d(void) { cpu.registers.d = dec(cpu.registers.d); }

// 0x16
inline void ld_d_n(unsigned char operand) { cpu.registers.d = operand; }

// 0x17
inline void rla(void) {
	int carry = FLAGS_ISSET(FLAGS_CARRY) ? 1 : 0;
	
	if(cpu.registers.a & 0x80) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	cpu.registers.a <<= 1;
	cpu.registers.a += carry;
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_ZERO | FLAGS_HALFCARRY);
}

// 0x18
inline void jr_n(unsigned char operand) {
	cpu.registers.pc += (signed char)operand;
}

// 0x19
inline void add_hl_de(void) { add2(&cpu.registers.hl, cpu.registers.de); }

// 0x1a
inline void ld_a_dep(void) { cpu.registers.a = readByte(cpu.registers.de); }

// 0x1b
inline void dec_de(void) { cpu.registers.de--; }

// 0x1c
inline void inc_e(void) { cpu.registers.e = inc(cpu.registers.e); }

// 0x1d
inline void dec_e(void) { cpu.registers.e = dec(cpu.registers.e); }

// 0x1e
inline void ld_e_n(unsigned char operand) { cpu.registers.e = operand; }

// 0x1f
inline void rra(void) {
	int carry = (FLAGS_ISSET(FLAGS_CARRY) ? 1 : 0) << 7;
	
	if(cpu.registers.a & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	cpu.registers.a >>= 1;
	cpu.registers.a += carry;
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_ZERO | FLAGS_HALFCARRY);
}

// 0x20
inline void jr_nz_n(unsigned char operand) {
	if(FLAGS_ISZERO) cpu.clocks += 8;
	else {
		cpu.registers.pc += (signed char)operand;
		cpu.clocks += 12;
	}
}

// 0x21
inline void ld_hl_nn(unsigned short operand) { cpu.registers.hl = operand; }

// 0x22
inline void ldi_hlp_a(void) { writeByte(cpu.registers.hl++, cpu.registers.a); }

// 0x23
inline void inc_hl(void) { cpu.registers.hl++; }

// 0x24
inline void inc_h(void) { cpu.registers.h = inc(cpu.registers.h); }

// 0x25
inline void dec_h(void) { cpu.registers.h = dec(cpu.registers.h); }

// 0x26
inline void ld_h_n(unsigned char operand) { cpu.registers.h = operand; }

// 0x27
inline void daa(void) {
	int s = cpu.registers.a;

	if (!FLAGS_ISNEGATIVE)
	{
		if ((FLAGS_ISHALFCARRY) || (s & 0xF) > 9)
			s += 0x06;

		if ((FLAGS_ISCARRY) || s > 0x9F)
			s += 0x60;
	}
	else
	{
		if (FLAGS_ISHALFCARRY)
			s = (s - 6) & 0xFF;

		if (FLAGS_ISCARRY)
			s -= 0x60;
	}

	if ((s & 0x100) == 0x100) {
		FLAGS_SET(FLAGS_CARRY);
	}
	else {
		// no carry clear
	}

	s = s & 0xFF;
		
	if(s != 0) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	cpu.registers.a = (unsigned char) s;

	FLAGS_CLEAR(FLAGS_HALFCARRY);
}

// 0x28
inline void jr_z_n(unsigned char operand) {
	if(FLAGS_ISZERO) {
		cpu.registers.pc += (signed char)operand;
		cpu.clocks += 12;
	}
	else cpu.clocks += 8;
}

// 0x29
inline void add_hl_hl(void) { add2(&cpu.registers.hl, cpu.registers.hl); }

// 0x2a
inline void ldi_a_hlp(void) { cpu.registers.a = readByte(cpu.registers.hl++); }

// 0x2b
inline void dec_hl(void) { cpu.registers.hl--; }

// 0x2c
inline void inc_l(void) { cpu.registers.l = inc(cpu.registers.l); }

// 0x2d
inline void dec_l(void) { cpu.registers.l = dec(cpu.registers.l); }

// 0x2e
inline void ld_l_n(unsigned char operand) { cpu.registers.l = operand; }

// 0x2f
inline void cpl(void) { cpu.registers.a = ~cpu.registers.a; FLAGS_SET(FLAGS_NEGATIVE | FLAGS_HALFCARRY); }

// 0x30
inline void jr_nc_n(char operand) {
	if(FLAGS_ISCARRY) cpu.clocks += 8;
	else {
		cpu.registers.pc += operand;
		cpu.clocks += 12;
	}
}

// 0x31
inline void ld_sp_nn(unsigned short operand) { cpu.registers.sp = operand; }

// 0x32
inline void ldd_hlp_a(void) { writeByte(cpu.registers.hl, cpu.registers.a); cpu.registers.hl--; }

// 0x33
inline void inc_sp(void) { cpu.registers.sp++; }

// 0x34
inline void inc_hlp(void) { writeByte(cpu.registers.hl, inc(readByte(cpu.registers.hl))); }

// 0x35
inline void dec_hlp(void) { writeByte(cpu.registers.hl, dec(readByte(cpu.registers.hl))); }

// 0x36
inline void ld_hlp_n(unsigned char operand) { writeByte(cpu.registers.hl, operand); }

// 0x37
inline void scf(void) { FLAGS_SET(FLAGS_CARRY); FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY); }

// 0x38
inline void jr_c_n(char operand) {
	if(FLAGS_ISCARRY) {
		cpu.registers.pc += operand;
		cpu.clocks += 12;
	}
	else cpu.clocks += 8;
}

// 0x39
inline void add_hl_sp(void) { add2(&cpu.registers.hl, cpu.registers.sp); }

// 0x3a
inline void ldd_a_hlp(void) { cpu.registers.a = readByte(cpu.registers.hl--); }

// 0x3b
inline void dec_sp(void) { cpu.registers.sp--; }

// 0x3c
inline void inc_a(void) { cpu.registers.a = inc(cpu.registers.a); }

// 0x3d
inline void dec_a(void) { cpu.registers.a = dec(cpu.registers.a); }

// 0x3e
inline void ld_a_n(unsigned char operand) { cpu.registers.a = operand; }

// 0x3f
inline void ccf(void) {
	if(FLAGS_ISCARRY) FLAGS_CLEAR(FLAGS_CARRY);
	else FLAGS_SET(FLAGS_CARRY);
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

// 0x41
inline void ld_b_c(void) { cpu.registers.b = cpu.registers.c; }

// 0x42
inline void ld_b_d(void) { cpu.registers.b = cpu.registers.d; }

// 0x43
inline void ld_b_e(void) { cpu.registers.b = cpu.registers.e; }

// 0x44
inline void ld_b_h(void) { cpu.registers.b = cpu.registers.h; }

// 0x45
inline void ld_b_l(void) { cpu.registers.b = cpu.registers.l; }

// 0x46
inline void ld_b_hlp(void) { cpu.registers.b = readByte(cpu.registers.hl); }

// 0x47
inline void ld_b_a(void) { cpu.registers.b = cpu.registers.a; }

// 0x48
inline void ld_c_b(void) { cpu.registers.c = cpu.registers.b; }

// 0x4a
inline void ld_c_d(void) { cpu.registers.c = cpu.registers.d; }

// 0x4b
inline void ld_c_e(void) { cpu.registers.c = cpu.registers.e; }

// 0x4c
inline void ld_c_h(void) { cpu.registers.c = cpu.registers.h; }

// 0x4d
inline void ld_c_l(void) { cpu.registers.c = cpu.registers.l; }

// 0x4e
inline void ld_c_hlp(void) { cpu.registers.c = readByte(cpu.registers.hl); }

// 0x4f
inline void ld_c_a(void) { cpu.registers.c = cpu.registers.a; }

// 0x50
inline void ld_d_b(void) { cpu.registers.d = cpu.registers.b; }

// 0x51
inline void ld_d_c(void) { cpu.registers.d = cpu.registers.c; }

// 0x53
inline void ld_d_e(void) { cpu.registers.d = cpu.registers.e; }

// 0x54
inline void ld_d_h(void) { cpu.registers.d = cpu.registers.h; }

// 0x55
inline void ld_d_l(void) { cpu.registers.d = cpu.registers.l; }

// 0x56
inline void ld_d_hlp(void) { cpu.registers.d = readByte(cpu.registers.hl); }

// 0x57
inline void ld_d_a(void) { cpu.registers.d = cpu.registers.a; }

// 0x58
inline void ld_e_b(void) { cpu.registers.e = cpu.registers.b; }

// 0x59
inline void ld_e_c(void) { cpu.registers.e = cpu.registers.c; }

// 0x5a
inline void ld_e_d(void) { cpu.registers.e = cpu.registers.d; }

// 0x5c
inline void ld_e_h(void) { cpu.registers.e = cpu.registers.h; }

// 0x5d
inline void ld_e_l(void) { cpu.registers.e = cpu.registers.l; }

// 0x5e
inline void ld_e_hlp(void) { cpu.registers.e = readByte(cpu.registers.hl); }

// 0x5f
inline void ld_e_a(void) { cpu.registers.e = cpu.registers.a; }

// 0x60
inline void ld_h_b(void) { cpu.registers.h = cpu.registers.b; }

// 0x61
inline void ld_h_c(void) { cpu.registers.h = cpu.registers.c; }

// 0x62
inline void ld_h_d(void) { cpu.registers.h = cpu.registers.d; }

// 0x63
inline void ld_h_e(void) { cpu.registers.h = cpu.registers.e; }

// 0x65
inline void ld_h_l(void) { cpu.registers.h = cpu.registers.l; }

// 0x66
inline void ld_h_hlp(void) { cpu.registers.h = readByte(cpu.registers.hl); }

// 0x67
inline void ld_h_a(void) { cpu.registers.h = cpu.registers.a; }

// 0x68
inline void ld_l_b(void) { cpu.registers.l = cpu.registers.b; }

// 0x69
inline void ld_l_c(void) { cpu.registers.l = cpu.registers.c; }

// 0x6a
inline void ld_l_d(void) { cpu.registers.l = cpu.registers.d; }

// 0x6b
inline void ld_l_e(void) { cpu.registers.l = cpu.registers.e; }

// 0x6c
inline void ld_l_h(void) { cpu.registers.l = cpu.registers.h; }

// 0x6e
inline void ld_l_hlp(void) { cpu.registers.l = readByte(cpu.registers.hl); }

// 0x6f
inline void ld_l_a(void) { cpu.registers.l = cpu.registers.a; }

// 0x70
inline void ld_hlp_b(void) { writeByte(cpu.registers.hl, cpu.registers.b); }

// 0x71
inline void ld_hlp_c(void) { writeByte(cpu.registers.hl, cpu.registers.c); }

// 0x72
inline void ld_hlp_d(void) { writeByte(cpu.registers.hl, cpu.registers.d); }

// 0x73
inline void ld_hlp_e(void) { writeByte(cpu.registers.hl, cpu.registers.e); }

// 0x74
inline void ld_hlp_h(void) { writeByte(cpu.registers.hl, cpu.registers.h); }

// 0x75
inline void ld_hlp_l(void) { writeByte(cpu.registers.hl, cpu.registers.l); }

// 0x76
inline void halt(void) {
	cpu.halted = 1;
}

// 0x77
inline void ld_hlp_a(void) { writeByte(cpu.registers.hl, cpu.registers.a); }

// 0x78
inline void ld_a_b(void) { cpu.registers.a = cpu.registers.b; }

// 0x79
inline void ld_a_c(void) { cpu.registers.a = cpu.registers.c; }

// 0x7a
inline void ld_a_d(void) { cpu.registers.a = cpu.registers.d; }

// 0x7b
inline void ld_a_e(void) { cpu.registers.a = cpu.registers.e; }

// 0x7c
inline void ld_a_h(void) { cpu.registers.a = cpu.registers.h; }

// 0x7d
inline void ld_a_l(void) { cpu.registers.a = cpu.registers.l; }

// 0x7e
inline void ld_a_hlp(void) { cpu.registers.a = readByte(cpu.registers.hl); }

// 0x80
inline void add_a_b(void) { add(&cpu.registers.a, cpu.registers.b); }

// 0x81
inline void add_a_c(void) { add(&cpu.registers.a, cpu.registers.c); }

// 0x82
inline void add_a_d(void) { add(&cpu.registers.a, cpu.registers.d); }

// 0x83
inline void add_a_e(void) { add(&cpu.registers.a, cpu.registers.e); }

// 0x84
inline void add_a_h(void) { add(&cpu.registers.a, cpu.registers.h); }

// 0x85
inline void add_a_l(void) { add(&cpu.registers.a, cpu.registers.l); }

// 0x86
inline void add_a_hlp(void) { add(&cpu.registers.a, readByte(cpu.registers.hl)); }

// 0x87
inline void add_a_a(void) { add(&cpu.registers.a, cpu.registers.a); }

// 0x88
inline void adc_b(void) { adc(cpu.registers.b); }

// 0x89
inline void adc_c(void) { adc(cpu.registers.c); }

// 0x8a
inline void adc_d(void) { adc(cpu.registers.d); }

// 0x8b
inline void adc_e(void) { adc(cpu.registers.e); }

// 0x8c
inline void adc_h(void) { adc(cpu.registers.h); }

// 0x8d
inline void adc_l(void) { adc(cpu.registers.l); }

// 0x8e
inline void adc_hlp(void) { adc(readByte(cpu.registers.hl)); }

// 0x8f
inline void adc_a(void) { adc(cpu.registers.a); }

// 0x90
inline void sub_b(void) { sub(cpu.registers.b); }

// 0x91
inline void sub_c(void) { sub(cpu.registers.c); }

// 0x92
inline void sub_d(void) { sub(cpu.registers.d); }

// 0x93
inline void sub_e(void) { sub(cpu.registers.e); }

// 0x94
inline void sub_h(void) { sub(cpu.registers.h); }

// 0x95
inline void sub_l(void) { sub(cpu.registers.l); }

// 0x96
inline void sub_hlp(void) { sub(readByte(cpu.registers.hl)); }

// 0x97
inline void sub_a(void) { sub(cpu.registers.a); }

// 0x98
inline void sbc_b(void) { sbc(cpu.registers.b); }

// 0x99
inline void sbc_c(void) { sbc(cpu.registers.c); }

// 0x9a
inline void sbc_d(void) { sbc(cpu.registers.d); }

// 0x9b
inline void sbc_e(void) { sbc(cpu.registers.e); }

// 0x9c
inline void sbc_h(void) { sbc(cpu.registers.h); }

// 0x9d
inline void sbc_l(void) { sbc(cpu.registers.l); }

// 0x9e
inline void sbc_hlp(void) { sbc(readByte(cpu.registers.hl)); }

// 0x9f
inline void sbc_a(void) { sbc(cpu.registers.a); }

// 0xa0
inline void and_b(void) { and_a(cpu.registers.b); }

// 0xa1
inline void and_c(void) { and_a(cpu.registers.c); }

// 0xa2
inline void and_d(void) { and_a(cpu.registers.d); }

// 0xa3
inline void and_e(void) { and_a(cpu.registers.e); }

// 0xa4
inline void and_h(void) { and_a(cpu.registers.h); }

// 0xa5
inline void and_l(void) { and_a(cpu.registers.l); }

// 0xa6
inline void and_hlp(void) { and_a(readByte(cpu.registers.hl)); }

// 0xa7
inline void and_a(void) { and_a(cpu.registers.a); }

// 0xa8
inline void xor_b(void) { xor_a(cpu.registers.b); }

// 0xa9
inline void xor_c(void) { xor_a(cpu.registers.c); }

// 0xaa
inline void xor_d(void) { xor_a(cpu.registers.d); }

// 0xab
inline void xor_e(void) { xor_a(cpu.registers.e); }

// 0xac
inline void xor_h(void) { xor_a(cpu.registers.h); }

// 0xad
inline void xor_l(void) { xor_a(cpu.registers.l); }

// 0xae
inline void xor_hlp(void) { xor_a(readByte(cpu.registers.hl)); }

// 0xaf
inline void xor_a(void) { xor_a(cpu.registers.a); }

// 0xb0
inline void or_b(void) { or_a(cpu.registers.b); }

// 0xb1
inline void or_c(void) { or_a(cpu.registers.c); }

// 0xb2
inline void or_d(void) { or_a(cpu.registers.d); }

// 0xb3
inline void or_e(void) { or_a(cpu.registers.e); }

// 0xb4
inline void or_h(void) { or_a(cpu.registers.h); }

// 0xb5
inline void or_l(void) { or_a(cpu.registers.l); }

// 0xb6
inline void or_hlp(void) { or_a(readByte(cpu.registers.hl)); }

// 0xb7
inline void or_a(void) { or_a(cpu.registers.a); }

// 0xb8
inline void cp_b(void) { cp(cpu.registers.b); }

// 0xb9
inline void cp_c(void) { cp(cpu.registers.c); }

// 0xba
inline void cp_d(void) { cp(cpu.registers.d); }

// 0xbb
inline void cp_e(void) { cp(cpu.registers.e); }

// 0xbc
inline void cp_h(void) { cp(cpu.registers.h); }

// 0xbd
inline void cp_l(void) { cp(cpu.registers.l); }

// 0xbe
inline void cp_hlp(void) { cp(readByte(cpu.registers.hl)); }

// 0xbf
inline void cp_a(void) { cp(cpu.registers.a); }

// 0xc0
inline void ret_nz(void) {
	if(FLAGS_ISZERO) cpu.clocks += 8;
	else {
		cpu.registers.pc = readShortFromStack();
		cpu.clocks += 20;
	}
}

// 0xc1
inline void pop_bc(void) { cpu.registers.bc = readShortFromStack(); }

// 0xc2
inline void jp_nz_nn(unsigned short operand) {
	if(FLAGS_ISZERO) cpu.clocks += 12;
	else {
		cpu.registers.pc = operand;
		cpu.clocks += 16;
	}
}

// 0xc3
inline void jp_nn(unsigned short operand) {
	cpu.registers.pc = operand;
}

// 0xc4
inline void call_nz_nn(unsigned short operand) {
	if(FLAGS_ISZERO) cpu.clocks += 12;
	else {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 24;
	}
}

// 0xc5
inline void push_bc(void) { writeShortToStack(cpu.registers.bc); }

// 0xc6
inline void add_a_n(unsigned char operand) { add(&cpu.registers.a, operand); }

// 0xc7
inline void rst_0(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0000; }

// 0xc8
inline void ret_z(void) {
	if(FLAGS_ISZERO) {
		cpu.registers.pc = readShortFromStack();
		cpu.clocks += 20;
	}
	else cpu.clocks += 8;
}

// 0xc9
inline void ret(void) { cpu.registers.pc = readShortFromStack(); }

// 0xca
inline void jp_z_nn(unsigned short operand) {
	if(FLAGS_ISZERO) {
		cpu.registers.pc = operand;
		cpu.clocks += 16;
	}
	else cpu.clocks += 12;
}

// 0xcb
// cb.c

// 0xcc
inline void call_z_nn(unsigned short operand) {
	if(FLAGS_ISZERO) {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 24;
	}
	else cpu.clocks += 12;
}

// 0xcd
inline void call_nn(unsigned short operand) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = operand; }

// 0xce
inline void adc_n(unsigned char operand) { adc(operand); }

// 0xcf
inline void rst_08(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0008; }

// 0xd0
inline void ret_nc(void) {
	if(FLAGS_ISCARRY) cpu.clocks += 8;
	else {
		cpu.registers.pc = readShortFromStack();
		cpu.clocks += 20;
	}
}

// 0xd1
inline void pop_de(void) { cpu.registers.de = readShortFromStack(); }

// 0xd2
inline void jp_nc_nn(unsigned short operand) {
	if(!FLAGS_ISCARRY) {
		cpu.registers.pc = operand;
		cpu.clocks += 16;
	}
	else cpu.clocks += 12;
}

// 0xd4
inline void call_nc_nn(unsigned short operand) {
	if(!FLAGS_ISCARRY) {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 24;
	}
	else cpu.clocks += 12;
}

// 0xd5
inline void push_de(void) { writeShortToStack(cpu.registers.de); }

// 0xd6
inline void sub_n(unsigned char operand) { sub(operand); }

// 0xd7
inline void rst_10(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0010; }

// 0xd8
inline void ret_c(void) {
	if(FLAGS_ISCARRY) {
		cpu.registers.pc = readShortFromStack();
		cpu.clocks += 20;
	}
	else cpu.clocks += 8;
}

// 0xd9
inline void ret_i(void) {
	cpu.IME = 1;
	cpu.registers.pc = readShortFromStack();
}

// 0xda
inline void jp_c_nn(unsigned short operand) {
	if(FLAGS_ISCARRY) {
		cpu.registers.pc = operand;
		cpu.clocks += 16;
	}
	else cpu.clocks += 12;
}

// 0xdc
inline void call_c_nn(unsigned short operand) {
	if(FLAGS_ISCARRY) {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 24;
	}
	else cpu.clocks += 12;
}

// 0xde
inline void sbc_n(unsigned char operand) { sbc(operand); }

// 0xdf
inline void rst_18(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0018; }

// 0xe0
inline void ld_ff_n_ap(unsigned char operand) { writeByte(0xff00 + operand, cpu.registers.a); }

// 0xe1
inline void pop_hl(void) { cpu.registers.hl = readShortFromStack(); }

// 0xe2
inline void ld_ff_c_a(void) { writeByte(0xff00 + cpu.registers.c, cpu.registers.a); }

// 0xe5
inline void push_hl(void) { writeShortToStack(cpu.registers.hl); }

// 0xe6
inline void and_n(unsigned char operand) {
	cpu.registers.a &= operand;
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE);
	FLAGS_SET(FLAGS_HALFCARRY);
	if(cpu.registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
}

// 0xe7
inline void rst_20(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0020; }

// 0xe8
inline void add_sp_n(unsigned char operand) {
	unsigned int result = cpu.registers.sp + operand;

	if ((result & 0xff00) != (cpu.registers.sp & 0xff00))
		FLAGS_SET(FLAGS_CARRY);
	else
		FLAGS_CLEAR(FLAGS_CARRY);

	if ((cpu.registers.sp & 0x000f) + (operand & 0x0f) > 0x0f) 
		FLAGS_SET(FLAGS_HALFCARRY);
	else
		FLAGS_CLEAR(FLAGS_HALFCARRY);

	// signed result
	if (operand & 0x80) {
		result -= 0x0100;
	}

	cpu.registers.sp = result & 0xffff;

	// _does_ clear the zero flag
	FLAGS_CLEAR(FLAGS_ZERO | FLAGS_NEGATIVE);
}

// 0xe9
inline void jp_hl(void) {
	cpu.registers.pc = cpu.registers.hl;
}

// 0xea
inline void ld_nnp_a(unsigned short operand) { writeByte(operand, cpu.registers.a); }

// 0xee
inline void xor_n(unsigned char operand) { xor_a(operand); }

//0xef
inline void rst_28(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0028; }

// 0xf0
inline void ld_ff_ap_n(unsigned char operand) { cpu.registers.a = readByte(0xff00 + operand); }

// 0xf1
inline void pop_af(void) { cpu.registers.af = readShortFromStack(); cpu.registers.f &= 0xF0; }

// 0xf2
inline void ld_a_ff_c(void) { cpu.registers.a = readByte(0xff00 + cpu.registers.c); }

// 0xf3
inline void di_inst(void) { cpu.IME = 0; }

// 0xf5
inline void push_af(void) { writeShortToStack(cpu.registers.af); }

// 0xf6
inline void or_n(unsigned char operand) { or_a(operand); }

// 0xf7
inline void rst_30(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0030; }

// 0xf8
inline void ld_hl_sp_n(unsigned char operand) {
	int result = cpu.registers.sp + operand;

	if ((result & 0xff00) != (cpu.registers.sp & 0xff00))
		FLAGS_SET(FLAGS_CARRY);
	else
		FLAGS_CLEAR(FLAGS_CARRY);

	if ((cpu.registers.sp & 0x000f) + (operand & 0x0f) > 0x0f)
		FLAGS_SET(FLAGS_HALFCARRY);
	else
		FLAGS_CLEAR(FLAGS_HALFCARRY);

	// signed result
	if (operand & 0x80) {
		result -= 0x0100;
	}

	FLAGS_CLEAR(FLAGS_ZERO | FLAGS_NEGATIVE);
	
	cpu.registers.hl = (unsigned short)(result & 0xffff);
}

// 0xf9
inline void ld_sp_hl(void) { cpu.registers.sp = cpu.registers.hl; }

// 0xfa
inline void ld_a_nnp(unsigned short operand) { cpu.registers.a = readByte(operand); }

// 0xfb
inline void ei(void) { cpu.IME = 1; }

// 0xfe
inline void cp_n(unsigned char operand) {
	FLAGS_SET(FLAGS_NEGATIVE);
	
	if(cpu.registers.a == operand) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);
	
	if(operand > cpu.registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if((operand & 0x0f) > (cpu.registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
}

//0xff
inline void rst_38(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0038; }

void cpuStep() {
	if (cpu.stopped || cpu.halted) {
		cpu.clocks += 12;
		return;
	}

	{
		TIME_SCOPE();

		for (int i = 0; i < CPU_BATCH; i++) {
			DebugPC(cpu.registers.pc);

			// perform inlined instruction op
			switch (readInstrByte(cpu.registers.pc++)) {
				#define INSTRUCTION_0(name,numticks,func,id,code)   case id: DebugInstruction(name); func(); cpu.clocks += numticks; code break;
				#define INSTRUCTION_1(name,numticks,func,id,code)   case id: DebugInstruction(name, readInstrByte(cpu.registers.pc)); { unsigned char operand = readInstrByte(cpu.registers.pc++); func(operand); cpu.clocks += numticks; code } break;
				#define INSTRUCTION_1S(name,numticks,func,id,code)  case id: DebugInstruction(name, readInstrByte(cpu.registers.pc)); { signed char operand = readInstrByte(cpu.registers.pc++); func(operand); cpu.clocks += numticks; code } break;
				#define INSTRUCTION_2(name,numticks,func,id,code)   case id: DebugInstruction(name, readInstrShort(cpu.registers.pc)); { unsigned short operand = readInstrShort(cpu.registers.pc++); ++cpu.registers.pc; func(operand); cpu.clocks += numticks; code } break;
				#include "cpu_instructions.inl"
				#undef INSTRUCTION_0
				#undef INSTRUCTION_1
				#undef INSTRUCTION_1S
				#undef INSTRUCTION_2
			}
		}
	}
}