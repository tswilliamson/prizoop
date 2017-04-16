
// simple display functionality for emulator / winsim
#if !TARGET_PRIZM

#include "debug.h"
#include "emulator.h"

#include "gpu.h"
#include "memory.h"
#include "keys.h"

bool skippingFrame = false;
bool stretch = false;
int framecounter = 0;
int frameSkip = 0;

static int lineBuffer[176] ALIGN(32) = { 0 };

#include "dmg_scanline.inl"
#include "cgb_scanline.inl"

void renderEmu() {
	if (skippingFrame)
		return;

	TIME_SCOPE();

	if (cgb.isCGB) {
		RenderCGBScanline();

		// stretch across screen?
		if (stretch) {
			unsigned short* scanlineStart = &((unsigned short*)GetVRAMAddress())[32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2)];

			if (cpu.memory.LY_lcdline & 1) {
				DoubleResolveCGBScanline<unsigned int>(scanlineStart, scanlineStart + LCD_WIDTH_PX);
			} else {
				ResolveCGBScanline<unsigned int>(scanlineStart);
			}
		} else {
			void* scanlineStart = &((unsigned short*)GetVRAMAddress())[112 + LCD_WIDTH_PX * (cpu.memory.LY_lcdline + 36)];
			ResolveCGBScanline<unsigned short>(scanlineStart);
		}
	} else {
		RenderDMGScanline();

		// stretch across screen?
		if (stretch) {
			unsigned short* scanlineStart = &((unsigned short*)GetVRAMAddress())[32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2)];

			if (cpu.memory.LY_lcdline & 1) {
				DoubleResolveDMGScanline<unsigned int>(scanlineStart, scanlineStart + LCD_WIDTH_PX);
			} else {
				ResolveDMGScanline<unsigned int>(scanlineStart);
			}
		} else {
			void* scanlineStart = &((unsigned short*)GetVRAMAddress())[112 + LCD_WIDTH_PX * (cpu.memory.LY_lcdline + 36)];
			ResolveDMGScanline<unsigned short>(scanlineStart);
		}
	}
}

void renderBlankEmu() {
	TIME_SCOPE();

	// stretch across screen?
	if (stretch) {
		unsigned short* scanlineStart = &((unsigned short*)GetVRAMAddress())[32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2)];

		unsigned int* curScan = (unsigned int*)scanlineStart;
		for (int i = 0; i < 160; i++) {
			*(curScan++) = dmgPalette[0] | (dmgPalette[0] << 16);
		}

		if (cpu.memory.LY_lcdline & 1) {
			curScan = (unsigned int*) (scanlineStart + LCD_WIDTH_PX);
			for (int i = 0; i < 160; i++) {
				*(curScan++) = dmgPalette[0] | (dmgPalette[0] << 16);
			}
		}
	} else {
		void* scanlineStart = &((unsigned short*)GetVRAMAddress())[112 + LCD_WIDTH_PX * (cpu.memory.LY_lcdline + 36)];

		unsigned short* curScan = (unsigned short*)scanlineStart;
		for (int i = 0; i < 160; i++) {
			*(curScan++) = dmgPalette[0];
		}
	}
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
		return;
	}

	// draw frame buffer here
	Bdisp_PutDisp_DD();

	// good time to refresh keys and check for os requests and such
	extern void refresh();
	refresh();
}


void(*renderScanline)(void) = renderEmu;
void(*renderBlankScanline)(void) = renderBlankEmu;
void(*drawFramebuffer)(void) = drawEmu;

void SetupDisplayDriver(bool withStretch, char withFrameskip) {
	frameSkip = withFrameskip;
	stretch = withStretch;

	drawFramebuffer = drawEmu;
	renderScanline = renderEmu;
	renderBlankScanline = renderBlankEmu;
}

#endif