
#include "platform.h"
#include "cpu.h"
#include "interrupts.h"

// based on timer div freq
int bitsToShift[4] = {
	10,
	4,
	6,
	8
};

void updateDiv() {
	cpu.div += (cpu.clocks - cpu.divBase);
	cpu.divBase = cpu.clocks;
}

void updateTimer() {
	if (cpu.memory.TAC_timerctl & 0x04) {
		const int bits = bitsToShift[cpu.memory.TAC_timerctl & 3];
		int curTimer = (cpu.timer >> bits) & 0xFFFF;
		cpu.timer = cpu.timer + (cpu.clocks - cpu.timerBase);
		int newTimer = (cpu.timer >> bits) & 0xFFFF;
		if ((newTimer & 0xFF00) != (curTimer & 0xFF00)) {
			// overflow occured
			cpu.memory.IF_intflag |= INTERRUPTS_TIMER;
			newTimer = cpu.memory.TMA_timermodulo;
		}
		newTimer = newTimer & 0xFF;
		cpu.memory.TIMA_timerctr = newTimer;

		// determine next interrupt hit for optimizations
		cpu.timerInterrupt = cpu.clocks + ((0xff - newTimer + 1) << bits) - 4;
	}
	else {
		cpu.timerInterrupt = 0xFFFFFFFF;
	}
	cpu.timerBase = cpu.clocks;
}

void writeTIMA(unsigned char value) {
	const int bits = bitsToShift[cpu.memory.TAC_timerctl & 3];
	cpu.memory.TIMA_timerctr = value;
	cpu.timer = (value << bits) | ((cpu.timer & 0xFFFFFFFF) >> (32 - bits));
	cpu.timerBase = cpu.clocks;
	cpu.memory.TIMA_timerctr = value;
	updateTimer();
}

void writeTAC(unsigned char value) {
	cpu.memory.TAC_timerctl = value;
	writeTIMA(cpu.memory.TIMA_timerctr);
}