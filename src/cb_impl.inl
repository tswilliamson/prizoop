#pragma once

// Implementation of extended instruction set functions
inline unsigned char rlc(unsigned char value) {
	value = (value << 1) | (value >> 7);

	if (value & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline unsigned char rrc(unsigned char value) {
	value = (value >> 1) | ((value << 7) & 0x80);

	if (value & 0x80) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline unsigned char rl(unsigned char value) {
	if (value & 0x80) {
		value <<= 1;
		if (FLAGS_ISCARRY) value |= 0x01;
		FLAGS_SET(FLAGS_CARRY);
	}
	else {
		value <<= 1;
		if (FLAGS_ISCARRY) value |= 0x01;
		FLAGS_CLEAR(FLAGS_CARRY);
	}

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline unsigned char rr(unsigned char value) {
	if (value & 0x01) {
		value >>= 1;
		if (FLAGS_ISCARRY) value |= 0x80;
		FLAGS_SET(FLAGS_CARRY);
	}
	else {
		value >>= 1;
		if (FLAGS_ISCARRY) value |= 0x80;
		FLAGS_CLEAR(FLAGS_CARRY);
	}

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline unsigned char sla(unsigned char value) {
	if (value & 0x80) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	value <<= 1;

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline unsigned char sra(unsigned char value) {
	if (value & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	value = (value & 0x80) | (value >> 1);

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline unsigned char swap(unsigned char value) {
	value = ((value & 0xf) << 4) | ((value & 0xf0) >> 4);

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY | FLAGS_CARRY);

	return value;
}

static unsigned char srl(unsigned char value) {
	if (value & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	value >>= 1;

	if (value) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);

	return value;
}

inline void bit(unsigned char bit, unsigned char value) {
	if (value & bit) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE);
	FLAGS_SET(FLAGS_HALFCARRY);
}

inline void set(unsigned char bit, unsigned char& value) {
	value |= bit;
}

// 0x00
inline void rlc_b(void) { cpu.registers.b = rlc(cpu.registers.b); }

// 0x01
inline void rlc_c(void) { cpu.registers.c = rlc(cpu.registers.c); }

// 0x02
inline void rlc_d(void) { cpu.registers.d = rlc(cpu.registers.d); }

// 0x03
inline void rlc_e(void) { cpu.registers.e = rlc(cpu.registers.e); }

// 0x04
inline void rlc_h(void) { cpu.registers.h = rlc(cpu.registers.h); }

// 0x05
inline void rlc_l(void) { cpu.registers.l = rlc(cpu.registers.l); }

// 0x06
inline void rlc_hlp(void) { writeByte(cpu.registers.hl, rlc(readByte(cpu.registers.hl))); }

// 0x07
inline void rlc_a(void) { cpu.registers.a = rlc(cpu.registers.a); }

// 0x08
inline void rrc_b(void) { cpu.registers.b = rrc(cpu.registers.b); }

// 0x09
inline void rrc_c(void) { cpu.registers.c = rrc(cpu.registers.c); }

// 0x0a
inline void rrc_d(void) { cpu.registers.d = rrc(cpu.registers.d); }

// 0x0b
inline void rrc_e(void) { cpu.registers.e = rrc(cpu.registers.e); }

// 0x0c
inline void rrc_h(void) { cpu.registers.h = rrc(cpu.registers.h); }

// 0x0d
inline void rrc_l(void) { cpu.registers.l = rrc(cpu.registers.l); }

// 0x0e
inline void rrc_hlp(void) { writeByte(cpu.registers.hl, rrc(readByte(cpu.registers.hl))); }

// 0x0f
inline void rrc_a(void) { cpu.registers.a = rrc(cpu.registers.a); }

// 0x10
inline void rl_b(void) { cpu.registers.b = rl(cpu.registers.b); }

// 0x11
inline void rl_c(void) { cpu.registers.c = rl(cpu.registers.c); }

// 0x12
inline void rl_d(void) { cpu.registers.d = rl(cpu.registers.d); }

// 0x13
inline void rl_e(void) { cpu.registers.e = rl(cpu.registers.e); }

// 0x14
inline void rl_h(void) { cpu.registers.h = rl(cpu.registers.h); }

// 0x15
inline void rl_l(void) { cpu.registers.l = rl(cpu.registers.l); }

// 0x16
inline void rl_hlp(void) { writeByte(cpu.registers.hl, rl(readByte(cpu.registers.hl))); }

// 0x17
inline void rl_a(void) { cpu.registers.a = rl(cpu.registers.a); }

// 0x18
inline void rr_b(void) { cpu.registers.b = rr(cpu.registers.b); }

// 0x19
inline void rr_c(void) { cpu.registers.c = rr(cpu.registers.c); }

// 0x1a
inline void rr_d(void) { cpu.registers.d = rr(cpu.registers.d); }

// 0x1b
inline void rr_e(void) { cpu.registers.e = rr(cpu.registers.e); }

// 0x1c
inline void rr_h(void) { cpu.registers.h = rr(cpu.registers.h); }

// 0x1d
inline void rr_l(void) { cpu.registers.l = rr(cpu.registers.l); }

// 0x1e
inline void rr_hlp(void) { writeByte(cpu.registers.hl, rr(readByte(cpu.registers.hl))); }

// 0x1f
inline void rr_a(void) { cpu.registers.a = rr(cpu.registers.a); }

// 0x20
inline void sla_b(void) { cpu.registers.b = sla(cpu.registers.b); }

// 0x21
inline void sla_c(void) { cpu.registers.c = sla(cpu.registers.c); }

// 0x22
inline void sla_d(void) { cpu.registers.d = sla(cpu.registers.d); }

// 0x23
inline void sla_e(void) { cpu.registers.e = sla(cpu.registers.e); }

// 0x24
inline void sla_h(void) { cpu.registers.h = sla(cpu.registers.h); }

// 0x25
inline void sla_l(void) { cpu.registers.l = sla(cpu.registers.l); }

// 0x26
inline void sla_hlp(void) { writeByte(cpu.registers.hl, sla(readByte(cpu.registers.hl))); }

// 0x27
inline void sla_a(void) { cpu.registers.a = sla(cpu.registers.a); }

// 0x28
inline void sra_b(void) { cpu.registers.b = sra(cpu.registers.b); }

// 0x29
inline void sra_c(void) { cpu.registers.c = sra(cpu.registers.c); }

// 0x2a
inline void sra_d(void) { cpu.registers.d = sra(cpu.registers.d); }

// 0x2b
inline void sra_e(void) { cpu.registers.e = sra(cpu.registers.e); }

// 0x2c
inline void sra_h(void) { cpu.registers.h = sra(cpu.registers.h); }

// 0x2d
inline void sra_l(void) { cpu.registers.l = sra(cpu.registers.l); }

// 0x2e
inline void sra_hlp(void) { writeByte(cpu.registers.hl, sra(readByte(cpu.registers.hl))); }

// 0x2f
inline void sra_a(void) { cpu.registers.a = sra(cpu.registers.a); }

// 0x30
inline void swap_b(void) { cpu.registers.b = swap(cpu.registers.b); }

// 0x31
inline void swap_c(void) { cpu.registers.c = swap(cpu.registers.c); }

// 0x32
inline void swap_d(void) { cpu.registers.d = swap(cpu.registers.d); }

// 0x33
inline void swap_e(void) { cpu.registers.e = swap(cpu.registers.e); }

// 0x34
inline void swap_h(void) { cpu.registers.h = swap(cpu.registers.h); }

// 0x35
inline void swap_l(void) { cpu.registers.l = swap(cpu.registers.l); }

// 0x36
inline void swap_hlp(void) { writeByte(cpu.registers.hl, swap(readByte(cpu.registers.hl))); }

// 0x37
inline void swap_a(void) { cpu.registers.a = swap(cpu.registers.a); }

// 0x38
inline void srl_b(void) { cpu.registers.b = srl(cpu.registers.b); }

// 0x39
inline void srl_c(void) { cpu.registers.c = srl(cpu.registers.c); }

// 0x3a
inline void srl_d(void) { cpu.registers.d = srl(cpu.registers.d); }

// 0x3b
inline void srl_e(void) { cpu.registers.e = srl(cpu.registers.e); }

// 0x3c
inline void srl_h(void) { cpu.registers.h = srl(cpu.registers.h); }

// 0x3d
inline void srl_l(void) { cpu.registers.l = srl(cpu.registers.l); }

// 0x3e
inline void srl_hlp(void) { writeByte(cpu.registers.hl, srl(readByte(cpu.registers.hl))); }

// 0x3f
inline void srl_a(void) {
	if (cpu.registers.a & 0x01) FLAGS_SET(FLAGS_CARRY);
	else FLAGS_CLEAR(FLAGS_CARRY);

	cpu.registers.a >>= 1;

	if (cpu.registers.a) FLAGS_CLEAR(FLAGS_ZERO);
	else FLAGS_SET(FLAGS_ZERO);

	FLAGS_CLEAR(FLAGS_NEGATIVE | FLAGS_HALFCARRY);
}

// 0x40
inline void bit_0_b(void) { bit(1 << 0, cpu.registers.b); }

// 0x41
inline void bit_0_c(void) { bit(1 << 0, cpu.registers.c); }

// 0x42
inline void bit_0_d(void) { bit(1 << 0, cpu.registers.d); }

// 0x43
inline void bit_0_e(void) { bit(1 << 0, cpu.registers.e); }

// 0x44
inline void bit_0_h(void) { bit(1 << 0, cpu.registers.h); }

// 0x45
inline void bit_0_l(void) { bit(1 << 0, cpu.registers.l); }

// 0x46
inline void bit_0_hlp(void) { bit(1 << 0, readByte(cpu.registers.hl)); }

// 0x47
inline void bit_0_a(void) { bit(1 << 0, cpu.registers.a); }

// 0x48
inline void bit_1_b(void) { bit(1 << 1, cpu.registers.b); }

// 0x49
inline void bit_1_c(void) { bit(1 << 1, cpu.registers.c); }

// 0x4a
inline void bit_1_d(void) { bit(1 << 1, cpu.registers.d); }

// 0x4b
inline void bit_1_e(void) { bit(1 << 1, cpu.registers.e); }

// 0x4c
inline void bit_1_h(void) { bit(1 << 1, cpu.registers.h); }

// 0x4d
inline void bit_1_l(void) { bit(1 << 1, cpu.registers.l); }

// 0x4e
inline void bit_1_hlp(void) { bit(1 << 1, readByte(cpu.registers.hl)); }

// 0x4f
inline void bit_1_a(void) { bit(1 << 1, cpu.registers.a); }

// 0x50
inline void bit_2_b(void) { bit(1 << 2, cpu.registers.b); }

// 0x51
inline void bit_2_c(void) { bit(1 << 2, cpu.registers.c); }

// 0x52
inline void bit_2_d(void) { bit(1 << 2, cpu.registers.d); }

// 0x53
inline void bit_2_e(void) { bit(1 << 2, cpu.registers.e); }

// 0x54
inline void bit_2_h(void) { bit(1 << 2, cpu.registers.h); }

// 0x55
inline void bit_2_l(void) { bit(1 << 2, cpu.registers.l); }

// 0x56
inline void bit_2_hlp(void) { bit(1 << 2, readByte(cpu.registers.hl)); }

// 0x57
inline void bit_2_a(void) { bit(1 << 2, cpu.registers.a); }

// 0x58
inline void bit_3_b(void) { bit(1 << 3, cpu.registers.b); }

// 0x59
inline void bit_3_c(void) { bit(1 << 3, cpu.registers.c); }

// 0x5a
inline void bit_3_d(void) { bit(1 << 3, cpu.registers.d); }

// 0x5b
inline void bit_3_e(void) { bit(1 << 3, cpu.registers.e); }

// 0x5c
inline void bit_3_h(void) { bit(1 << 3, cpu.registers.h); }

// 0x5d
inline void bit_3_l(void) { bit(1 << 3, cpu.registers.l); }

// 0x5e
inline void bit_3_hlp(void) { bit(1 << 3, readByte(cpu.registers.hl)); }

// 0x5f
inline void bit_3_a(void) { bit(1 << 3, cpu.registers.a); }

// 0x60
inline void bit_4_b(void) { bit(1 << 4, cpu.registers.b); }

// 0x61
inline void bit_4_c(void) { bit(1 << 4, cpu.registers.c); }

// 0x62
inline void bit_4_d(void) { bit(1 << 4, cpu.registers.d); }

// 0x63
inline void bit_4_e(void) { bit(1 << 4, cpu.registers.e); }

// 0x64
inline void bit_4_h(void) { bit(1 << 4, cpu.registers.h); }

// 0x65
inline void bit_4_l(void) { bit(1 << 4, cpu.registers.l); }

// 0x66
inline void bit_4_hlp(void) { bit(1 << 4, readByte(cpu.registers.hl)); }

// 0x67
inline void bit_4_a(void) { bit(1 << 4, cpu.registers.a); }

// 0x68
inline void bit_5_b(void) { bit(1 << 5, cpu.registers.b); }

// 0x69
inline void bit_5_c(void) { bit(1 << 5, cpu.registers.c); }

// 0x6a
inline void bit_5_d(void) { bit(1 << 5, cpu.registers.d); }

// 0x6b
inline void bit_5_e(void) { bit(1 << 5, cpu.registers.e); }

// 0x6c
inline void bit_5_h(void) { bit(1 << 5, cpu.registers.h); }

// 0x6d
inline void bit_5_l(void) { bit(1 << 5, cpu.registers.l); }

// 0x6e
inline void bit_5_hlp(void) { bit(1 << 5, readByte(cpu.registers.hl)); }

// 0x6f
inline void bit_5_a(void) { bit(1 << 5, cpu.registers.a); }

// 0x70
inline void bit_6_b(void) { bit(1 << 6, cpu.registers.b); }

// 0x71
inline void bit_6_c(void) { bit(1 << 6, cpu.registers.c); }

// 0x72
inline void bit_6_d(void) { bit(1 << 6, cpu.registers.d); }

// 0x73
inline void bit_6_e(void) { bit(1 << 6, cpu.registers.e); }

// 0x74
inline void bit_6_h(void) { bit(1 << 6, cpu.registers.h); }

// 0x75
inline void bit_6_l(void) { bit(1 << 6, cpu.registers.l); }

// 0x76
inline void bit_6_hlp(void) { bit(1 << 6, readByte(cpu.registers.hl)); }

// 0x77
inline void bit_6_a(void) { bit(1 << 6, cpu.registers.a); }

// 0x78
inline void bit_7_b(void) { bit(1 << 7, cpu.registers.b); }

// 0x79
inline void bit_7_c(void) { bit(1 << 7, cpu.registers.c); }

// 0x7a
inline void bit_7_d(void) { bit(1 << 7, cpu.registers.d); }

// 0x7b
inline void bit_7_e(void) { bit(1 << 7, cpu.registers.e); }

// 0x7c
inline void bit_7_h(void) { bit(1 << 7, cpu.registers.h); }

// 0x7d
inline void bit_7_l(void) { bit(1 << 7, cpu.registers.l); }

// 0x7e
inline void bit_7_hlp(void) { bit(1 << 7, readByte(cpu.registers.hl)); }

// 0x7f
inline void bit_7_a(void) { bit(1 << 7, cpu.registers.a); }

// 0x80
inline void res_0_b(void) { cpu.registers.b &= ~(1 << 0); }

// 0x81
inline void res_0_c(void) { cpu.registers.c &= ~(1 << 0); }

// 0x82
inline void res_0_d(void) { cpu.registers.d &= ~(1 << 0); }

// 0x83
inline void res_0_e(void) { cpu.registers.e &= ~(1 << 0); }

// 0x84
inline void res_0_h(void) { cpu.registers.h &= ~(1 << 0); }

// 0x85
inline void res_0_l(void) { cpu.registers.l &= ~(1 << 0); }

// 0x86
inline void res_0_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 0)); }

// 0x87
inline void res_0_a(void) { cpu.registers.a &= ~(1 << 0); }

// 0x88
inline void res_1_b(void) { cpu.registers.b &= ~(1 << 1); }

// 0x89
inline void res_1_c(void) { cpu.registers.c &= ~(1 << 1); }

// 0x8a
inline void res_1_d(void) { cpu.registers.d &= ~(1 << 1); }

// 0x8b
inline void res_1_e(void) { cpu.registers.e &= ~(1 << 1); }

// 0x8c
inline void res_1_h(void) { cpu.registers.h &= ~(1 << 1); }

// 0x8d
inline void res_1_l(void) { cpu.registers.l &= ~(1 << 1); }

// 0x8e
inline void res_1_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 1)); }

// 0x8f
inline void res_1_a(void) { cpu.registers.a &= ~(1 << 1); }

// 0x90
inline void res_2_b(void) { cpu.registers.b &= ~(1 << 2); }

// 0x91
inline void res_2_c(void) { cpu.registers.c &= ~(1 << 2); }

// 0x92
inline void res_2_d(void) { cpu.registers.d &= ~(1 << 2); }

// 0x93
inline void res_2_e(void) { cpu.registers.e &= ~(1 << 2); }

// 0x94
inline void res_2_h(void) { cpu.registers.h &= ~(1 << 2); }

// 0x95
inline void res_2_l(void) { cpu.registers.l &= ~(1 << 2); }

// 0x96
inline void res_2_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 2)); }

// 0x97
inline void res_2_a(void) { cpu.registers.a &= ~(1 << 2); }

// 0x98
inline void res_3_b(void) { cpu.registers.b &= ~(1 << 3); }

// 0x99
inline void res_3_c(void) { cpu.registers.c &= ~(1 << 3); }

// 0x9a
inline void res_3_d(void) { cpu.registers.d &= ~(1 << 3); }

// 0x9b
inline void res_3_e(void) { cpu.registers.e &= ~(1 << 3); }

// 0x9c
inline void res_3_h(void) { cpu.registers.h &= ~(1 << 3); }

// 0x9d
inline void res_3_l(void) { cpu.registers.l &= ~(1 << 3); }

// 0x9e
inline void res_3_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 3)); }

// 0x9f
inline void res_3_a(void) { cpu.registers.a &= ~(1 << 3); }

// 0xa0
inline void res_4_b(void) { cpu.registers.b &= ~(1 << 4); }

// 0xa1
inline void res_4_c(void) { cpu.registers.c &= ~(1 << 4); }

// 0xa2
inline void res_4_d(void) { cpu.registers.d &= ~(1 << 4); }

// 0xa3
inline void res_4_e(void) { cpu.registers.e &= ~(1 << 4); }

// 0xa4
inline void res_4_h(void) { cpu.registers.h &= ~(1 << 4); }

// 0xa5
inline void res_4_l(void) { cpu.registers.l &= ~(1 << 4); }

// 0xa6
inline void res_4_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 4)); }

// 0xa7
inline void res_4_a(void) { cpu.registers.a &= ~(1 << 4); }

// 0xa8
inline void res_5_b(void) { cpu.registers.b &= ~(1 << 5); }

// 0xa9
inline void res_5_c(void) { cpu.registers.c &= ~(1 << 5); }

// 0xaa
inline void res_5_d(void) { cpu.registers.d &= ~(1 << 5); }

// 0xab
inline void res_5_e(void) { cpu.registers.e &= ~(1 << 5); }

// 0xac
inline void res_5_h(void) { cpu.registers.h &= ~(1 << 5); }

// 0xad
inline void res_5_l(void) { cpu.registers.l &= ~(1 << 5); }

// 0xae
inline void res_5_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 5)); }

// 0xaf
inline void res_5_a(void) { cpu.registers.a &= ~(1 << 5); }

// 0xb0
inline void res_6_b(void) { cpu.registers.b &= ~(1 << 6); }

// 0xb1
inline void res_6_c(void) { cpu.registers.c &= ~(1 << 6); }

// 0xb2
inline void res_6_d(void) { cpu.registers.d &= ~(1 << 6); }

// 0xb3
inline void res_6_e(void) { cpu.registers.e &= ~(1 << 6); }

// 0xb4
inline void res_6_h(void) { cpu.registers.h &= ~(1 << 6); }

// 0xb5
inline void res_6_l(void) { cpu.registers.l &= ~(1 << 6); }

// 0xb6
inline void res_6_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 6)); }

// 0xb7
inline void res_6_a(void) { cpu.registers.a &= ~(1 << 6); }

// 0xb8
inline void res_7_b(void) { cpu.registers.b &= ~(1 << 7); }

// 0xb9
inline void res_7_c(void) { cpu.registers.c &= ~(1 << 7); }

// 0xba
inline void res_7_d(void) { cpu.registers.d &= ~(1 << 7); }

// 0xbb
inline void res_7_e(void) { cpu.registers.e &= ~(1 << 7); }

// 0xbc
inline void res_7_h(void) { cpu.registers.h &= ~(1 << 7); }

// 0xbd
inline void res_7_l(void) { cpu.registers.l &= ~(1 << 7); }

// 0xbe
inline void res_7_hlp(void) { writeByte(cpu.registers.hl, readByte(cpu.registers.hl) & ~(1 << 7)); }

// 0xbf
inline void res_7_a(void) { cpu.registers.a &= ~(1 << 7); }

// 0xc0
inline void set_0_b(void) { set(1 << 0, cpu.registers.b); }

// 0xc1
inline void set_0_c(void) { set(1 << 0, cpu.registers.c); }

// 0xc2
inline void set_0_d(void) { set(1 << 0, cpu.registers.d); }

// 0xc3
inline void set_0_e(void) { set(1 << 0, cpu.registers.e); }

// 0xc4
inline void set_0_h(void) { set(1 << 0, cpu.registers.h); }

// 0xc5
inline void set_0_l(void) { set(1 << 0, cpu.registers.l); }

// 0xc6
inline void set_0_hlp(void) { writeByte(cpu.registers.hl, (1 << 0) | readByte(cpu.registers.hl)); }

// 0xc7
inline void set_0_a(void) { set(1 << 0, cpu.registers.a); }

// 0xc8
inline void set_1_b(void) { set(1 << 1, cpu.registers.b); }

// 0xc9
inline void set_1_c(void) { set(1 << 1, cpu.registers.c); }

// 0xca
inline void set_1_d(void) { set(1 << 1, cpu.registers.d); }

// 0xcb
inline void set_1_e(void) { set(1 << 1, cpu.registers.e); }

// 0xcc
inline void set_1_h(void) { set(1 << 1, cpu.registers.h); }

// 0xcd
inline void set_1_l(void) { set(1 << 1, cpu.registers.l); }

// 0xce
inline void set_1_hlp(void) { writeByte(cpu.registers.hl, (1 << 1) | readByte(cpu.registers.hl)); }

// 0xcf
inline void set_1_a(void) { set(1 << 1, cpu.registers.a); }

// 0xd0
inline void set_2_b(void) { set(1 << 2, cpu.registers.b); }

// 0xd1
inline void set_2_c(void) { set(1 << 2, cpu.registers.c); }

// 0xd2
inline void set_2_d(void) { set(1 << 2, cpu.registers.d); }

// 0xd3
inline void set_2_e(void) { set(1 << 2, cpu.registers.e); }

// 0xd4
inline void set_2_h(void) { set(1 << 2, cpu.registers.h); }

// 0xd5
inline void set_2_l(void) { set(1 << 2, cpu.registers.l); }

// 0xd6
inline void set_2_hlp(void) { writeByte(cpu.registers.hl, (1 << 2) | readByte(cpu.registers.hl)); }

// 0xd7
inline void set_2_a(void) { set(1 << 2, cpu.registers.a); }

// 0xd8
inline void set_3_b(void) { set(1 << 3, cpu.registers.b); }

// 0xd9
inline void set_3_c(void) { set(1 << 3, cpu.registers.c); }

// 0xda
inline void set_3_d(void) { set(1 << 3, cpu.registers.d); }

// 0xdb
inline void set_3_e(void) { set(1 << 3, cpu.registers.e); }

// 0xdc
inline void set_3_h(void) { set(1 << 3, cpu.registers.h); }

// 0xdd
inline void set_3_l(void) { set(1 << 3, cpu.registers.l); }

// 0xde
inline void set_3_hlp(void) { writeByte(cpu.registers.hl, (1 << 3) | readByte(cpu.registers.hl)); }

// 0xdf
inline void set_3_a(void) { set(1 << 3, cpu.registers.a); }

// 0xe0
inline void set_4_b(void) { set(1 << 4, cpu.registers.b); }

// 0xe1
inline void set_4_c(void) { set(1 << 4, cpu.registers.c); }

// 0xe2
inline void set_4_d(void) { set(1 << 4, cpu.registers.d); }

// 0xe3
inline void set_4_e(void) { set(1 << 4, cpu.registers.e); }

// 0xe4
inline void set_4_h(void) { set(1 << 4, cpu.registers.h); }

// 0xe5
inline void set_4_l(void) { set(1 << 4, cpu.registers.l); }

// 0xe6
inline void set_4_hlp(void) { writeByte(cpu.registers.hl, (1 << 4) | readByte(cpu.registers.hl)); }

// 0xe7
inline void set_4_a(void) { set(1 << 4, cpu.registers.a); }

// 0xe8
inline void set_5_b(void) { set(1 << 5, cpu.registers.b); }

// 0xe9
inline void set_5_c(void) { set(1 << 5, cpu.registers.c); }

// 0xea
inline void set_5_d(void) { set(1 << 5, cpu.registers.d); }

// 0xeb
inline void set_5_e(void) { set(1 << 5, cpu.registers.e); }

// 0xec
inline void set_5_h(void) { set(1 << 5, cpu.registers.h); }

// 0xed
inline void set_5_l(void) { set(1 << 5, cpu.registers.l); }

// 0xee
inline void set_5_hlp(void) { writeByte(cpu.registers.hl, (1 << 5) | readByte(cpu.registers.hl)); }

// 0xef
inline void set_5_a(void) { set(1 << 5, cpu.registers.a); }

// 0xf0
inline void set_6_b(void) { set(1 << 6, cpu.registers.b); }

// 0xf1
inline void set_6_c(void) { set(1 << 6, cpu.registers.c); }

// 0xf2
inline void set_6_d(void) { set(1 << 6, cpu.registers.d); }

// 0xf3
inline void set_6_e(void) { set(1 << 6, cpu.registers.e); }

// 0xf4
inline void set_6_h(void) { set(1 << 6, cpu.registers.h); }

// 0xf5
inline void set_6_l(void) { set(1 << 6, cpu.registers.l); }

// 0xf6
inline void set_6_hlp(void) { writeByte(cpu.registers.hl, (1 << 6) | readByte(cpu.registers.hl)); }

// 0xf7
inline void set_6_a(void) { set(1 << 6, cpu.registers.a); }

// 0xf8
inline void set_7_b(void) { set(1 << 7, cpu.registers.b); }

// 0xf9
inline void set_7_c(void) { set(1 << 7, cpu.registers.c); }

// 0xfa
inline void set_7_d(void) { set(1 << 7, cpu.registers.d); }

// 0xfb
inline void set_7_e(void) { set(1 << 7, cpu.registers.e); }

// 0xfc
inline void set_7_h(void) { set(1 << 7, cpu.registers.h); }

// 0xfd
inline void set_7_l(void) { set(1 << 7, cpu.registers.l); }

// 0xfe
inline void set_7_hlp(void) { writeByte(cpu.registers.hl, (1 << 7) | readByte(cpu.registers.hl)); }

// 0xff
inline void set_7_a(void) { set(1 << 7, cpu.registers.a); }
