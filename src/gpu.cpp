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

int windowLineOffset = 0;

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
				windowLineOffset = 0;

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

unsigned short MortonTable[256] = {
	0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
	0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
	0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
	0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
	0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
	0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
	0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
	0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
	0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
	0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
	0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
	0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
	0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
	0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
	0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
	0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555,
};

unsigned short ReverseMortonTable[256] = {
	0x0000, 0x4000, 0x1000, 0x5000, 0x0400, 0x4400, 0x1400, 0x5400, 0x0100, 0x4100, 0x1100, 0x5100, 0x0500, 0x4500, 0x1500, 0x5500,
	0x0040, 0x4040, 0x1040, 0x5040, 0x0440, 0x4440, 0x1440, 0x5440, 0x0140, 0x4140, 0x1140, 0x5140, 0x0540, 0x4540, 0x1540, 0x5540,
	0x0010, 0x4010, 0x1010, 0x5010, 0x0410, 0x4410, 0x1410, 0x5410, 0x0110, 0x4110, 0x1110, 0x5110, 0x0510, 0x4510, 0x1510, 0x5510,
	0x0050, 0x4050, 0x1050, 0x5050, 0x0450, 0x4450, 0x1450, 0x5450, 0x0150, 0x4150, 0x1150, 0x5150, 0x0550, 0x4550, 0x1550, 0x5550,
	0x0004, 0x4004, 0x1004, 0x5004, 0x0404, 0x4404, 0x1404, 0x5404, 0x0104, 0x4104, 0x1104, 0x5104, 0x0504, 0x4504, 0x1504, 0x5504,
	0x0044, 0x4044, 0x1044, 0x5044, 0x0444, 0x4444, 0x1444, 0x5444, 0x0144, 0x4144, 0x1144, 0x5144, 0x0544, 0x4544, 0x1544, 0x5544,
	0x0014, 0x4014, 0x1014, 0x5014, 0x0414, 0x4414, 0x1414, 0x5414, 0x0114, 0x4114, 0x1114, 0x5114, 0x0514, 0x4514, 0x1514, 0x5514,
	0x0054, 0x4054, 0x1054, 0x5054, 0x0454, 0x4454, 0x1454, 0x5454, 0x0154, 0x4154, 0x1154, 0x5154, 0x0554, 0x4554, 0x1554, 0x5554,
	0x0001, 0x4001, 0x1001, 0x5001, 0x0401, 0x4401, 0x1401, 0x5401, 0x0101, 0x4101, 0x1101, 0x5101, 0x0501, 0x4501, 0x1501, 0x5501,
	0x0041, 0x4041, 0x1041, 0x5041, 0x0441, 0x4441, 0x1441, 0x5441, 0x0141, 0x4141, 0x1141, 0x5141, 0x0541, 0x4541, 0x1541, 0x5541,
	0x0011, 0x4011, 0x1011, 0x5011, 0x0411, 0x4411, 0x1411, 0x5411, 0x0111, 0x4111, 0x1111, 0x5111, 0x0511, 0x4511, 0x1511, 0x5511,
	0x0051, 0x4051, 0x1051, 0x5051, 0x0451, 0x4451, 0x1451, 0x5451, 0x0151, 0x4151, 0x1151, 0x5151, 0x0551, 0x4551, 0x1551, 0x5551,
	0x0005, 0x4005, 0x1005, 0x5005, 0x0405, 0x4405, 0x1405, 0x5405, 0x0105, 0x4105, 0x1105, 0x5105, 0x0505, 0x4505, 0x1505, 0x5505,
	0x0045, 0x4045, 0x1045, 0x5045, 0x0445, 0x4445, 0x1445, 0x5445, 0x0145, 0x4145, 0x1145, 0x5145, 0x0545, 0x4545, 0x1545, 0x5545,
	0x0015, 0x4015, 0x1015, 0x5015, 0x0415, 0x4415, 0x1415, 0x5415, 0x0115, 0x4115, 0x1115, 0x5115, 0x0515, 0x4515, 0x1515, 0x5515,
	0x0055, 0x4055, 0x1055, 0x5055, 0x0455, 0x4455, 0x1455, 0x5455, 0x0155, 0x4155, 0x1155, 0x5155, 0x0555, 0x4555, 0x1555, 0x5555,
};