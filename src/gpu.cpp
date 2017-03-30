#include "platform.h"
#include "debug.h"

#include "display.h"
#include "memory.h"
#include "cpu.h"
#include "interrupts.h"

#include "gpu.h"

struct gpu_type gpu;

tilestype* tiles = NULL;

void(*gpuStep)(void) = NULL;

bool invalidFrame = false;

void stepLCDOff(void) {
	if (cpu.memory.LCDC_ctl & 0x80) {
		//  LCD was re-enabled
		gpuStep = stepLCDOn_OAM;
		invalidFrame = true;
		gpu.nextTick = cpu.clocks + 80;
	} else {
		gpu.nextTick = cpu.clocks + 456;

		// good time to refresh the keys
		extern void refresh();
		refresh();
	}
}

static inline void setMode(int mode, int ticksNeeded, void(*newHandler)()) {
	SET_LCDC_MODE(mode);
	gpuStep = newHandler;
	gpu.nextTick += ticksNeeded;
}

static inline void SetLY(unsigned int ly) {
	cpu.memory.LY_lcdline = ly;

	if (ly == cpu.memory.LYC_lcdcompare) {
		cpu.memory.STAT_lcdstatus |= STAT_LYCSIGNAL;
		if (cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) {
			// in some cases it is ignored:
			if ((ly == 0 || ly > 0x90) && (cpu.memory.STAT_lcdstatus & STAT_VBLANKCHECK))
				return;

			cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
		}
	} else {
		cpu.memory.STAT_lcdstatus &= ~STAT_LYCSIGNAL;
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

		// past LYC check ignores hblank check
		if ((cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK) &&
			(!(cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) || cpu.memory.LY_lcdline != cpu.memory.LYC_lcdcompare)) {
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
		SetLY(cpu.memory.LY_lcdline + 1);

		if (cpu.memory.LY_lcdline == 144) {
			if (drawFramebuffer && !invalidFrame) {
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

			// previous lyc check ignores vblank interrupt
			if ((cpu.memory.STAT_lcdstatus & STAT_VBLANKCHECK) &&
				(!(cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK)) &&
			    (!(cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) || 0x8F != cpu.memory.LYC_lcdcompare)) {

				cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
			}

			setMode(GPU_MODE_VBLANK, 456, stepLCDOn_VBLANK);
		} else {
			// lyc check or hblank check disables stat OAM interrupt
			if ((cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) &&
				(!(cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) || cpu.memory.LY_lcdline-1 != cpu.memory.LYC_lcdcompare) &&
				!(cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK)) {

				cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
			}

			setMode(GPU_MODE_OAM, 80, stepLCDOn_OAM);
		}
	}
}

void stepLCDOn_VBLANK(void) {
	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < gpu.nextTick && cpu.timerInterrupt > gpu.nextTick) {
			cpu.clocks += gpu.nextTick - cpu.clocks;
			gpu.nextTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= gpu.nextTick) {
		switch (cpu.memory.LY_lcdline) {
			case 0x00:
				// check if lcd was disabled:
				if (cpu.memory.LCDC_ctl & 0x80) {
					invalidFrame = false;
					setMode(GPU_MODE_OAM, 80, stepLCDOn_OAM);

					// vlank check disables stat OAM interrupt
					if ((cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) && !(cpu.memory.STAT_lcdstatus & STAT_VBLANKCHECK)) {
						cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
					}
				} else {
					//  LCD was disabled, but first draw the blank result
					for (int i = 0; i < 144; i++) {
						cpu.memory.LY_lcdline = i;
						renderBlankScanline();
					}
					cpu.memory.LY_lcdline = 0;
					drawFramebuffer();

					// set the LCD step off (and technically, HBLANK mode, which indicates allowing write to all display memory)
					setMode(0, 456, stepLCDOff);
				}
				break;
			case 0x98:
				SetLY(cpu.memory.LY_lcdline + 1);
				gpu.nextTick += 56;
				break;
			case 0x99:
				SetLY(0);
				gpu.nextTick += 400;
				break;
			default:
				SetLY(cpu.memory.LY_lcdline + 1);
				gpu.nextTick += 456;
				break;
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
