#pragma once

#define INTERRUPTS_VBLANK	(1 << 0)
#define INTERRUPTS_LCDSTAT	(1 << 1)
#define INTERRUPTS_TIMER	(1 << 2)
#define INTERRUPTS_SERIAL	(1 << 3)
#define INTERRUPTS_JOYPAD	(1 << 4)

// avoid calling interrupt step based on this check:
inline bool interruptCheck() {
	// same as below except uses integer math:
	//	return (cpu.clocks >= cpu.timerInterrupt) || (cpu.memory.IE_intenable & cpu.memory.IF_intflag);
#ifdef LITTLE_E
	return (cpu.clocks >= cpu.timerInterrupt) || (cpu.memory.longs[0x03] & cpu.memory.longs[0x3f] & 0xFF000000);
#else
	return (cpu.clocks >= cpu.timerInterrupt) || (cpu.memory.longs[0x03] & cpu.memory.longs[0x3f] & 0xFF);
#endif
}

void interruptStep(void);

// program counter position for each interrupt when triggered
#define INT_VBLANK_PC		0x0040
#define INT_STAT_PC			0x0048
#define INT_TIMER_PC		0x0050
#define INT_SERIAL_PC		0x0058
#define INT_JOYPAD_PC		0x0060

void ret_i(void);
