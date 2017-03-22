#include "platform.h"
#include "debug.h"

#include "cpu.h"
#include "memory.h"
#include "display.h"
#include "keys.h"
#include "main.h"

#include "interrupts.h"

inline void issueInterrupt(unsigned char flagMask, unsigned short toPC) {
	// exit a halt no matter what
	if (cpu.halted) {
		cpu.halted = 0;
		cpu.clocks += 4;
	}

	if (cpu.IME) {
		// clear interrupt flag
		cpu.memory.IF_intflag &= flagMask;

		// virtual di instruction
		cpu.IME = 0;
		writeShortToStack(cpu.registers.pc);
		cpu.registers.pc = toPC;

		// whole process takes 20 clocks
		cpu.clocks += 20;
	}
}

void interruptStep(void) {
	TIME_SCOPE();

	// update timer in case it sends an interrupt
	if (cpu.clocks >= cpu.timerInterrupt) {
		updateTimer();
	}

	unsigned char fire = cpu.memory.IE_intenable & cpu.memory.IF_intflag;
	if ((cpu.IME || cpu.halted) && fire) {

		// these are else if'd by interrupt priority
		if (fire & INTERRUPTS_VBLANK) {
			issueInterrupt(~INTERRUPTS_VBLANK, INT_VBLANK_PC);
		}
		else if (fire & INTERRUPTS_LCDSTAT) {
			issueInterrupt(~INTERRUPTS_LCDSTAT, INT_STAT_PC);
		}
		else if (fire & INTERRUPTS_TIMER) {
			issueInterrupt(~INTERRUPTS_TIMER, INT_TIMER_PC);
		}
		else if (fire & INTERRUPTS_SERIAL) {
			issueInterrupt(~INTERRUPTS_SERIAL, INT_SERIAL_PC);
		}
		else if (fire & INTERRUPTS_JOYPAD) {
			issueInterrupt(~INTERRUPTS_JOYPAD, INT_JOYPAD_PC);
		}
	}
}

