
// simple display functionality for emulator / winsim
#if !TARGET_PRIZM

#include "debug.h"

#include "gpu.h"
#include "memory.h"
#include "keys.h"

bool skippingFrame = false;
bool stretch = false;
int framecounter = 0;
int frameSkip = 0;

unsigned short colorPaletteBG[4] = {
	COLOR_WHITE,
	COLOR_LIGHTCYAN,
	COLOR_CYAN,
	COLOR_DARKCYAN,
};

unsigned short colorPaletteSprite[4] = {
	COLOR_WHITE,
	COLOR_LIGHTCYAN,
	COLOR_CYAN,
	COLOR_DARKCYAN,
};

#include "gpu_scanline.inl"

void renderEmu() {
	if (skippingFrame)
		return;

	TIME_SCOPE();

	// stretch across screen?
	if (stretch) {
		unsigned short* scanlineStart = &((unsigned short*)GetVRAMAddress())[32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2)];
		RenderScanline<unsigned int>(scanlineStart);

		if (cpu.memory.LY_lcdline & 1) {
			RenderScanline<unsigned int>(scanlineStart + LCD_WIDTH_PX);
		}
	}
	else {
		void* scanlineStart = &((unsigned short*)GetVRAMAddress())[112 + LCD_WIDTH_PX * cpu.memory.LY_lcdline];
		RenderScanline<unsigned short>(scanlineStart);
	}
}

void renderBlankEmu() {
	TIME_SCOPE();

	// stretch across screen?
	if (stretch) {
		unsigned short* scanlineStart = &((unsigned short*)GetVRAMAddress())[32 + LCD_WIDTH_PX * ((cpu.memory.LY_lcdline * 3) / 2)];
		memset(scanlineStart, 0xFF, 160 * 4);

		if (cpu.memory.LY_lcdline & 1) {
			memset(scanlineStart + LCD_WIDTH_PX, 0xFF, 160 * 4);
		}
	}
	else {
		void* scanlineStart = &((unsigned short*)GetVRAMAddress())[112 + LCD_WIDTH_PX * cpu.memory.LY_lcdline];
		memset(scanlineStart, 0xFF, 160 * 2);
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
			// negative is automatic (frame skip amt stored in HOW negative it is)
			if (fps > 597 && frameSkip != -1) {
				frameSkip++;
			}
			else if (fps && fps < 500 && frameSkip != -4) {
				frameSkip--;
			}
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

#if TARGET_WINSIM
#include "prizmsim.h"
void refresh() {
	keys.k1.a = GetKey_SimFast(KEY_CTRL_SHIFT) ? 0 : 1;	// SHIFT
	keys.k1.b = GetKey_SimFast(KEY_CTRL_ALPHA) ? 0 : 1;	// ALPHA
	keys.k1.select = GetKey_SimFast(KEY_CTRL_OPTN) ? 0 : 1; // OPTN
	keys.k1.start = GetKey_SimFast(KEY_CTRL_VARS) ? 0 : 1; // VARS
	keys.k2.right = GetKey_SimFast(KEY_CTRL_RIGHT) ? 0 : 1;
	keys.k2.left = GetKey_SimFast(KEY_CTRL_LEFT) ? 0 : 1;
	keys.k2.up = GetKey_SimFast(KEY_CTRL_UP) ? 0 : 1;
	keys.k2.down = GetKey_SimFast(KEY_CTRL_DOWN) ? 0 : 1;
}
#endif

void(*renderScanline)(void) = renderEmu;
void(*renderBlankScanline)(void) = renderBlankEmu;
void(*drawFramebuffer)(void) = drawEmu;

void SetupDisplayDriver(bool withStretch, char withFrameskip) {
	frameSkip = withFrameskip;
	stretch = withStretch;
}

void SetupDisplayColors(unsigned short bg0, unsigned short bg1, unsigned short bg2, unsigned short bg3, unsigned short sp0, unsigned short sp1, unsigned short sp2, unsigned short sp3) {
	colorPaletteBG[0] = bg0;
	colorPaletteBG[1] = bg1;
	colorPaletteBG[2] = bg2;
	colorPaletteBG[3] = bg3;
	colorPaletteSprite[0] = sp0;
	colorPaletteSprite[1] = sp1;
	colorPaletteSprite[2] = sp2;
	colorPaletteSprite[3] = sp3;
}

#endif