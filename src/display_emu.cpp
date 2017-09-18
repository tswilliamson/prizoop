
// simple display functionality for emulator / winsim
#if !TARGET_PRIZM

#include "debug.h"
#include "emulator.h"

#include "gpu.h"
#include "memory.h"
#include "keys.h"

unsigned int framecounter = 0;

bool skippingFrame = false;
int frameSkip = 0;

int lineBuffer[176] ALIGN(32) = { 0 };
int prevLineBuffer[168] ALIGN(32) = { 0 };

#include "tilerow.inl"
#include "dmg_scanline.inl"
#include "cgb_scanline.inl"
#include "scanline_resolve.inl"

static void resolveLine() {
	static unsigned short* const vram = (unsigned short*)GetVRAMAddress();
	
	unsigned int* scanline;
	switch (emulator.settings.scaleMode) {
		case emu_scale::NONE:
			scanline = (unsigned int*) (vram + 112 + LCD_WIDTH_PX * (cpu.memory.LY_lcdline + 36));
			DirectScanline16(scanline);
			break;
		case emu_scale::LO_150:
			scanline = (unsigned int*)(vram + 72 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2));

			if (cpu.memory.LY_lcdline & 1) {
				DirectScanline24(scanline);
				DirectScanline24(scanline + LCD_WIDTH_PX / 2);
			} else {
				DirectScanline24(scanline);
			}
			break;
		case emu_scale::HI_150:
			scanline = (unsigned int*) (vram + 72 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2));

			if (cpu.memory.LY_lcdline & 1) {
				BlendMixedScanline24(scanline);
				BlendScanline24(scanline + LCD_WIDTH_PX / 2);
			} else {
				BlendScanline24(scanline);
				memcpy(prevLineBuffer, lineBuffer, sizeof(prevLineBuffer));
			}
			break;
		case emu_scale::LO_200:
			scanline = (unsigned int*)(vram + 32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2));

			if (cpu.memory.LY_lcdline & 1) {
				DirectDoubleScanline32(scanline, scanline + LCD_WIDTH_PX / 2);
			} else {
				DirectScanline32(scanline);
			}
			break;
		case emu_scale::HI_200:
			scanline = (unsigned int*)(vram + 32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2));

			if (cpu.memory.LY_lcdline & 1) {
				BlendMixedScanline32(scanline);
				DirectScanline32(scanline + LCD_WIDTH_PX / 2);
			} else {
				DirectScanline32(scanline);
				memcpy(prevLineBuffer, lineBuffer, sizeof(prevLineBuffer));
			}
			break;
	}
}

void renderEmu() {
	if (skippingFrame)
		return;

	TIME_SCOPE();

	if (cgb.isCGB) {
		RenderCGBScanline();

		// resolve to colors
		if (cgb.dirtyPalette) {
			cgbResolvePalette();
		}
	} else {
		RenderDMGScanline();
	}

	resolveLine();
}

void renderBlankEmu() {
	TIME_SCOPE();

	memset(lineBuffer, 0, sizeof(lineBuffer));
	resolveLine();
}

void drawEmu() {

	TIME_SCOPE();

	// frame counter
	framecounter++;

	// determine frame rate based on last framebuffer call:
	static int fps = 0;
	int curfps = fps;

	static int lastticks = 0;
	if (framecounter % 32 == 0) {
		int ticks = RTC_GetTicks();
		int tickdiff = ticks - lastticks;
		if (tickdiff) {
			curfps = 40960 / tickdiff;
			lastticks = ticks;
		}
	}

	// TODO : not the best... only clamps speed to 64 FPS (7% too high)
	if (emulator.settings.clampSpeed) {
		static int lastClampTicks = 0;
		int curTicks = RTC_GetTicks();
		while (curTicks == lastClampTicks || curTicks == lastClampTicks + 1) { curTicks = RTC_GetTicks(); }
		lastClampTicks = curTicks;
	}

	if (curfps != fps) {
		fps = curfps;
#if TARGET_PRIZMEMU
		// report frame rate:
		memset(ScopeTimer::debugString, 0, sizeof(ScopeTimer::debugString));
		sprintf(ScopeTimer::debugString, "FPS:%d.%d, Skip:%d", fps / 10, fps % 10, frameSkip);
#endif
	}

	// determine frame skip
	skippingFrame = false;
	if (frameSkip && framecounter > 4) {
		if (frameSkip < 0) {
			// negative is automatic (frame skip amt stored in HOW negative it is, though Windows is pretty much always full speed)
			skippingFrame = (framecounter % (-frameSkip)) != 0;
		}
		else {
			skippingFrame = (framecounter % (frameSkip + 1)) != 0;
		}
	}

	if (skippingFrame) {
		refreshKeys(false);
		return;
	}

	// draw frame buffer here
	Bdisp_PutDisp_DD();

	// good time to refresh keys and check for os requests and such
	refreshKeys(true);
}


void(*renderScanline)(void) = renderEmu;
void(*renderBlankScanline)(void) = renderBlankEmu;
void(*drawFramebuffer)(void) = drawEmu;

void SetupDisplayDriver(char withFrameskip) {
	frameSkip = withFrameskip;

	drawFramebuffer = drawEmu;
	renderScanline = renderEmu;
	renderBlankScanline = renderBlankEmu;
}

#endif