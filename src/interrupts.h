#pragma once

#define INTERRUPTS_VBLANK	(1 << 0)
#define INTERRUPTS_LCDSTAT	(1 << 1)
#define INTERRUPTS_TIMER	(1 << 2)
#define INTERRUPTS_SERIAL	(1 << 3)
#define INTERRUPTS_JOYPAD	(1 << 4)

void interruptStep(void);

// program counter position for each interrupt when triggered
#define INT_VBLANK_PC		0x0040
#define INT_STAT_PC			0x0048
#define INT_TIMER_PC		0x0050
#define INT_SERIAL_PC		0x0058
#define INT_JOYPAD_PC		0x0060

void ret_i(void);
