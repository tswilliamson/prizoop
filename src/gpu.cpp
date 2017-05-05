#include "platform.h"
#include "debug.h"

#include "display.h"
#include "memory.h"
#include "cpu.h"
#include "cgb.h"
#include "interrupts.h"
#include "emulator.h"

#include "gpu.h"
#include "snd.h"
#include "keys.h"

void(*gpuStep)(void) = NULL;

unsigned int ppuPalette[64] = { 0 };

unsigned int gpuTimes[5] = {
	204,		// HBLANK
	456,		// VBLANK
	80,			// OAM
	172,		// VRAM
	56,			// VBLANK_SPECIAL
};

bool invalidFrame = false;

void stepLCDOff(void) {
	if (cpu.memory.LCDC_ctl & 0x80) {
		//  LCD was re-enabled
		gpuStep = stepLCDOn_OAM;
		invalidFrame = true;
		cpu.gpuTick = cpu.clocks + gpuTimes[GPU_MODE_OAM];
	} else {
		cpu.gpuTick = cpu.clocks + gpuTimes[GPU_MODE_VBLANK];

		// good time for sound update
		condSoundUpdate();

		// good time to refresh the keys
		refreshKeys(true);

		// run inactive sound logic if sound disabled
		if (!emulator.settings.sound) {
			sndInactiveFrame();
		}
	}
}

static inline void setMode(int mode, int ticksNeeded, void(*newHandler)()) {
	SET_LCDC_MODE(mode);
	gpuStep = newHandler;
	cpu.gpuTick += ticksNeeded;
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

	// update sound 150x per frame
	condSoundUpdate();
}

void stepLCDOn_OAM(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < cpu.gpuTick && cpu.timerInterrupt > cpu.gpuTick) {
			cpu.clocks += cpu.gpuTick - cpu.clocks;
			cpu.gpuTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= cpu.gpuTick) {
		setMode(GPU_MODE_VRAM, gpuTimes[GPU_MODE_VRAM], stepLCDOn_VRAM);
	}
}

void stepLCDOn_VRAM(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < cpu.gpuTick && cpu.timerInterrupt > cpu.gpuTick) {
			cpu.clocks += cpu.gpuTick - cpu.clocks;
			cpu.gpuTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= cpu.gpuTick) {
		if (!invalidFrame)
			renderScanline();

		// past LYC check ignores hblank check
		if ((cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK) &&
			(!(cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) || cpu.memory.LY_lcdline != cpu.memory.LYC_lcdcompare)) {
			cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
		}

		if (cgb.isCGB && cgb.hblankDmaActive) {
			cgbHBlankDMA();
		}

		setMode(GPU_MODE_HBLANK, gpuTimes[GPU_MODE_HBLANK], stepLCDOn_HBLANK);
	}
}

void stepLCDOn_HBLANK(void) {
	TIME_SCOPE();

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < cpu.gpuTick && cpu.timerInterrupt > cpu.gpuTick) {
			cpu.clocks += cpu.gpuTick - cpu.clocks;
			cpu.gpuTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= cpu.gpuTick) {
		SetLY(cpu.memory.LY_lcdline + 1);

		if (cpu.memory.LY_lcdline == 144) {
			if (drawFramebuffer && !invalidFrame) {
				drawFramebuffer();
			}
			cpu.memory.IF_intflag |= INTERRUPTS_VBLANK;

			// joypad interrupt here (though I don't think many games used it)
			if (!cgb.isCGB && (cpu.halted || cpu.stopped || cpu.memory.IE_intenable & INTERRUPTS_JOYPAD)) {
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

			setMode(GPU_MODE_VBLANK, gpuTimes[GPU_MODE_VBLANK], stepLCDOn_VBLANK);
		} else {
			// lyc check or hblank check disables stat OAM interrupt
			if ((cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) &&
				(!(cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) || cpu.memory.LY_lcdline-1 != cpu.memory.LYC_lcdcompare) &&
				!(cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK)) {

				cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
			}

			setMode(GPU_MODE_OAM, gpuTimes[GPU_MODE_OAM], stepLCDOn_OAM);
		}
	}
}

void stepLCDOn_VBLANK(void) {
	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME) {
		// don't screw up the timer or overcompensate
		if (cpu.clocks < cpu.gpuTick && cpu.timerInterrupt > cpu.gpuTick) {
			cpu.clocks += cpu.gpuTick - cpu.clocks;
			cpu.gpuTick = cpu.clocks;
		}
	}

	if (cpu.clocks >= cpu.gpuTick) {
		switch (cpu.memory.LY_lcdline) {
			case 0x00:
				// run inactive sound logic if sound disabled
				if (!emulator.settings.sound) {
					sndInactiveFrame();
				}

				// check for dirty rtc value once per frame
				if (mbcIsRTC()) {
					rtcCheckDirty();
				}

				// check if lcd was disabled:
				if (cpu.memory.LCDC_ctl & 0x80) {
					invalidFrame = false;
					setMode(GPU_MODE_OAM, gpuTimes[GPU_MODE_OAM], stepLCDOn_OAM);

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

					// send one hblank dma off if there is one
					if (cgb.isCGB && cgb.hblankDmaActive) {
						cgbHBlankDMA();
					}

					// set the LCD step off (and technically, HBLANK mode, which indicates allowing write to all display memory)
					setMode(0, gpuTimes[GPU_MODE_VBLANK], stepLCDOff);
				}
				break;
			case 0x98:
				SetLY(cpu.memory.LY_lcdline + 1);
				cpu.gpuTick += gpuTimes[4];
				break;
			case 0x99:
				SetLY(0);
				cpu.gpuTick += gpuTimes[GPU_MODE_VBLANK] - gpuTimes[4];
				break;
			default:
				SetLY(cpu.memory.LY_lcdline + 1);
				cpu.gpuTick += gpuTimes[GPU_MODE_VBLANK];
				break;
		}
	}
}

void SetupDisplayPalette() {
	resolveDMGBGPalette();
	resolveDMGOBJ0Palette();
	resolveDMGOBJ1Palette();
}