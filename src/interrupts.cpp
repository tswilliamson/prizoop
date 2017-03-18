#include "platform.h"
#include "debug.h"

#include "cpu.h"
#include "memory.h"
#include "display.h"
#include "keys.h"
#include "main.h"

#include "interrupts.h"

struct interrupt interrupt;

void interruptStep(void) {
	if(interrupt.master && interrupt.enable && interrupt.flags) {
		unsigned char fire = interrupt.enable & interrupt.flags;

		if (fire) {
			cpu.halted = 0;

			if (fire & INTERRUPTS_VBLANK) {
				interrupt.flags &= ~INTERRUPTS_VBLANK;
				vblank();
			}

			if (fire & INTERRUPTS_LCDSTAT) {
				interrupt.flags &= ~INTERRUPTS_LCDSTAT;
				lcdStat();
			}

			if (fire & INTERRUPTS_TIMER) {
				interrupt.flags &= ~INTERRUPTS_TIMER;
				timer();
			}

			if (fire & INTERRUPTS_SERIAL) {
				interrupt.flags &= ~INTERRUPTS_SERIAL;
				serial();
			}

			if (fire & INTERRUPTS_JOYPAD) {
				interrupt.flags &= ~INTERRUPTS_JOYPAD;
				joypad();
			}
		}
	}
}

void vblank(void) {
	interrupt.master = 0;
	writeShortToStack(cpu.registers.pc);
	cpu.registers.pc = 0x40;
	
	cpu.clocks += 20;
}

void lcdStat(void) {
	interrupt.master = 0;
	writeShortToStack(cpu.registers.pc);
	cpu.registers.pc = 0x48;
	
	cpu.clocks += 20;
}

void timer(void) {
	interrupt.master = 0;
	writeShortToStack(cpu.registers.pc);
	cpu.registers.pc = 0x50;
	
	cpu.clocks += 20;
}

void serial(void) {
	interrupt.master = 0;
	writeShortToStack(cpu.registers.pc);
	cpu.registers.pc = 0x58;
	
	cpu.clocks += 20;
}

void joypad(void) {
	interrupt.master = 0;
	writeShortToStack(cpu.registers.pc);
	cpu.registers.pc = 0x60;
	
	cpu.clocks += 20;
}

void ret_i(void) {
	interrupt.master = 1;
	cpu.registers.pc = readShortFromStack();
}
