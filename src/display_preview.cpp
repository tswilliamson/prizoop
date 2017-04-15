
// displays a frame to the a pause preview window (which is just the palette entries 0-11 at half rez) and then returns to the menu on drawFrame 

#include "debug.h"
#include "emulator.h"

#include "gpu.h"
#include "memory.h"
#include "keys.h"

static unsigned short colorPalette[12] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

static int lineBuffer[176];

#include "gpu_scanline.inl"

static void renderPreviewLine() {
	if ((cpu.memory.LY_lcdline & 1) == 0) {
		unsigned short line[160];
		RenderScanline();
		ResolveScanline<unsigned short>(line);

		unsigned char* previewLine = &emulator.pausePreview[80 * cpu.memory.LY_lcdline / 4];
		// every other pixel goes into the preview line, packed at 4 bpp
		for (int i = 0; i < 160; i += 4) {
			*(previewLine++) = ((line[i] & 0xF) << 4) | (line[i + 2] & 0xF);
		}
	}
}

static void renderPreviewBlankLine() {
	if ((cpu.memory.LY_lcdline & 1) == 0) {
		unsigned char* previewLine = &emulator.pausePreview[80 * cpu.memory.LY_lcdline / 4];
		memset(previewLine, 0, 40);
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