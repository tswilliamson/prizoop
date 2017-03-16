
// display DMA not supported in windows simulator or emulator
#if TARGET_PRIZM

#include "platform.h"
#include "memory.h"
#include "gpu.h"
#include "debug.h"
#include "main.h"

#include "display.h"

#include "fxcg\display.h"

static int frameSkip = 0;
static int framecounter = 0;
static bool skippingFrame = false;			// whether the current frame is being skipped, determined by frameSkip value

// default render callbacks to 0
void(*renderScanline)(void) = 0;
void(*drawFramebuffer)(void) = 0;

void renderScanline1x1(void);
void renderScanlineFit(void);
void drawFramebufferMain(void);

unsigned short colorPalette[4] = {
	COLOR_WHITE,
	COLOR_LIGHTCYAN,
	COLOR_CYAN,
	COLOR_DARKCYAN,
};

void SetupDisplayDriver(bool withStretch, char withFrameskip) {
	frameSkip = withFrameskip;

	drawFramebuffer = drawFramebufferMain;
	renderScanline = withStretch ? renderScanlineFit : renderScanline1x1;
}

void SetupDisplayColors(unsigned short c0, unsigned short c1, unsigned short c2, unsigned short c3) {
	colorPalette[0] = c0;
	colorPalette[1] = c1;
	colorPalette[2] = c2;
	colorPalette[3] = c3;
}

// number of scanlines to buffer, 2, 4, 6, or 8. Only supports up to 4 when using DMA
#define SCANLINE_BUFFER 4

// mem copy or DMA into LCD controller
#define USEMEMCPY 0

#define LCD_GRAM	0x202
#define LCD_BASE	0xB4000000
#define SYNCO() __asm__ volatile("SYNCO\n\t":::"memory");

// DMA0 operation register
#define DMA0_DMAOR	(volatile unsigned short*)0xFE008060
#define DMA0_CHCR_0	(volatile unsigned*)0xFE00802C
#define DMA0_SAR_0	(volatile unsigned*)0xFE008020
#define DMA0_DAR_0	(volatile unsigned*)0xFE008024
#define DMA0_TCR_0	(volatile unsigned*)0xFE008028


void DmaWaitNext(void) {
	while (1) {
		if ((*DMA0_DMAOR) & 4)//Address error has occurred stop looping
			break;
		if ((*DMA0_CHCR_0) & 2)//Transfer is done
			break;
	}
	SYNCO();
	*DMA0_CHCR_0 &= ~1;
	*DMA0_DMAOR = 0;
}

#if !USEMEMCPY
void DmaDrawStrip(void* srcAddress, unsigned int size) {
	// disable dma so we can issue new command
	*DMA0_CHCR_0 &= ~1;
	*DMA0_DMAOR = 0;

	*((volatile unsigned*)MSTPCR0) &= ~(1 << 21);//Clear bit 21
	*DMA0_SAR_0 = ((unsigned int)srcAddress);  // &0x1FFFFFFF;	//Source address is VRAM
	*DMA0_DAR_0 = LCD_BASE & 0x1FFFFFFF;				//Destination is LCD
	*DMA0_TCR_0 = size / 32;							//Transfer count bytes/32
	*DMA0_CHCR_0 = 0x00101400;
	*DMA0_DMAOR |= 1;//Enable DMA on all channels
	*DMA0_DMAOR &= ~6;//Clear flags
	*DMA0_CHCR_0 |= 1;//Enable channel0 DMA
}
#endif

// scanline buffer
#if USEMEMCPY
static unsigned short scanGroup[SCANLINE_BUFFER * 320] ALIGN_256;
const int curScanBuffer = 0;
#else
#define scanGroup ((unsigned short*) 0xE5007000)
static int curScanBuffer = 0;
#endif

static unsigned char curScan = 0;

inline void scanlineFlush1x1() {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 2;

#if USEMEMCPY
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 0], screenWidth * SCANLINE_BUFFER * 2);
#else
	DmaWaitNext();

	Bdisp_WriteDDRegister3_bit7(1);
	Bdisp_DefineDMARange(118, 277, 36 + cpu.memory.LY_lcdline - SCANLINE_BUFFER + 1, 36 + cpu.memory.LY_lcdline);
	Bdisp_DDRegisterSelect(LCD_GRAM);

	DmaDrawStrip(&scanGroup[curScanBuffer * scanBufferSize], scanBufferSize);
	curScanBuffer = 1 - curScanBuffer;
#endif

	curScan = 0;
}

void renderScanline1x1(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 2;

	if (skippingFrame)
		return;

	TIME_SCOPE();

	int mapOffset = (cpu.memory.LY_lcdline & GPU_CONTROL_TILEMAP) ? 0x1c00 : 0x1800;
	mapOffset += (((cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5;

	void* scanlineStart = &scanGroup[160 * curScan + curScanBuffer*scanBufferSize];


	// if bg enabled
	{
		int i;
		int x = cpu.memory.SCX_bgscrollx & 7;
		int y = (cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 7;

		// finish/draw left tile
		int lineOffset = (cpu.memory.SCX_bgscrollx >> 3);
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

				if (sy <= cpu.memory.LY_lcdline && (sy + 8) > cpu.memory.LY_lcdline) {
					int sx = sprite.x - 8;

					int pixelOffset = sx;

					unsigned char tileRow;
					if (sprite.vFlip) tileRow = 7 - (cpu.memory.LY_lcdline - sy);
					else tileRow = cpu.memory.LY_lcdline - sy;

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

	// blit every SCANLINE_BUFFER # lines
	curScan++;
	if (curScan == SCANLINE_BUFFER)
	{
		TIME_SCOPE_NAMED(Scanline_Flush);
		scanlineFlush1x1();
	}
}


inline void scanlineFlushFit() {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 4 * 3 / 2;

#if USEMEMCPY
	// alternating copy bytes to scale y by 50%
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 0], screenWidth * 4);
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 1], screenWidth * 2);
#if SCANLINE_BUFFER > 2
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 2], screenWidth * 4);
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 3], screenWidth * 2);
#if SCANLINE_BUFFER > 4
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 4], screenWidth * 4);
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 5], screenWidth * 2);
#if SCANLINE_BUFFER > 6
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 6], screenWidth * 4);
	memcpy((unsigned short *)(LCD_BASE), &scanGroup[screenWidth * 7], screenWidth * 2);
#endif
#endif
#endif
#else
	DmaWaitNext();

	Bdisp_WriteDDRegister3_bit7(1);

#if SCANLINE_BUFFER == 2
	// special case for stretching by drawing 2 lines at a time, 1 then 2, etc
	int startLine = cpu.memory.LY_lcdline * 3 / 2;
	if (cpu.memory.LY_lcdline & 1) {
		Bdisp_DefineDMARange(38, 357, startLine, startLine + 1);
	} else {
		Bdisp_DefineDMARange(38, 357, startLine, startLine);
	}

	Bdisp_DDRegisterSelect(LCD_GRAM);
	if (curScan == 1) {
		DmaDrawStrip(&scanGroup[curScanBuffer * scanBufferSize], 160 * 4);
	} else {
		DmaDrawStrip(&scanGroup[curScanBuffer * scanBufferSize], 160 * 4 * 2);
		curScanBuffer = 1 - curScanBuffer;
		curScan = 0;
	}

	return;
#else
	int startLine = (cpu.memory.LY_lcdline - SCANLINE_BUFFER + 1) * 3 / 2;
	Bdisp_DefineDMARange(38, 357, startLine, startLine + SCANLINE_BUFFER * 3 / 2 - 1);
#endif

	Bdisp_DDRegisterSelect(LCD_GRAM);

	DmaDrawStrip(&scanGroup[curScanBuffer * scanBufferSize], scanBufferSize);
	curScanBuffer = 1 - curScanBuffer;
#endif

	curScan = 0;
}

void renderScanlineFit(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 4 * 3 / 2;

	if (skippingFrame)
		return;

	TIME_SCOPE();

	const int scanBufferOffset[8] = {
		0, 1, 3, 4,
		6, 7, 9, 10,
	};

	int mapOffset = (gpu.control & GPU_CONTROL_TILEMAP) ? 0x1c00 : 0x1800;
	mapOffset += (((cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5;

	void* scanlineStart =
#if !USEMEMCPY
		&scanGroup[320 * scanBufferOffset[curScan] + curScanBuffer*scanBufferSize];
#else
		&scanGroup[320 * curScan + curScanBuffer*scanBufferSize];
#endif

#if !USEMEMCPY && SCANLINE_BUFFER > 2
repeatForStretch:
#endif

	// if bg enabled
	{
		int i;
		int x = cpu.memory.SCX_bgscrollx & 7;
		int y = (cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 7;

		// finish/draw left tile
		int lineOffset = (cpu.memory.SCX_bgscrollx >> 3);
		unsigned short tile = (unsigned short)vram[mapOffset + lineOffset];
		const unsigned char* tileRow = tiles->data[tile][y];

		unsigned int* scanline = (unsigned int*) scanlineStart;

		const unsigned int palette[4] = {
			colorPalette[backgroundPalette[0]] | (colorPalette[backgroundPalette[0]] << 16),
			colorPalette[backgroundPalette[1]] | (colorPalette[backgroundPalette[1]] << 16),
			colorPalette[backgroundPalette[2]] | (colorPalette[backgroundPalette[2]] << 16),
			colorPalette[backgroundPalette[3]] | (colorPalette[backgroundPalette[3]] << 16),
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

	unsigned int* scanline = (unsigned int*) scanlineStart;

	// if sprites enabled
	{
		const unsigned int palette[8] = {
			colorPalette[spritePalette[0][0]] | (colorPalette[spritePalette[0][0]] << 16),
			colorPalette[spritePalette[0][1]] | (colorPalette[spritePalette[0][1]] << 16),
			colorPalette[spritePalette[0][2]] | (colorPalette[spritePalette[0][2]] << 16),
			colorPalette[spritePalette[0][3]] | (colorPalette[spritePalette[0][3]] << 16),
			colorPalette[spritePalette[1][0]] | (colorPalette[spritePalette[1][0]] << 16),
			colorPalette[spritePalette[1][1]] | (colorPalette[spritePalette[1][1]] << 16),
			colorPalette[spritePalette[1][2]] | (colorPalette[spritePalette[1][2]] << 16),
			colorPalette[spritePalette[1][3]] | (colorPalette[spritePalette[1][3]] << 16),
		};

		for (int i = 0; i < 40; i++) {
			const sprite& sprite = ((struct sprite *)oam)[i];

			if (sprite.x) {
				int sy = sprite.y - 16;

				if (sy <= cpu.memory.LY_lcdline && (sy + 8) > cpu.memory.LY_lcdline) {
					int sx = sprite.x - 8;

					int pixelOffset = sx;

					unsigned char tileRow;
					if (sprite.vFlip) tileRow = 7 - (cpu.memory.LY_lcdline - sy);
					else tileRow = cpu.memory.LY_lcdline - sy;

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

#if !USEMEMCPY
#if SCANLINE_BUFFER == 2
	// special optimization doing 2 lines at a time, flush with each:
	curScan++;
	scanlineFlushFit();
	return;
#else
	void* nextLine = &scanGroup[320 * (scanBufferOffset[curScan]+1) + curScanBuffer*scanBufferSize];
	if ((curScan & 1) == 1 && scanlineStart != nextLine) {
		scanlineStart = nextLine;
		goto repeatForStretch;
	}
#endif
#endif

	// blit every SCANLINE_BUFFER # lines
	curScan++;
	if (curScan == SCANLINE_BUFFER)
	{
		TIME_SCOPE_NAMED(Scanline_Flush);
		scanlineFlushFit();
	}
}

void drawFramebufferMain(void) {
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
		// report frame rate:
		memset(ScopeTimer::debugString, 0, sizeof(ScopeTimer::debugString));
		sprintf(ScopeTimer::debugString, "FPS:%d.%d, Skip:%d", fps / 10, fps % 10, frameSkip);
	}

	// determine frame skip
	skippingFrame = false;
	if (frameSkip && framecounter > 4) {
		if (frameSkip < 0) {
			// negative is automatic (frame skip amt stored in HOW negative it is)
			if (fps > 597 && frameSkip != -1) {
				frameSkip++;
			} else if (fps && fps < 500 && frameSkip != -4) {
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

	// this doesn't happen often.. scan lines sometimes get lost
	while (curScan) {
		renderScanline();
	}

	// draw frame buffer here
#if USEMEMCPY
	// we write directly to the LCD controller, so do the magic that let's us reset the window
	Bdisp_WriteDDRegister3_bit7(1);
#if STRETCH
	Bdisp_DefineDMARange(38, 357, 0, 215);
#else
	Bdisp_DefineDMARange(118, 277, 36, 179);
#endif
	Bdisp_DDRegisterSelect(LCD_GRAM);

	*((volatile unsigned*) MSTPCR0) &= ~(1 << 21);//Clear bit 21

	// Disable all DMA
	*DMA0_CHCR_0 &= ~1; //Disable DMA on channel 0
	*DMA0_DMAOR = 0;
#else
	// frame end.. kill DMA operations to make sure they stay in sync
	DmaWaitNext();
	*((volatile unsigned*)MSTPCR0) &= ~(1 << 21);//Clear bit 21
	*DMA0_CHCR_0 &= ~1; //Disable DMA on channel 0
	*DMA0_DMAOR = 0;
#endif

	// good time to refresh keys and check for os requests and such
	extern void refresh();
	refresh();
}

#endif