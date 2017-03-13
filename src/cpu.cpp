
#include "platform.h"

#include "debug.h"
#include "registers.h"
#include "memory.h"
#include "interrupts.h"
#include "keys.h"
#include "gpu.h"
#include "cb.h"
#include "display.h"
#include "main.h"

#include "cpu.h"

#if TARGET_WINSIM
#include <Windows.h>
#define DEBUG_ROM 0
#endif

#if DEBUG_ROM
int iter = 0;
#define DebugInstruction(name) { char buffer[256]; sprintf(buffer, "(%d) 0x%04x : %s\n", iter++, registers.pc-1, name); OutputDebugString(buffer); }
#else
#define DebugInstruction(...) 
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
#define CPU_BATCH 24

cpuInfo cpu;

void reset(void) {
	memset(sram, 0, sizeof(sram));
	memcpy(hram_io, ioReset, sizeof(hram_io));
	memset(vram, 0, sizeof(vram));
	memset(oam, 0, sizeof(oam));
	memset(wram, 0, sizeof(wram));
	
	registers.a = 0x01;
	registers.f = 0xb0;
	registers.b = 0x00;
	registers.c = 0x13;
	registers.d = 0x00;
	registers.e = 0xd8;
	registers.h = 0x01;
	registers.l = 0x4d;
	registers.sp = 0xfffe;
	registers.pc = 0x100;
	
	interrupt.master = 1;
	interrupt.enable = 0;
	interrupt.flags = 0;
	
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
	
	gpu.control = 0;
	gpu.scrollX = 0;
	gpu.scrollY = 0;
	gpu.scanline = 0;
	gpu.tick = 0;
	
	cpu.ticks = 0;
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
}

inline void undefined(void) {
	registers.pc--;
	
	unsigned char instruction = readByte(registers.pc);

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
	printRegisters();

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
	
	*destination = (unsigned char)(result & 0xff);
	
	if(*destination) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	if(((*destination & 0x0f) + (value & 0x0f)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	FLAGS_CLEAR(FLAGS_NEGATIVE);
}

static inline void add2(unsigned short *destination, unsigned short value) {
	unsigned long result = *destination + value;
	
	if(result & 0xffff0000) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	*destination = (unsigned short)(result & 0xffff);
	
	if(((*destination & 0x0f) + (value & 0x0f)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	// zero flag left alone
	
	FLAGS_CLEAR(FLAGS_NEGATIVE);
}

static inline void adc(unsigned char value) {
	value += FLAGS_ISCARRY ? 1 : 0;
	
	int result = registers.a + value;
	
	if(result & 0xff00) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if(value == registers.a) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);
	
	if(((value & 0x0f) + (registers.a & 0x0f)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	FLAGS_SET(FLAGS_NEGATIVE);
	
	registers.a = (unsigned char)(result & 0xff);
}

static inline void sbc(unsigned char value) {
	value += FLAGS_ISCARRY ? 1 : 0;
	
	FLAGS_SET(FLAGS_NEGATIVE);
	
	if(value > registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if(value == registers.a) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);
	
	if((value & 0x0f) > (registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	registers.a -= value;
}

static inline void sub(unsigned char value) {
	FLAGS_SET(FLAGS_NEGATIVE);
	
	if(value > registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if((value & 0x0f) > (registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	registers.a -= value;
	
	if(registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
}

static inline void and_a(unsigned char value) {
	registers.a &= value;
	
	if(registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE);
	FLAGS_SET(FLAGS_HALFCARRY);
}

static inline void or_a(unsigned char value) {
	registers.a |= value;
	
	if(registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

static inline void xor_a(unsigned char value) {
	registers.a ^= value;
	
	if(registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

static inline void cp(unsigned char value) {
	if (registers.a == value) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);

	if (value > registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	if ((value & 0x0f) > (registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);

	FLAGS_SET(FLAGS_NEGATIVE);
}

// 0x00
inline void nop(void) {  }

// 0x01
inline void ld_bc_nn(unsigned short operand) { registers.bc = operand; }

// 0x02
inline void ld_bcp_a(void) { writeByte(registers.bc, registers.a); }

// 0x03
inline void inc_bc(void) { registers.bc++; }

// 0x04
inline void inc_b(void) { registers.b = inc(registers.b); }

// 0x05
inline void dec_b(void) { registers.b = dec(registers.b); }

// 0x06
inline void ld_b_n(unsigned char operand) { registers.b = operand; }

// 0x07
inline void rlca(void) {
	unsigned char carry = (registers.a & 0x80) >> 7;
	if(carry) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	registers.a <<= 1;
	registers.a += carry;
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_ZERO | FLAGS_HALFCARRY);
}

// 0x08
inline void ld_nnp_sp(unsigned short operand) { writeShort(operand, registers.sp); }

// 0x09
inline void add_hl_bc(void) { add2(&registers.hl, registers.bc); }

// 0x0a
inline void ld_a_bcp(void) { registers.a = readByte(registers.bc); }

// 0x0b
inline void dec_bc(void) { registers.bc--; }

// 0x0c
inline void inc_c(void) { registers.c = inc(registers.c); }

// 0x0d
inline void dec_c(void) { registers.c = dec(registers.c); }

// 0x0e
inline void ld_c_n(unsigned char operand) { registers.c = operand; }

// 0x0f
inline void rrca(void) {
	unsigned char carry = registers.a & 0x01;
	if(carry) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	registers.a >>= 1;
	if(carry) registers.a |= 0x80;
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_ZERO | FLAGS_HALFCARRY);
}

// 0x10
inline void stop(unsigned char operand) { cpu.stopped = 1; }

// 0x11
inline void ld_de_nn(unsigned short operand) { registers.de = operand; }

// 0x12
inline void ld_dep_a(void) { writeByte(registers.de, registers.a); }

// 0x13
inline void inc_de(void) { registers.de++; }

// 0x14
inline void inc_d(void) { registers.d = inc(registers.d); }

// 0x15
inline void dec_d(void) { registers.d = dec(registers.d); }

// 0x16
inline void ld_d_n(unsigned char operand) { registers.d = operand; }

// 0x17
inline void rla(void) {
	int carry = FLAGS_ISSET(FLAGS_CARRY) ? 1 : 0;
	
	if(registers.a & 0x80) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	registers.a <<= 1;
	registers.a += carry;
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_ZERO | FLAGS_HALFCARRY);
}

// 0x18
inline void jr_n(unsigned char operand) {
	registers.pc += (signed char)operand;

#if DEBUG
	debugJump();
#endif
}

// 0x19
inline void add_hl_de(void) { add2(&registers.hl, registers.de); }

// 0x1a
inline void ld_a_dep(void) { registers.a = readByte(registers.de); }

// 0x1b
inline void dec_de(void) { registers.de--; }

// 0x1c
inline void inc_e(void) { registers.e = inc(registers.e); }

// 0x1d
inline void dec_e(void) { registers.e = dec(registers.e); }

// 0x1e
inline void ld_e_n(unsigned char operand) { registers.e = operand; }

// 0x1f
inline void rra(void) {
	int carry = (FLAGS_ISSET(FLAGS_CARRY) ? 1 : 0) << 7;
	
	if(registers.a & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	registers.a >>= 1;
	registers.a += carry;
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_ZERO | FLAGS_HALFCARRY);
}

// 0x20
inline void jr_nz_n(unsigned char operand) {
	if(FLAGS_ISZERO) cpu.ticks += 8;
	else {
		registers.pc += (signed char)operand;

#if DEBUG
		debugJump();
#endif
		cpu.ticks += 12;
	}
}

// 0x21
inline void ld_hl_nn(unsigned short operand) { registers.hl = operand; }

// 0x22
inline void ldi_hlp_a(void) { writeByte(registers.hl++, registers.a); }

// 0x23
inline void inc_hl(void) { registers.hl++; }

// 0x24
inline void inc_h(void) { registers.h = inc(registers.h); }

// 0x25
inline void dec_h(void) { registers.h = dec(registers.h); }

// 0x26
inline void ld_h_n(unsigned char operand) { registers.h = operand; }

// 0x27
inline void daa(void) {
	{
		unsigned short s = registers.a;
		
		if(FLAGS_ISNEGATIVE) {
			if(FLAGS_ISHALFCARRY) s = (s - 0x06)&0xFF;
			if(FLAGS_ISCARRY) s -= 0x60;
		}
		else {
			if(FLAGS_ISHALFCARRY || (s & 0xF) > 9) s += 0x06;
			if(FLAGS_ISCARRY || s > 0x9F) s += 0x60;
		}
		
		registers.a = s;
		FLAGS_CLEAR(FLAGS_HALFCARRY);
		
		if(registers.a) FLAGS_CLEAR(FLAGS_ZERO);
		else FLAGS_SET(FLAGS_ZERO);
		
		if(s >= 0x100) FLAGS_SET(FLAGS_CARRY);
	}
}

// 0x28
inline void jr_z_n(unsigned char operand) {
	if(FLAGS_ISZERO) {
		registers.pc += (signed char)operand;
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 12;
	}
	else cpu.ticks += 8;
}

// 0x29
inline void add_hl_hl(void) { add2(&registers.hl, registers.hl); }

// 0x2a
inline void ldi_a_hlp(void) { registers.a = readByte(registers.hl++); }

// 0x2b
inline void dec_hl(void) { registers.hl--; }

// 0x2c
inline void inc_l(void) { registers.l = inc(registers.l); }

// 0x2d
inline void dec_l(void) { registers.l = dec(registers.l); }

// 0x2e
inline void ld_l_n(unsigned char operand) { registers.l = operand; }

// 0x2f
inline void cpl(void) { registers.a = ~registers.a; FLAGS_SET(FLAGS_NEGATIVE | FLAGS_HALFCARRY); }

// 0x30
inline void jr_nc_n(char operand) {
	if(FLAGS_ISCARRY) cpu.ticks += 8;
	else {
		registers.pc += operand;
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 12;
	}
}

// 0x31
inline void ld_sp_nn(unsigned short operand) { registers.sp = operand; }

// 0x32
inline void ldd_hlp_a(void) { writeByte(registers.hl, registers.a); registers.hl--; }

// 0x33
inline void inc_sp(void) { registers.sp++; }

// 0x34
inline void inc_hlp(void) { writeByte(registers.hl, inc(readByte(registers.hl))); }

// 0x35
inline void dec_hlp(void) { writeByte(registers.hl, dec(readByte(registers.hl))); }

// 0x36
inline void ld_hlp_n(unsigned char operand) { writeByte(registers.hl, operand); }

// 0x37
inline void scf(void) { FLAGS_SET(FLAGS_CARRY); FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY); }

// 0x38
inline void jr_c_n(char operand) {
	if(FLAGS_ISCARRY) {
		registers.pc += operand;
		cpu.ticks += 12;
	}
	else cpu.ticks += 8;
}

// 0x39
inline void add_hl_sp(void) { add2(&registers.hl, registers.sp); }

// 0x3a
inline void ldd_a_hlp(void) { registers.a = readByte(registers.hl--); }

// 0x3b
inline void dec_sp(void) { registers.sp--; }

// 0x3c
inline void inc_a(void) { registers.a = inc(registers.a); }

// 0x3d
inline void dec_a(void) { registers.a = dec(registers.a); }

// 0x3e
inline void ld_a_n(unsigned char operand) { registers.a = operand; }

// 0x3f
inline void ccf(void) {
	if(FLAGS_ISCARRY) FLAGS_CLEAR(FLAGS_CARRY);
	else FLAGS_SET(FLAGS_CARRY);
	
	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

// 0x41
inline void ld_b_c(void) { registers.b = registers.c; }

// 0x42
inline void ld_b_d(void) { registers.b = registers.d; }

// 0x43
inline void ld_b_e(void) { registers.b = registers.e; }

// 0x44
inline void ld_b_h(void) { registers.b = registers.h; }

// 0x45
inline void ld_b_l(void) { registers.b = registers.l; }

// 0x46
inline void ld_b_hlp(void) { registers.b = readByte(registers.hl); }

// 0x47
inline void ld_b_a(void) { registers.b = registers.a; }

// 0x48
inline void ld_c_b(void) { registers.c = registers.b; }

// 0x4a
inline void ld_c_d(void) { registers.c = registers.d; }

// 0x4b
inline void ld_c_e(void) { registers.c = registers.e; }

// 0x4c
inline void ld_c_h(void) { registers.c = registers.h; }

// 0x4d
inline void ld_c_l(void) { registers.c = registers.l; }

// 0x4e
inline void ld_c_hlp(void) { registers.c = readByte(registers.hl); }

// 0x4f
inline void ld_c_a(void) { registers.c = registers.a; }

// 0x50
inline void ld_d_b(void) { registers.d = registers.b; }

// 0x51
inline void ld_d_c(void) { registers.d = registers.c; }

// 0x53
inline void ld_d_e(void) { registers.d = registers.e; }

// 0x54
inline void ld_d_h(void) { registers.d = registers.h; }

// 0x55
inline void ld_d_l(void) { registers.d = registers.l; }

// 0x56
inline void ld_d_hlp(void) { registers.d = readByte(registers.hl); }

// 0x57
inline void ld_d_a(void) { registers.d = registers.a; }

// 0x58
inline void ld_e_b(void) { registers.e = registers.b; }

// 0x59
inline void ld_e_c(void) { registers.e = registers.c; }

// 0x5a
inline void ld_e_d(void) { registers.e = registers.d; }

// 0x5c
inline void ld_e_h(void) { registers.e = registers.h; }

// 0x5d
inline void ld_e_l(void) { registers.e = registers.l; }

// 0x5e
inline void ld_e_hlp(void) { registers.e = readByte(registers.hl); }

// 0x5f
inline void ld_e_a(void) { registers.e = registers.a; }

// 0x60
inline void ld_h_b(void) { registers.h = registers.b; }

// 0x61
inline void ld_h_c(void) { registers.h = registers.c; }

// 0x62
inline void ld_h_d(void) { registers.h = registers.d; }

// 0x63
inline void ld_h_e(void) { registers.h = registers.e; }

// 0x65
inline void ld_h_l(void) { registers.h = registers.l; }

// 0x66
inline void ld_h_hlp(void) { registers.h = readByte(registers.hl); }

// 0x67
inline void ld_h_a(void) { registers.h = registers.a; }

// 0x68
inline void ld_l_b(void) { registers.l = registers.b; }

// 0x69
inline void ld_l_c(void) { registers.l = registers.c; }

// 0x6a
inline void ld_l_d(void) { registers.l = registers.d; }

// 0x6b
inline void ld_l_e(void) { registers.l = registers.e; }

// 0x6c
inline void ld_l_h(void) { registers.l = registers.h; }

// 0x6e
inline void ld_l_hlp(void) { registers.l = readByte(registers.hl); }

// 0x6f
inline void ld_l_a(void) { registers.l = registers.a; }

// 0x70
inline void ld_hlp_b(void) { writeByte(registers.hl, registers.b); }

// 0x71
inline void ld_hlp_c(void) { writeByte(registers.hl, registers.c); }

// 0x72
inline void ld_hlp_d(void) { writeByte(registers.hl, registers.d); }

// 0x73
inline void ld_hlp_e(void) { writeByte(registers.hl, registers.e); }

// 0x74
inline void ld_hlp_h(void) { writeByte(registers.hl, registers.h); }

// 0x75
inline void ld_hlp_l(void) { writeByte(registers.hl, registers.l); }

// 0x76
inline void halt(void) {
	if(interrupt.master) {
		cpu.halted = 1;
	}
	else registers.pc++;
}

// 0x77
inline void ld_hlp_a(void) { writeByte(registers.hl, registers.a); }

// 0x78
inline void ld_a_b(void) { registers.a = registers.b; }

// 0x79
inline void ld_a_c(void) { registers.a = registers.c; }

// 0x7a
inline void ld_a_d(void) { registers.a = registers.d; }

// 0x7b
inline void ld_a_e(void) { registers.a = registers.e; }

// 0x7c
inline void ld_a_h(void) { registers.a = registers.h; }

// 0x7d
inline void ld_a_l(void) { registers.a = registers.l; }

// 0x7e
inline void ld_a_hlp(void) { registers.a = readByte(registers.hl); }

// 0x80
inline void add_a_b(void) { add(&registers.a, registers.b); }

// 0x81
inline void add_a_c(void) { add(&registers.a, registers.c); }

// 0x82
inline void add_a_d(void) { add(&registers.a, registers.d); }

// 0x83
inline void add_a_e(void) { add(&registers.a, registers.e); }

// 0x84
inline void add_a_h(void) { add(&registers.a, registers.h); }

// 0x85
inline void add_a_l(void) { add(&registers.a, registers.l); }

// 0x86
inline void add_a_hlp(void) { add(&registers.a, readByte(registers.hl)); }

// 0x87
inline void add_a_a(void) { add(&registers.a, registers.a); }

// 0x88
inline void adc_b(void) { adc(registers.b); }

// 0x89
inline void adc_c(void) { adc(registers.c); }

// 0x8a
inline void adc_d(void) { adc(registers.d); }

// 0x8b
inline void adc_e(void) { adc(registers.e); }

// 0x8c
inline void adc_h(void) { adc(registers.h); }

// 0x8d
inline void adc_l(void) { adc(registers.l); }

// 0x8e
inline void adc_hlp(void) { adc(readByte(registers.hl)); }

// 0x8f
inline void adc_a(void) { adc(registers.a); }

// 0x90
inline void sub_b(void) { sub(registers.b); }

// 0x91
inline void sub_c(void) { sub(registers.c); }

// 0x92
inline void sub_d(void) { sub(registers.d); }

// 0x93
inline void sub_e(void) { sub(registers.e); }

// 0x94
inline void sub_h(void) { sub(registers.h); }

// 0x95
inline void sub_l(void) { sub(registers.l); }

// 0x96
inline void sub_hlp(void) { sub(readByte(registers.hl)); }

// 0x97
inline void sub_a(void) { sub(registers.a); }

// 0x98
inline void sbc_b(void) { sbc(registers.b); }

// 0x99
inline void sbc_c(void) { sbc(registers.c); }

// 0x9a
inline void sbc_d(void) { sbc(registers.d); }

// 0x9b
inline void sbc_e(void) { sbc(registers.e); }

// 0x9c
inline void sbc_h(void) { sbc(registers.h); }

// 0x9d
inline void sbc_l(void) { sbc(registers.l); }

// 0x9e
inline void sbc_hlp(void) { sbc(readByte(registers.hl)); }

// 0x9f
inline void sbc_a(void) { sbc(registers.a); }

// 0xa0
inline void and_b(void) { and_a(registers.b); }

// 0xa1
inline void and_c(void) { and_a(registers.c); }

// 0xa2
inline void and_d(void) { and_a(registers.d); }

// 0xa3
inline void and_e(void) { and_a(registers.e); }

// 0xa4
inline void and_h(void) { and_a(registers.h); }

// 0xa5
inline void and_l(void) { and_a(registers.l); }

// 0xa6
inline void and_hlp(void) { and_a(readByte(registers.hl)); }

// 0xa7
inline void and_a(void) { and_a(registers.a); }

// 0xa8
inline void xor_b(void) { xor_a(registers.b); }

// 0xa9
inline void xor_c(void) { xor_a(registers.c); }

// 0xaa
inline void xor_d(void) { xor_a(registers.d); }

// 0xab
inline void xor_e(void) { xor_a(registers.e); }

// 0xac
inline void xor_h(void) { xor_a(registers.h); }

// 0xad
inline void xor_l(void) { xor_a(registers.l); }

// 0xae
inline void xor_hlp(void) { xor_a(readByte(registers.hl)); }

// 0xaf
inline void xor_a(void) { xor_a(registers.a); }

// 0xb0
inline void or_b(void) { or_a(registers.b); }

// 0xb1
inline void or_c(void) { or_a(registers.c); }

// 0xb2
inline void or_d(void) { or_a(registers.d); }

// 0xb3
inline void or_e(void) { or_a(registers.e); }

// 0xb4
inline void or_h(void) { or_a(registers.h); }

// 0xb5
inline void or_l(void) { or_a(registers.l); }

// 0xb6
inline void or_hlp(void) { or_a(readByte(registers.hl)); }

// 0xb7
inline void or_a(void) { or_a(registers.a); }

// 0xb8
inline void cp_b(void) { cp(registers.b); }

// 0xb9
inline void cp_c(void) { cp(registers.c); }

// 0xba
inline void cp_d(void) { cp(registers.d); }

// 0xbb
inline void cp_e(void) { cp(registers.e); }

// 0xbc
inline void cp_h(void) { cp(registers.h); }

// 0xbd
inline void cp_l(void) { cp(registers.l); }

// 0xbe
inline void cp_hlp(void) { cp(readByte(registers.hl)); }

// 0xbf
inline void cp_a(void) { cp(registers.a); }

// 0xc0
inline void ret_nz(void) {
	if(FLAGS_ISZERO) cpu.ticks += 8;
	else {
		registers.pc = readShortFromStack();
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 20;
	}
}

// 0xc1
inline void pop_bc(void) { registers.bc = readShortFromStack(); }

// 0xc2
inline void jp_nz_nn(unsigned short operand) {
	if(FLAGS_ISZERO) cpu.ticks += 12;
	else {
		registers.pc = operand;
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 16;
	}
}

// 0xc3
inline void jp_nn(unsigned short operand) {
	registers.pc = operand;
#if DEBUG
	debugJump();
#endif
}

// 0xc4
inline void call_nz_nn(unsigned short operand) {
	if(FLAGS_ISZERO) cpu.ticks += 12;
	else {
		writeShortToStack(registers.pc);
		registers.pc = operand;
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 24;
	}
}

// 0xc5
inline void push_bc(void) { writeShortToStack(registers.bc); }

// 0xc6
inline void add_a_n(unsigned char operand) { add(&registers.a, operand); }

// 0xc7
inline void rst_0(void) { writeShortToStack(registers.pc); registers.pc = 0x0000; }

// 0xc8
inline void ret_z(void) {
	if(FLAGS_ISZERO) {
		registers.pc = readShortFromStack();
		cpu.ticks += 20;
	}
	else cpu.ticks += 8;
}

// 0xc9
inline void ret(void) { registers.pc = readShortFromStack(); }

// 0xca
inline void jp_z_nn(unsigned short operand) {
	if(FLAGS_ISZERO) {
		registers.pc = operand;
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 16;
	}
	else cpu.ticks += 12;
}

// 0xcb
// cb.c

// 0xcc
inline void call_z_nn(unsigned short operand) {
	if(FLAGS_ISZERO) {
		writeShortToStack(registers.pc);
		registers.pc = operand;
		cpu.ticks += 24;
	}
	else cpu.ticks += 12;
}

// 0xcd
inline void call_nn(unsigned short operand) { writeShortToStack(registers.pc); registers.pc = operand; }

// 0xce
inline void adc_n(unsigned char operand) { adc(operand); }

// 0xcf
inline void rst_08(void) { writeShortToStack(registers.pc); registers.pc = 0x0008; }

// 0xd0
inline void ret_nc(void) {
	if(FLAGS_ISCARRY) cpu.ticks += 8;
	else {
		registers.pc = readShortFromStack();
		cpu.ticks += 20;
	}
}

// 0xd1
inline void pop_de(void) { registers.de = readShortFromStack(); }

// 0xd2
inline void jp_nc_nn(unsigned short operand) {
	if(!FLAGS_ISCARRY) {
		registers.pc = operand;
		cpu.ticks += 16;
	}
	else cpu.ticks += 12;
}

// 0xd4
inline void call_nc_nn(unsigned short operand) {
	if(!FLAGS_ISCARRY) {
		writeShortToStack(registers.pc);
		registers.pc = operand;
		cpu.ticks += 24;
	}
	else cpu.ticks += 12;
}

// 0xd5
inline void push_de(void) { writeShortToStack(registers.de); }

// 0xd6
inline void sub_n(unsigned char operand) { sub(operand); }

// 0xd7
inline void rst_10(void) { writeShortToStack(registers.pc); registers.pc = 0x0010; }

// 0xd8
inline void ret_c(void) {
	if(FLAGS_ISCARRY) {
		registers.pc = readShortFromStack();
		cpu.ticks += 20;
	}
	else cpu.ticks += 8;
}

// 0xd9
// interrupts.c

// 0xda
inline void jp_c_nn(unsigned short operand) {
	if(FLAGS_ISCARRY) {
		registers.pc = operand;
#if DEBUG
		debugJump();
#endif
		cpu.ticks += 16;
	}
	else cpu.ticks += 12;
}

// 0xdc
inline void call_c_nn(unsigned short operand) {
	if(FLAGS_ISCARRY) {
		writeShortToStack(registers.pc);
		registers.pc = operand;
		cpu.ticks += 24;
	}
	else cpu.ticks += 12;
}

// 0xde
inline void sbc_n(unsigned char operand) { sbc(operand); }

// 0xdf
inline void rst_18(void) { writeShortToStack(registers.pc); registers.pc = 0x0018; }

// 0xe0
inline void ld_ff_n_ap(unsigned char operand) { writeByte(0xff00 + operand, registers.a); }

// 0xe1
inline void pop_hl(void) { registers.hl = readShortFromStack(); }

// 0xe2
inline void ld_ff_c_a(void) { writeByte(0xff00 + registers.c, registers.a); }

// 0xe5
inline void push_hl(void) { writeShortToStack(registers.hl); }

// 0xe6
inline void and_n(unsigned char operand) {
	registers.a &= operand;
	
	FLAGS_CLEAR(FLAGS_CARRY | FLAGS_NEGATIVE);
	FLAGS_SET(FLAGS_HALFCARRY);
	if(registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);
}

// 0xe7
inline void rst_20(void) { writeShortToStack(registers.pc); registers.pc = 0x0020; }

// 0xe8
inline void add_sp_n(char operand) {
	int result = registers.sp + operand;
	
	if(result & 0xffff0000) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	registers.sp = result & 0xffff;
	
	if(((registers.sp & 0x0f) + (operand & 0x0f)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	// _does_ clear the zero flag
	FLAGS_CLEAR(FLAGS_ZERO | FLAGS_NEGATIVE);
}

// 0xe9
inline void jp_hl(void) {
	registers.pc = registers.hl;
#if DEBUG
	debugJump();
#endif
}

// 0xea
inline void ld_nnp_a(unsigned short operand) { writeByte(operand, registers.a); }

// 0xee
inline void xor_n(unsigned char operand) { xor_a(operand); }

//0xef
inline void rst_28(void) { writeShortToStack(registers.pc); registers.pc = 0x0028; }

// 0xf0
inline void ld_ff_ap_n(unsigned char operand) { registers.a = readByte(0xff00 + operand); }

// 0xf1
inline void pop_af(void) { registers.af = readShortFromStack(); }

// 0xf2
inline void ld_a_ff_c(void) { registers.a = readByte(0xff00 + registers.c); }

// 0xf3
inline void di_inst(void) { interrupt.master = 0; }

// 0xf5
inline void push_af(void) { writeShortToStack(registers.af); }

// 0xf6
inline void or_n(unsigned char operand) { or_a(operand); }

// 0xf7
inline void rst_30(void) { writeShortToStack(registers.pc); registers.pc = 0x0030; }

// 0xf8
inline void ld_hl_sp_n(unsigned char operand) {
	int result = registers.sp + (signed char)operand;
	
	if(result & 0xffff0000) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if(((registers.sp & 0x0f) + (operand & 0x0f)) > 0x0f) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
	
	FLAGS_CLEAR(FLAGS_ZERO | FLAGS_NEGATIVE);
	
	registers.hl = (unsigned short)(result & 0xffff);
}

// 0xf9
inline void ld_sp_hl(void) { registers.sp = registers.hl; }

// 0xfa
inline void ld_a_nnp(unsigned short operand) { registers.a = readByte(operand); }

// 0xfb
inline void ei(void) { interrupt.master = 1; }

// 0xfe
inline void cp_n(unsigned char operand) {
	FLAGS_SET(FLAGS_NEGATIVE);
	
	if(registers.a == operand) FLAGS_SET(FLAGS_ZERO);
	else FLAGS_CLEAR(FLAGS_ZERO);
	
	if(operand > registers.a) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);
	
	if((operand & 0x0f) > (registers.a & 0x0f)) FLAGS_SET(FLAGS_HALFCARRY);
	else FLAGS_CLEAR(FLAGS_HALFCARRY);
}

//0xff
inline void rst_38(void) { writeShortToStack(registers.pc); registers.pc = 0x0038; }

void cpuStep() {
	if (cpu.stopped || cpu.halted) return;

	{
		TIME_SCOPE();

		for (int i = 0; i < CPU_BATCH; i++) {
#if DEBUG
			// General breakpoints
			//if(registers.pc == 0x034c) { // incorrect load
			//if(registers.pc == 0x0309) { // start of function which writes to ff80
			//if(registers.pc == 0x2a02) { // closer to function call which writes to ff80
			//if(registers.pc == 0x034c) { // function which writes to ffa6 timer

			//if(registers.pc == 0x036c) { // loop
			//if(registers.pc == 0x0040) { // vblank

			//if(registers.pc == 0x29fa) { // input
			//	realtimeDebugEnable = 1;
			//}

			if (realtimeDebugEnable) realtimeDebug();
#endif

			// perform inlined instruction op
			switch (readByte(registers.pc++)) {
				#define INSTRUCTION_0(name,numticks,func,id,code)   case id: DebugInstruction(name); func(); cpu.ticks += numticks; code break;
				#define INSTRUCTION_1(name,numticks,func,id,code)   case id: DebugInstruction(name); { unsigned char operand = readByte(registers.pc++); func(operand); cpu.ticks += numticks; code } break;
				#define INSTRUCTION_1S(name,numticks,func,id,code)  case id: DebugInstruction(name); { char operand = readByte(registers.pc++); func(operand); cpu.ticks += numticks; code } break;
				#define INSTRUCTION_2(name,numticks,func,id,code)   case id: DebugInstruction(name); { unsigned short operand = readShort(registers.pc++); ++registers.pc; func(operand); cpu.ticks += numticks; code } break;
				#include "cpu_instructions.inl"
				#undef INSTRUCTION_0
				#undef INSTRUCTION_1
				#undef INSTRUCTION_1S
				#undef INSTRUCTION_2
			}
		}
	}
}