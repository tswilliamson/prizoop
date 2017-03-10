#pragma once

#define FLAGS_ZERO (1 << 7)
#define FLAGS_ZERO_BIT 7
#define FLAGS_NEGATIVE (1 << 6)
#define FLAGS_NEGATIVE_BIT 6
#define FLAGS_HALFCARRY (1 << 5)
#define FLAGS_HALFCARRY_BIT 5
#define FLAGS_CARRY (1 << 4)
#define FLAGS_CARRY_BIT 4

#define FLAGS_ISZERO (registers.f & FLAGS_ZERO)
#define FLAGS_ISNEGATIVE (registers.f & FLAGS_NEGATIVE)
#define FLAGS_ISCARRY (registers.f & FLAGS_CARRY)
#define FLAGS_ISHALFCARRY (registers.f & FLAGS_HALFCARRY)

#define FLAGS_ISSET(x) (registers.f & (x))
#define FLAGS_SET(x) (registers.f |= (x))
#define FLAGS_CLEAR(x) (registers.f &= ~(x))
#define FLAGS_COND(bit, cond) (registers.f = (registers.f & ~(1 << bit)) | (cond << bit))

struct cpuInfo {
	unsigned char halted;
	unsigned long ticks;
	unsigned char stopped;
};

extern cpuInfo cpu;

void reset(void);
void cpuStep(void);