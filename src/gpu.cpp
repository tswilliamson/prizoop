#include "platform.h"
#include "debug.h"

#include "display.h"
#include "memory.h"
#include "cpu.h"
#include "interrupts.h"

#include "gpu.h"

struct gpu_type gpu;

tilestype* tiles = NULL;

unsigned char backgroundPalette[4];
unsigned char spritePalette[2][4];

void(*gpuStep)(void) = NULL;

bool invalidFrame = false;

void stepLCDOff(void) {
	if (cpu.memory.LCDC_ctl & 0x80) {
		//  LCD was re-enabled, but first 
		gpuStep = stepLCDOn_OAM;
		invalidFrame = true;
		gpu.nextTick = cpu.clocks + 80;
	}
	else {
		gpu.nextTick = cpu.clocks + 456;
	}
}

void stepLCDOff_DrawScreen(void) {
	if (cpu.clocks - gpu.nextTick >= 70224) {
		// draw empty framebuffer and switch to totally off
		for (int i = 0; i < 144; i++) {
			renderBlankScanline();
		}
		drawFramebuffer();
		gpuStep = stepLCDOff;
	} else {
		gpu.nextTick = cpu.clocks + 456;
	}

	if (cpu.memory.LCDC_ctl & 0x80) {
		//  LCD was re-enabled, but first 
		gpuStep = stepLCDOn_OAM;
		invalidFrame = true;
		gpu.nextTick = cpu.clocks + 80;
	}
}

static inline void setMode(int mode, int ticksNeeded, void(*newHandler)()) {
	SET_LCDC_MODE(mode);
	gpuStep = newHandler;
	gpu.nextTick += ticksNeeded;
}

static inline void hblank(void) {
	cpu.memory.LY_lcdline++;

	if (cpu.memory.LY_lcdline == cpu.memory.LYC_lcdcompare) {
		cpu.memory.STAT_lcdstatus |= STAT_LYCSIGNAL;
		if (cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) {
			cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
		}
	}
}

void stepLCDOn_OAM(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < gpu.nextTick && cpu.timerInterrupt > gpu.nextTick) {
			cpu.clocks += gpu.nextTick - cpu.clocks;
			gpu.nextTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= gpu.nextTick) {
		setMode(GPU_MODE_VRAM, 172, stepLCDOn_VRAM);
	}
}

void stepLCDOn_VRAM(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < gpu.nextTick && cpu.timerInterrupt > gpu.nextTick) {
			cpu.clocks += gpu.nextTick - cpu.clocks;
			gpu.nextTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= gpu.nextTick) {
		if (!invalidFrame)
			renderScanline();

		if (cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK) {
			cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
		}

		setMode(GPU_MODE_HBLANK, 204, stepLCDOn_HBLANK);
	}
}

void stepLCDOn_HBLANK(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < gpu.nextTick && cpu.timerInterrupt > gpu.nextTick) {
			cpu.clocks += gpu.nextTick - cpu.clocks;
			gpu.nextTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= gpu.nextTick) {
		hblank();

		if (cpu.memory.LY_lcdline == 144) {
			if (drawFramebuffer) {
				drawFramebuffer();
			}
			cpu.memory.IF_intflag |= INTERRUPTS_VBLANK;

			// joypad interrupt here (though I don't think many games used it)
			if (cpu.halted || cpu.stopped || cpu.memory.IE_intenable & INTERRUPTS_JOYPAD) {
				unsigned char jPad = readByteSpecial(0xFF00);
				if ((jPad & 0x0F) != 0x0F) {
					cpu.memory.IF_intflag |= INTERRUPTS_JOYPAD;
				}
			}

			if (cpu.memory.STAT_lcdstatus & STAT_VBLANKCHECK) {
				cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
			}

			setMode(GPU_MODE_VBLANK, 456, stepLCDOn_VBLANK);
		} else {
			if (cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) {
				cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
			}

			setMode(GPU_MODE_OAM, 80, stepLCDOn_OAM);
		}
	}
}

void stepLCDOn_VBLANK(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < gpu.nextTick && cpu.timerInterrupt > gpu.nextTick) {
			cpu.clocks += gpu.nextTick - cpu.clocks;
			gpu.nextTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= gpu.nextTick) {
		hblank();

		if (cpu.memory.LY_lcdline > 153) {
			// back to line 0, OAM mode
			cpu.memory.LY_lcdline = 0;

			if (0 == cpu.memory.LYC_lcdcompare) {
				cpu.memory.STAT_lcdstatus |= STAT_LYCSIGNAL;
				if (cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) {
					cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
				}
			}

			// check if lcd was disabled:
			if (cpu.memory.LCDC_ctl & 0x80) {
				if (cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) {
					cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
				}
				invalidFrame = false;
			}
			else {
				gpuStep = stepLCDOff_DrawScreen;
			}

			setMode(GPU_MODE_OAM, 80, stepLCDOn_OAM);
		} else {
			gpu.nextTick += 456;
		}
	}
}

void updateTile(unsigned short address, unsigned char value) {
	TIME_SCOPE();

	address &= 0x1ffe;
	
	unsigned short tile = (address >> 4) & 511;
	unsigned short y = (address >> 1) & 7;
	
	unsigned char x, bitIndex;
	for(x = 0; x < 8; x++) {
		bitIndex = 1 << (7 - x);
		
		//((unsigned char (*)[8][8])tiles)[tile][y][x] = ((vram[address] & bitIndex) ? 1 : 0) + ((vram[address + 1] & bitIndex) ? 2 : 0);
		tiles->data[tile][y][x] = ((vram[address] & bitIndex) ? 1 : 0) + ((vram[address + 1] & bitIndex) ? 2 : 0);
	}
}
