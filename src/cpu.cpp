
#include "platform.h"

#include "debug.h"
#include "cpu.h"
#include "memory.h"
#include "interrupts.h"
#include "keys.h"
#include "gpu.h"
#include "display.h"
#include "main.h"
#include "cgb.h"
#include "snd.h"
#include "emulator.h"

cpu_type cpu ALIGN(256);

CT_ASSERT(sizeof(cpu.memory) == 0x100);

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

// max number of cpu instructions to batch between GPU/interrupt checks (higher = less accurate, faster emulation)
#define MAX_CPU_BATCH 12

// min number of cpu instructions to batch between GPU/interrupt checks (higher = less accurate, faster emulation)
#define MIN_CPU_BATCH 4

// number of batches to do between system checks
#define BATCHES 1024

void cpuReset(void) {
	memset(sram, 0, sizeof(sram));
	memcpy(&cpu.memory, ioReset, sizeof(cpu.memory));
	memset(oam, 0, sizeof(oam));
	memset(wram_perm, 0, sizeof(wram_perm));
	memset(wram_gb, 0, sizeof(wram_gb));
	
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
	
	cpu.gpuTick = 0;
	
	cpu.clocks = 0;
	cpu.stopped = 0;
	cpu.halted = 0;
	
	// TODO check this.. should be same as ioReset? Or does it serve a different purpose
	cpu.memory.P1_joypad = 0xCF;
	writeByte(0xFF05, 0);
	writeByte(0xFF06, 0);
	writeByte(0xFF07, 0);
	writeByte(0xFF10, 0x80);
	writeByte(0xFF11, 0xBF);
	writeByte(0xFF12, 0xF3);
	cpu.memory.all[0x14] = 0xBF;
	writeByte(0xFF16, 0x3F);
	writeByte(0xFF17, 0x00);
	cpu.memory.all[0x19] = 0xBF;
	writeByte(0xFF1A, 0x7A);
	writeByte(0xFF1B, 0xFF);
	writeByte(0xFF1C, 0x9F);
	cpu.memory.all[0x1E] = 0xBF;
	writeByte(0xFF20, 0xFF);
	writeByte(0xFF21, 0x00);
	writeByte(0xFF22, 0x00);
	cpu.memory.all[0x23] = 0xBF;
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
	gpuStep = stepLCDOn_OAM;
}

inline void undefined(void) {
	cpu.registers.pc--;
	
	unsigned char instruction = readByte(cpu.registers.pc);

#if DEBUG_BREAKPOINT
	HitBreakpoint();
#endif

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

void cb_n(unsigned int instruction);

inline unsigned char inc(unsigned char value) {
	value++;
		
	// carry is ignored!
	cpu.registers.f =
		((value == 0) << FLAGS_Z_BIT) |															// ZERO
		0 |																						// NEGATIVE
		(((value & 0x0f) == 0) << FLAGS_HC_BIT) |												// HALF-CARRY
		(FLAGS_ISCARRY);																		// CARRY
	
	return value;
}

inline unsigned char dec(unsigned char value) {
	value--;

	// carry is ignored!
	cpu.registers.f =
		((value == 0) << FLAGS_Z_BIT) |															// ZERO
		FLAGS_N |																				// NEGATIVE
		(((value & 0x0f) == 0x0f) << FLAGS_HC_BIT) |											// HALF-CARRY
		(FLAGS_ISCARRY);																		// CARRY
	
	return value;
}

static inline void add(unsigned char *destination, unsigned char value) {
	unsigned int result = *destination + value;
	
	cpu.registers.f =
		(((result & 0xff) == 0) << FLAGS_Z_BIT) |												// ZERO
		0 |																						// NEGATIVE
		((((*destination & 0x0f) + (value & 0x0f)) & 0x10) << (FLAGS_HC_BIT - 4)) |					// HALF-CARRY
		(((result & 0xff00) != 0) << FLAGS_C_BIT);												// CARRY

	*destination = (unsigned char)(result & 0xff);
}

static inline void add2(unsigned int *destination, unsigned short value) {
	unsigned int result = *destination + value;

	// zero flag left alone
	cpu.registers.f =
		(FLAGS_ISZERO) |																		// ZERO
		0 |																						// NEGATIVE
		(((((*destination & 0x0fff) + (value & 0x0fff)) & 0x1000) != 0) << FLAGS_HC_BIT) |		// HALF-CARRY
		(((result & 0xffff0000) != 0) << FLAGS_C_BIT);											// CARRY

	*destination = (result & 0xffff);
}

static inline void sub(unsigned char value) {
	cpu.registers.f =
		((cpu.registers.a == value) << FLAGS_Z_BIT) |						// ZERO
		FLAGS_N |															// NEGATIVE
		(((value & 0x0f) > (cpu.registers.a & 0x0f)) << FLAGS_HC_BIT) |		// HALF-CARRY
		((value > cpu.registers.a) << FLAGS_C_BIT);							// CARRY

	cpu.registers.a -= value;
}

// 0x00
inline void nop(void) {  }

// 0x01
inline void ld_bc_nn(unsigned short operand) { cpu.registers.bc = operand; }

// 0x02
inline void ld_bcp_a(void) { writeByte(cpu.registers.bc, cpu.registers.a); }

// 0x03
inline void inc_bc(void) { cpu.registers.bc++; cpu.registers.bc &= 0xFFFF; }

// 0x04
inline void inc_b(void) { cpu.registers.b = inc(cpu.registers.b); }

// 0x05
inline void dec_b(void) { cpu.registers.b = dec(cpu.registers.b); }

// 0x06
inline void ld_b_n(unsigned char operand) { cpu.registers.b = operand; }

// 0x07
inline void rlca(void) {
	cpu.registers.a = (cpu.registers.a << 1) | (cpu.registers.a >> 7);

	cpu.registers.f =
		0 |																	// ZERO
		0 |																	// NEGATIVE
		0 |																	// HALF-CARRY
		((cpu.registers.a & 0x01) << FLAGS_C_BIT);							// CARRY
}

// 0x08
inline void ld_nnp_sp(unsigned short operand) { writeShort(operand, cpu.registers.sp); }

// 0x09
inline void add_hl_bc(void) { add2(&cpu.registers.hl, cpu.registers.bc); }

// 0x0a
inline void ld_a_bcp(void) { cpu.registers.a = readByte(cpu.registers.bc); }

// 0x0b
inline void dec_bc(void) { cpu.registers.bc--; cpu.registers.bc &= 0xFFFF; }

// 0x0c
inline void inc_c(void) { cpu.registers.c = inc(cpu.registers.c); }

// 0x0d
inline void dec_c(void) { cpu.registers.c = dec(cpu.registers.c); }

// 0x0e
inline void ld_c_n(unsigned char operand) { cpu.registers.c = operand; }

// 0x0f
inline void rrca(void) {
	cpu.registers.a = (cpu.registers.a >> 1) | (cpu.registers.a << 7);

	cpu.registers.f =
		0 |																	// ZERO
		0 |																	// NEGATIVE
		0 |																	// HALF-CARRY
		((cpu.registers.a & 0x80) >> (7 - FLAGS_C_BIT));					// CARRY
}

// 0x10
void stop(unsigned char operand) {
	// are we attempting a speed switch?
	if (cgb.isCGB && (cpu.memory.KEY1_cgbspeed & 1)) {
		cgbSpeedSwitch();
	} else {
		cpu.stopped = 1;
	}
}

// 0x11
inline void ld_de_nn(unsigned short operand) { cpu.registers.de = operand; }

// 0x12
inline void ld_dep_a(void) { writeByte(cpu.registers.de, cpu.registers.a); }

// 0x13
inline void inc_de(void) { cpu.registers.de++; cpu.registers.de &= 0xFFFF; }

// 0x14
inline void inc_d(void) { cpu.registers.d = inc(cpu.registers.d); }

// 0x15
inline void dec_d(void) { cpu.registers.d = dec(cpu.registers.d); }

// 0x16
inline void ld_d_n(unsigned char operand) { cpu.registers.d = operand; }

// 0x17
inline void rla(void) {
	int carry = FLAGS_ISCARRY ? 1 : 0;

	cpu.registers.f =
		0 |																	// ZERO
		0 |																	// NEGATIVE
		0 |																	// HALF-CARRY
		((cpu.registers.a & 0x80) >> (7 - FLAGS_C_BIT));					// CARRY
	
	cpu.registers.a <<= 1;
	cpu.registers.a += carry;
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
inline void dec_de(void) { cpu.registers.de--; cpu.registers.de &= 0xFFFF; }

// 0x1c
inline void inc_e(void) { cpu.registers.e = inc(cpu.registers.e); }

// 0x1d
inline void dec_e(void) { cpu.registers.e = dec(cpu.registers.e); }

// 0x1e
inline void ld_e_n(unsigned char operand) { cpu.registers.e = operand; }

// 0x1f
inline void rra(void) {
	int carry = (FLAGS_ISCARRY ? 1 : 0) << 7;

	cpu.registers.f =
		0 |																	// ZERO
		0 |																	// NEGATIVE
		0 |																	// HALF-CARRY
		((cpu.registers.a & 0x01) << FLAGS_C_BIT);							// CARRY
	
	cpu.registers.a >>= 1;
	cpu.registers.a += carry;
}

// 0x20
inline void jr_nz_n(unsigned char operand) {
	if(FLAGS_ISZERO) cpu.clocks += 4;
	else {
		cpu.registers.pc += (signed char)operand;
		cpu.clocks += 8;
	}
}

// 0x21
inline void ld_hl_nn(unsigned short operand) { cpu.registers.hl = operand; }

// 0x22
inline void ldi_hlp_a(void) { writeByte(cpu.registers.hl++, cpu.registers.a); }

// 0x23
inline void inc_hl(void) { cpu.registers.hl++; cpu.registers.hl &= 0xFFFF; }

// 0x24
inline void inc_h(void) { cpu.registers.h = inc(cpu.registers.h); }

// 0x25
inline void dec_h(void) { cpu.registers.h = dec(cpu.registers.h); }

// 0x26
inline void ld_h_n(unsigned char operand) { cpu.registers.h = operand; }

// 0x27
void daa(void) {
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
		FLAGS_SET(FLAGS_C);
	}
	else {
		// no carry clear
	}

	s = s & 0xFF;
		
	if(s != 0) FLAGS_CLEAR(FLAGS_Z);
	else FLAGS_SET(FLAGS_Z);

	cpu.registers.a = (unsigned char) s;

	FLAGS_CLEAR(FLAGS_HC);
}

// 0x28
inline void jr_z_n(unsigned char operand) {
	if(FLAGS_ISZERO) {
		cpu.registers.pc += (signed char)operand;
		cpu.clocks += 8;
	}
	else cpu.clocks += 4;
}

// 0x29
inline void add_hl_hl(void) { add2(&cpu.registers.hl, cpu.registers.hl); }

// 0x2a
inline void ldi_a_hlp(void) { cpu.registers.a = readByte(cpu.registers.hl++); cpu.registers.de &= 0xFFFF; }

// 0x2b
inline void dec_hl(void) { cpu.registers.hl--; cpu.registers.hl &= 0xFFFF; }

// 0x2c
inline void inc_l(void) { cpu.registers.l = inc(cpu.registers.l); }

// 0x2d
inline void dec_l(void) { cpu.registers.l = dec(cpu.registers.l); }

// 0x2e
inline void ld_l_n(unsigned char operand) { cpu.registers.l = operand; }

// 0x2f
inline void cpl(void) { cpu.registers.a = ~cpu.registers.a; FLAGS_SET(FLAGS_N | FLAGS_HC); }

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
inline void scf(void) { 
	// set carry flag
	cpu.registers.f =
		(FLAGS_ISZERO) |							// ZERO
		0 |											// NEGATIVE
		0 |											// HALF-CARRY
		(1 << FLAGS_C_BIT);							// CARRY
}

// 0x38
inline void jr_c_n(char operand) {
	if(FLAGS_ISCARRY) {
		cpu.registers.pc += operand;
		cpu.clocks += 8;
	}
	else cpu.clocks += 4;
}

// 0x39
inline void add_hl_sp(void) { add2(&cpu.registers.hl, cpu.registers.sp); }

// 0x3a
inline void ldd_a_hlp(void) { cpu.registers.a = readByte(cpu.registers.hl--); cpu.registers.hl &= 0xFFFF; }

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
	// complement carry flag
	cpu.registers.f =
		(FLAGS_ISZERO) |							// ZERO
		0 |											// NEGATIVE
		0 |											// HALF-CARRY
		((FLAGS_ISCARRY == 0) << FLAGS_C_BIT);		// CARRY
}

// 0x40-0x47 (except 0x46)
inline void ld_b(unsigned char value) { cpu.registers.b = value; }

// 0x46
inline void ld_b_hlp(void) { cpu.registers.b = readByte(cpu.registers.hl); }

// 0x48-0x4f (except 0x4e)
inline void ld_c(unsigned char value) { cpu.registers.c = value; }

// 0x4e
inline void ld_c_hlp(void) { cpu.registers.c = readByte(cpu.registers.hl); }

// 0x50-0x57 (except 0x56)
inline void ld_d(unsigned char value) { cpu.registers.d = value; }

// 0x56
inline void ld_d_hlp(void) { cpu.registers.d = readByte(cpu.registers.hl); }

// 0x58-5f (except 0x5e)
inline void ld_e(unsigned char value) { cpu.registers.e = value; }

// 0x5e
inline void ld_e_hlp(void) { cpu.registers.e = readByte(cpu.registers.hl); }

// 0x60-67 (except 0x66)
inline void ld_h(unsigned char value) { cpu.registers.h = value; }

// 0x66
inline void ld_h_hlp(void) { cpu.registers.h = readByte(cpu.registers.hl); }

// 0x68-6f (except 0x6e)
inline void ld_l(unsigned char value) { cpu.registers.l = value; }

// 0x6e
inline void ld_l_hlp(void) { cpu.registers.l = readByte(cpu.registers.hl); }

// 0x70-0x77 (except 0x76)
inline void ld_hlp(unsigned char value) { writeByte(cpu.registers.hl, value); }

// 0x76
inline void halt(void) {
	cpu.halted = 1;
}

// 0x78-0x7f (except 0x7e)
inline void ld_a(unsigned char value) { cpu.registers.a = value; }

// 0x7e
inline void ld_a_hlp(void) { cpu.registers.a = readByte(cpu.registers.hl); }

// 0x80-0x87 (except 0x86)
inline void add_a(unsigned char value) { add(&cpu.registers.a, value); }

// 0x86
inline void add_a_hlp(void) { add(&cpu.registers.a, readByte(cpu.registers.hl)); }

// 0x88-0x8f (except 0x8e)
inline void adc(unsigned char value) {
	int result = cpu.registers.a + value + (FLAGS_ISCARRY ? 1 : 0);

	cpu.registers.f =
		(((result & 0xff) == 0) << FLAGS_Z_BIT) |															// ZERO
		0 |																									// NEGATIVE
		((((value & 0x0f) + (cpu.registers.a & 0x0f) + (FLAGS_ISCARRY ? 1 : 0)) & 0x10) << (FLAGS_HC_BIT-4)) |	// HALF-CARRY
		((result & 0x100) >> (8-FLAGS_C_BIT));																// CARRY

	cpu.registers.a = (unsigned char)(result & 0xff);
}

// 0x8e
inline void adc_hlp(void) { adc(readByte(cpu.registers.hl)); }

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


// 0x98-0x9f (except 0x9e)
inline void sbc(unsigned char value) {
	int result = cpu.registers.a - value - (FLAGS_ISCARRY ? 1 : 0);

	cpu.registers.f =
		(((result & 0xff) == 0) << FLAGS_Z_BIT) |														// ZERO
		FLAGS_N |																						// NEGATIVE
		(((value & 0x0f) + (FLAGS_ISCARRY ? 1 : 0) > (cpu.registers.a & 0x0f)) << FLAGS_HC_BIT) |		// HALF-CARRY
		((result & 0x100) >> (8-FLAGS_C_BIT));															// CARRY

	cpu.registers.a = (unsigned char)(result & 0xff);

}

// 0x9e
inline void sbc_hlp(void) { sbc(readByte(cpu.registers.hl)); }

// 0xa0-a7 (except 0xa6)
inline void and_op(unsigned char value) {
	cpu.registers.a &= value;

	cpu.registers.f =
		((cpu.registers.a == 0) << FLAGS_Z_BIT) |							// ZERO
		0 |																	// NEGATIVE
		FLAGS_HC |															// HALF-CARRY
		0;																	// CARRY
}

// 0xa6
inline void and_hlp(void) { and_op(readByte(cpu.registers.hl)); }

// 0xa8-0xaf (except 0xae)
inline void xor_op(unsigned char value) {
	cpu.registers.a ^= value;

	cpu.registers.f =
		((cpu.registers.a == 0) << FLAGS_Z_BIT) |							// ZERO
		0 |																	// NEGATIVE
		0 |																	// HALF-CARRY
		0;																	// CARRY
}

// 0xae
inline void xor_hlp(void) { xor_op(readByte(cpu.registers.hl)); }

// 0xb0-b8 (except 0xb7)
inline void or_op(unsigned char value) {
	cpu.registers.a |= value;

	cpu.registers.f =
		((cpu.registers.a == 0) << FLAGS_Z_BIT) |							// ZERO
		0 |																	// NEGATIVE
		0 |																	// HALF-CARRY
		0;																	// CARRY
}

// 0xb6
inline void or_hlp(void) { or_op(readByte(cpu.registers.hl)); }

// 0xb8-bf (except 0xbe)
inline void cp(unsigned char value) {
	cpu.registers.f =
		((cpu.registers.a == value) << FLAGS_Z_BIT) |						// ZERO
		FLAGS_N |															// NEGATIVE
		(((value & 0x0f) > (cpu.registers.a & 0x0f)) << FLAGS_HC_BIT) |		// HALF-CARRY
		((value > cpu.registers.a) << 4);									// CARRY
}

// 0xbe
inline void cp_hlp(void) { cp(readByte(cpu.registers.hl)); }

// 0xc0
inline void ret_nz(void) {
	if(FLAGS_ISZERO) cpu.clocks += 4;
	else {
		cpu.registers.pc = readShortFromStack();
		cpu.clocks += 16;
	}
}

// 0xc1
inline void pop_bc(void) { cpu.registers.bc = readShortFromStack(); }

// 0xc2
inline void jp_nz_nn(unsigned short operand) {
	if(FLAGS_ISZERO) cpu.clocks += 8;
	else {
		cpu.registers.pc = operand;
		cpu.clocks += 12;
	}
}

// 0xc3
inline void jp_nn(unsigned short operand) {
	cpu.registers.pc = operand;
}

// 0xc4
inline void call_nz_nn(unsigned short operand) {
	if(FLAGS_ISZERO) cpu.clocks += 8;
	else {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 20;
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
		cpu.clocks += 16;
	}
	else cpu.clocks += 4;
}

// 0xc9
inline void ret(void) { cpu.registers.pc = readShortFromStack(); }

// 0xca
inline void jp_z_nn(unsigned short operand) {
	if(FLAGS_ISZERO) {
		cpu.registers.pc = operand;
		cpu.clocks += 12;
	}
	else cpu.clocks += 8;
}

// 0xcb
// cb.c

// 0xcc
inline void call_z_nn(unsigned short operand) {
	if(FLAGS_ISZERO) {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 20;
	}
	else cpu.clocks += 8;
}

// 0xcd
inline void call_nn(unsigned short operand) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = operand; }

// 0xce
inline void adc_n(unsigned char operand) { adc(operand); }

// 0xcf
inline void rst_08(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0008; }

// 0xd0
inline void ret_nc(void) {
	if(FLAGS_ISCARRY) cpu.clocks += 4;
	else {
		cpu.registers.pc = readShortFromStack();
		cpu.clocks += 16;
	}
}

// 0xd1
inline void pop_de(void) { cpu.registers.de = readShortFromStack(); }

// 0xd2
inline void jp_nc_nn(unsigned short operand) {
	if(!FLAGS_ISCARRY) {
		cpu.registers.pc = operand;
		cpu.clocks += 12;
	}
	else cpu.clocks += 8;
}

// 0xd4
inline void call_nc_nn(unsigned short operand) {
	if(!FLAGS_ISCARRY) {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 20;
	}
	else cpu.clocks += 8;
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
		cpu.clocks += 16;
	}
	else cpu.clocks += 4;
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
		cpu.clocks += 12;
	}
	else cpu.clocks += 8;
}

// 0xdc
inline void call_c_nn(unsigned short operand) {
	if(FLAGS_ISCARRY) {
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = operand;
		cpu.clocks += 20;
	}
	else cpu.clocks += 8;
}

// 0xde
inline void sbc_n(unsigned char operand) { sbc(operand); }

// 0xdf
inline void rst_18(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0018; }

// 0xe0
inline void ld_ff_n_ap(unsigned char operand) {
	if (specialMap[operand] & 0x02)
		writeByteSpecial(operand, cpu.registers.a);
	else
		cpu.memory.all[operand] = cpu.registers.a;
}

// 0xe1
inline void pop_hl(void) { cpu.registers.hl = readShortFromStack(); }

// 0xe2
inline void ld_ff_c_a(void) {
	if (specialMap[cpu.registers.c] & 0x02)
		writeByteSpecial(cpu.registers.c, cpu.registers.a);
	else
		cpu.memory.all[cpu.registers.c] = cpu.registers.a;
}

// 0xe5
inline void push_hl(void) { writeShortToStack(cpu.registers.hl); }

// 0xe7
inline void rst_20(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0020; }

// 0xe8
inline void add_sp_n(unsigned char operand) {
	unsigned int result = cpu.registers.sp + operand;

	// _does_ clear the zero flag
	cpu.registers.f =
		0 |																						// ZERO
		0 |																						// NEGATIVE
		(((cpu.registers.sp & 0x000f) + (operand & 0x0f) > 0x0f) << FLAGS_HC_BIT) |				// HALF-CARRY
		(((result & 0xff00) != (cpu.registers.sp & 0xff00)) << FLAGS_C_BIT);					// CARRY

	// signed result
	if (operand & 0x80) {
		result -= 0x0100;
	}

	cpu.registers.sp = result & 0xffff;
}

// 0xe9
inline void jp_hl(void) {
	cpu.registers.pc = cpu.registers.hl;
}

// 0xea
inline void ld_nnp_a(unsigned short operand) { writeByte(operand, cpu.registers.a); }

// 0xee
inline void xor_n(unsigned char operand) { xor_op(operand); }

//0xef
inline void rst_28(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0028; }

// 0xf0
inline void ld_ff_ap_n(unsigned char operand) {
	cpu.registers.a = (specialMap[operand] & 0x01) ? readByteSpecial(0xFF00 | operand) : cpu.memory.all[operand];
}

// 0xf1
inline void pop_af(void) { cpu.registers.af = readShortFromStack(); cpu.registers.f &= 0xF0; }

// 0xf2
inline void ld_a_ff_c(void) {
	cpu.registers.a = (specialMap[cpu.registers.c] & 0x01) ? readByteSpecial(0xFF00 | cpu.registers.c) : cpu.memory.all[cpu.registers.c];
}

// 0xf3
inline void di_inst(void) { cpu.IME = 0; }

// 0xf5
inline void push_af(void) { writeShortToStack(cpu.registers.af); }

// 0xf6
inline void or_n(unsigned char operand) { or_op(operand); }

// 0xf7
inline void rst_30(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0030; }

// 0xf8
void ld_hl_sp_n(unsigned char operand) {
	int result = cpu.registers.sp + operand;

	cpu.registers.f =
		0 |																						// ZERO
		0 |																						// NEGATIVE
		(((cpu.registers.sp & 0x000f) + (operand & 0x0f) > 0x0f) << FLAGS_HC_BIT) |				// HALF-CARRY
		(((result & 0xff00) != (cpu.registers.sp & 0xff00)) << FLAGS_C_BIT);					// CARRY

	// signed result
	if (operand & 0x80) {
		result -= 0x0100;
	}
	
	cpu.registers.hl = (unsigned short)(result & 0xffff);
}

// 0xf9
inline void ld_sp_hl(void) { cpu.registers.sp = cpu.registers.hl; }

// 0xfa
inline void ld_a_nnp(unsigned short operand) { cpu.registers.a = readByte(operand); }

// 0xfb
inline void ei(void) { cpu.IME = 1; }

//0xff
inline void rst_38(void) { writeShortToStack(cpu.registers.pc); cpu.registers.pc = 0x0038; }

// extended instruction set
#include "cb_impl.inl"

static unsigned char* const regMap[8] = {
	&cpu.registers.b,
	&cpu.registers.c,
	&cpu.registers.d,
	&cpu.registers.e,
	&cpu.registers.h,
	&cpu.registers.l,
	NULL,
	&cpu.registers.a
};

static const char* regNames[8] = {
	"B",
	"C",
	"D",
	"E",
	"H",
	"L",
	"(HL)",
	"A"
};

#define INSTRUCTION_0(name,numticks,func,id,code)   case id: DebugInstruction(name); func(); cpu.clocks += (numticks - 4); code break;
#define INSTRUCTION_1(name,numticks,func,id,code)   case id: DebugInstruction(name, pc[1]); { cpu.registers.pc += 1; func(pc[1]); cpu.clocks += (numticks - 4); code } break;
#define INSTRUCTION_1S(name,numticks,func,id,code)  case id: DebugInstruction(name, pc[1]); { cpu.registers.pc += 1; func((signed char) pc[1]); cpu.clocks += (numticks - 4); code } break;
#define INSTRUCTION_2(name,numticks,func,id,code)   case id: DebugInstruction(name, pc[1] | (pc[2] << 8)); { cpu.registers.pc += 2; func(pc[1] | (pc[2] << 8)); cpu.clocks += (numticks - 4); code } break;
#define INSTRUCTION_L(name,numticks,func,id,code)   case id: 
#define INSTRUCTION_E(name,numticks,func,id,code)   case id: DebugInstructionMapped(name, regNames[pc[0] & 7]); func(*regMap[pc[0] & 7]); cpu.clocks += (numticks - 4); code break;
#define CB_INSTRUCTION(name,numticks,func,id,code)  case id: DebugInstruction(name); func(); cpu.clocks += (numticks - 4); code break;
#define CB_INSTR______(name,numticks,func,id,code)  case id: 
#define CB_INSTRMAPPED(name,numticks,func,id,code)  case id: DebugInstructionMapped(name, regNames[operand & 7]); func(*regMap[operand & 7]); cpu.clocks += (numticks - 4); code break;

#define LEAVE_LOOP {cpu.clocks -= (numInstr - i - 1) * 4; i = numInstr;}

void cb_n(int operand);

void cpuStep() {
	{
		TIME_SCOPE();

		for (int b = 0; b < BATCHES; b++) {
			if (cpu.stopped || cpu.halted) {
				// just advance the clock til something happens
				int numClocks = max(cpu.gpuTick - cpu.clocks, 4);

				if (cpu.memory.TAC_timerctl & 0x04) {
					numClocks = min(cpu.timerInterrupt - cpu.clocks, numClocks);
				}

				cpu.clocks += numClocks;
			} else {
				// 8 clocks per instruction is about the average from empirical testing
				int numInstr = min(max(cpu.gpuTick - cpu.clocks, (MIN_CPU_BATCH * 8)) / 8, MAX_CPU_BATCH);

				if (cpu.memory.TAC_timerctl & 0x04) {
					numInstr = min(max(cpu.timerInterrupt - cpu.clocks, (MIN_CPU_BATCH * 8)) / 8, numInstr);
				}

				// instructions start with a "base" of 4 clocks a piece
				cpu.clocks += numInstr * 4;

				for (int i = 0; i < numInstr; i++) {
					DebugPC(cpu.registers.pc);
					unsigned char* pc = getInstrByte(cpu.registers.pc++);
					// perform inlined instruction op
					switch (pc[0]) {
						// main instruction set
						#include "cpu_instructions.inl"
						// handle extended instruction set call
						case 0xcb:
							cb_n(pc[1]);
							break; 
							// unknown instruction
						default:
							undefined();
							break;
					}
				}
			}

			if (gpuCheck()) gpuStep();
			if (interruptCheck()) interruptStep();
		}
	}
}

void cb_n(int operand) {
	cpu.registers.pc += 1;
	switch (operand) {
#include "cb_instructions.inl"
	}
}

#undef INSTRUCTION_0
#undef INSTRUCTION_1
#undef INSTRUCTION_1S
#undef INSTRUCTION_2
#undef INSTRUCTION_L
#undef INSTRUCTION_E
#undef CB_INSTRUCTION