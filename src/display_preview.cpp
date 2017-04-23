
// displays a frame to the a pause preview window (which is just the palette entries 0-11 at half rez) and then returns to the menu on drawFrame 

#include "debug.h"
#include "emulator.h"

#include "gpu.h"
#include "memory.h"
#include "keys.h"

#include "tilerow.inl"
#include "dmg_scanline.inl"
#include "cgb_scanline.inl"

static void renderPreviewLine() {
	if ((cpu.memory.LY_lcdline & 1) == 0) {
		if (cgb.isCGB) {
			RenderCGBScanline();

			unsigned char* previewLine = &emulator.pausePreview[80 * cpu.memory.LY_lcdline / 2];
			// every other pixel goes into the preview line, packed at 8 bpp
			for (int i = 7; i < 167; i += 2) {
				*(previewLine++) = lineBuffer[i];
			}
		} else {
			RenderDMGScanline();

			unsigned char* previewLine = &emulator.pausePreview[80 * cpu.memory.LY_lcdline / 2];
			// every other pixel goes into the preview line, packed at 4 bpp
			for (int i = 7; i < 167; i += 2) {
				*(previewLine++) = (lineBuffer[i+1] << 4) | lineBuffer[i];
			}
		}

	}
}

static void renderPreviewBlankLine() {
	if ((cpu.memory.LY_lcdline & 1) == 0) {
		unsigned char* previewLine = &emulator.pausePreview[80 * cpu.memory.LY_lcdline / 2];
		memset(previewLine, 0, 80);
	}
}

static void renderPreviewScreen() {
	// set the preview to valid and exit
	emulator.pausePreviewValid = true;
	keys.exit = true;
}

void enablePausePreview() {
	if (renderScanline != renderPreviewLine) {
		renderScanline = renderPreviewLine;
		renderBlankScanline = renderPreviewBlankLine;
		drawFramebuffer = renderPreviewScreen;
	}
}