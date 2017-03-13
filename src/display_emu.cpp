
// simple display functionality for emulator / winsim
#if !TARGET_PRIZM

#include "gpu.h"
#include "memory.h"
#include "keys.h"

bool skippingFrame = false;
int framecounter = 0;
int frameSkip = 0;

unsigned short colorPalette[4] = {
	COLOR_WHITE,
	COLOR_LIGHTCYAN,
	COLOR_CYAN,
	COLOR_DARKCYAN,
};

void renderEmu() {

	if (skippingFrame)
		return;

	TIME_SCOPE();

	int mapOffset = (gpu.control & GPU_CONTROL_TILEMAP) ? 0x1c00 : 0x1800;
	mapOffset += (((gpu.scanline + gpu.scrollY) & 255) >> 3) << 5;

	void* scanlineStart = &((unsigned short*)GetVRAMAddress())[LCD_WIDTH_PX * gpu.scanline];


	// if bg enabled
	{
		int i;
		int x = gpu.scrollX & 7;
		int y = (gpu.scanline + gpu.scrollY) & 7;

		// finish/draw left tile
		int lineOffset = (gpu.scrollX >> 3);
		unsigned short tile = (unsigned short)vram[mapOffset + lineOffset];
		const unsigned char* tileRow = tiles->data[tile][y];

		unsigned short* scanline = (unsigned short*)scanlineStart;

		const unsigned short palette[4] = {
			colorPalette[backgroundPalette[0]],
			colorPalette[backgroundPalette[1]],
			colorPalette[backgroundPalette[2]],
			colorPalette[backgroundPalette[3]]
		};

		for (i = 0; i < 160; i++) {
			// stretch crashing here for some reason
			scanline[i] = palette[tileRow[x]];

			x++;
			if (x == 8) {
				x = 0;
				lineOffset = (lineOffset + 1) & 31;
				tile = vram[mapOffset + lineOffset];
				tileRow = tiles->data[tile][y];
			}
		}
	}

	unsigned short* scanline = (unsigned short*)scanlineStart;

	// if sprites enabled
	{
		const unsigned short palette[8] = {
			colorPalette[spritePalette[0][0]],
			colorPalette[spritePalette[0][1]],
			colorPalette[spritePalette[0][2]],
			colorPalette[spritePalette[0][3]],
			colorPalette[spritePalette[1][0]],
			colorPalette[spritePalette[1][1]],
			colorPalette[spritePalette[1][2]],
			colorPalette[spritePalette[1][3]],
		};

		for (int i = 0; i < 40; i++) {
			const sprite& sprite = ((struct sprite *)oam)[i];

			if (sprite.x) {
				int sy = sprite.y - 16;

				if (sy <= gpu.scanline && (sy + 8) > gpu.scanline) {
					int sx = sprite.x - 8;

					int pixelOffset = sx;

					unsigned char tileRow;
					if (sprite.vFlip) tileRow = 7 - (gpu.scanline - sy);
					else tileRow = gpu.scanline - sy;

					int x;
					if (sprite.priority) {
						if (sprite.hFlip) {
							for (x = 7; x >= 0; x--, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == palette[0]) {
									unsigned char colour = tiles->data[sprite.tile][tileRow][x];

									if (colour) {
										scanline[pixelOffset] = palette[sprite.palette * 4 + colour];
									}
								}
							}
						}
						else {
							for (x = 0; x < 8; x++, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == palette[0]) {
									unsigned char colour = tiles->data[sprite.tile][tileRow][x];

									if (colour) {
										scanline[pixelOffset] = palette[sprite.palette * 4 + colour];
									}
								}
							}
						}
					}
					else {
						if (sprite.hFlip) {
							for (x = 7; x >= 0; x--, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160) {
									unsigned char colour = tiles->data[sprite.tile][tileRow][x];

									if (colour) {
										scanline[pixelOffset] = palette[sprite.palette * 4 + colour];
									}
								}
							}
						}
						else {
							for (x = 0; x < 8; x++, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160) {
									unsigned char colour = tiles->data[sprite.tile][tileRow][x];

									if (colour) {
										scanline[pixelOffset] = palette[sprite.palette * 4 + colour];
									}
								}
							}
						}
					}
				}
			}
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
		curfps = 40960 / tickdiff;
		lastticks = ticks;
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
void(*drawFramebuffer)(void) = drawEmu;

void SetupDisplayDriver(bool withStretch, char withFrameskip) {
	frameSkip = withFrameskip;
}
void SetupDisplayColors(unsigned short c0, unsigned short c1, unsigned short c2, unsigned short c3) {
	colorPalette[0] = c0;
	colorPalette[1] = c1;
	colorPalette[2] = c2;
	colorPalette[3] = c3;
}

#endif