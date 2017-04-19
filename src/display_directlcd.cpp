
// display DMA not supported in windows simulator or emulator
#if TARGET_PRIZM

#include "platform.h"
#include "debug.h"
#include "emulator.h"

#include "memory.h"
#include "gpu.h"
#include "debug.h"
#include "main.h"

#include "display.h"

#include "snd.h"

int framecounter = 0;

static int frameSkip = 0;
static bool skippingFrame = false;			// whether the current frame is being skipped, determined by frameSkip value

// default render callbacks to 0
void(*renderScanline)(void) = 0;
void(*renderBlankScanline)(void) = 0;
void(*drawFramebuffer)(void) = 0;

void renderScanline1x1_DMG(void);
void renderScanline1x1_CGB(void);
void renderBlankScanline1x1(void);
void renderScanlineFit_DMG(void);
void renderScanlineFit_CGB(void);
void renderBlankScanlineFit(void);
void drawFramebufferMain(void);

#define lineBuffer ((int*)0xE5017000)

#include "dmg_scanline.inl"
#include "cgb_scanline.inl"

void SetupDisplayDriver(bool withStretch, char withFrameskip) {
	frameSkip = withFrameskip;

	drawFramebuffer = drawFramebufferMain;

	if (cgb.isCGB) {
		renderScanline = withStretch ? renderScanlineFit_CGB : renderScanline1x1_CGB;
	} else {
		renderScanline = withStretch ? renderScanlineFit_DMG : renderScanline1x1_DMG;
	}
	renderBlankScanline = withStretch ? renderBlankScanlineFit : renderBlankScanline1x1;
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

void renderScanline1x1_DMG(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 2;

	if (skippingFrame)
		return;

	TIME_SCOPE();

	RenderDMGScanline();

	void* scanlineStart = &scanGroup[160 * curScan + curScanBuffer*scanBufferSize];
	ResolveDMGScanline<unsigned short>(scanlineStart);

	// blit every SCANLINE_BUFFER # lines
	curScan++;
	if (curScan == SCANLINE_BUFFER)
	{
		TIME_SCOPE_NAMED(Scanline_Flush);
		scanlineFlush1x1();
	}
}

void renderScanline1x1_CGB(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 2;

	if (skippingFrame)
		return;

	TIME_SCOPE();

	RenderCGBScanline();

	void* scanlineStart = &scanGroup[160 * curScan + curScanBuffer*scanBufferSize];
	ResolveCGBScanline<unsigned short>(scanlineStart);

	// blit every SCANLINE_BUFFER # lines
	curScan++;
	if (curScan == SCANLINE_BUFFER)
	{
		TIME_SCOPE_NAMED(Scanline_Flush);
		scanlineFlush1x1();
	}
}

void renderBlankScanline1x1(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 2;

	skippingFrame = false;

	TIME_SCOPE();

	void* scanlineStart = &scanGroup[160 * curScan + curScanBuffer*scanBufferSize];

	unsigned short* curScanLine = (unsigned short*) scanlineStart;
	unsigned short blank = cgb.isCGB ? 0xFFFF : (unsigned short)dmgPalette[0];
	for (int i = 0; i < 160; i++) {
		*(curScanLine++) = blank;
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

void renderScanlineFit_DMG(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 4 * 3 / 2;

	if (skippingFrame)
		return;

	TIME_SCOPE();

	const int scanBufferOffset[8] = {
		0, 1, 3, 4,
		6, 7, 9, 10,
	};

	RenderDMGScanline();

	unsigned short* scanlineStart =
#if !USEMEMCPY
		&scanGroup[320 * scanBufferOffset[curScan] + curScanBuffer*scanBufferSize];
#else
		&scanGroup[320 * curScan + curScanBuffer*scanBufferSize];
#endif

#if !USEMEMCPY
#if SCANLINE_BUFFER == 2
	// special optimization doing 2 lines at a time, flush with each:
	ResolveScanline<unsigned int>(scanlineStart);
	curScan++;
	scanlineFlushFit();
	return;
#else
	if ((curScan & 1) == 1) {
		DoubleResolveDMGScanline<unsigned int>(scanlineStart, scanlineStart + 320);
	} else {
		ResolveDMGScanline<unsigned int>(scanlineStart);
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

void renderScanlineFit_CGB(void) {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 4 * 3 / 2;

	if (skippingFrame)
		return;

	TIME_SCOPE();

	const int scanBufferOffset[8] = {
		0, 1, 3, 4,
		6, 7, 9, 10,
	};

	RenderCGBScanline();

	unsigned short* scanlineStart =
#if !USEMEMCPY
		&scanGroup[320 * scanBufferOffset[curScan] + curScanBuffer*scanBufferSize];
#else
		&scanGroup[320 * curScan + curScanBuffer*scanBufferSize];
#endif

#if !USEMEMCPY
#if SCANLINE_BUFFER == 2
	// special optimization doing 2 lines at a time, flush with each:
	ResolveScanline<unsigned int>(scanlineStart);
	curScan++;
	scanlineFlushFit();
	return;
#else
	if ((curScan & 1) == 1) {
		DoubleResolveCGBScanline<unsigned int>(scanlineStart, scanlineStart + 320);
	} else {
		ResolveCGBScanline<unsigned int>(scanlineStart);
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

void renderBlankScanlineFit() {
	const int scanBufferSize = SCANLINE_BUFFER * 160 * 4 * 3 / 2;

	skippingFrame = false;

	TIME_SCOPE();

	const int scanBufferOffset[8] = {
		0, 1, 3, 4,
		6, 7, 9, 10,
	};


	void* scanlineStart =
#if !USEMEMCPY
		&scanGroup[320 * scanBufferOffset[curScan] + curScanBuffer*scanBufferSize];
#else
		&scanGroup[320 * curScan + curScanBuffer*scanBufferSize];
#endif

	unsigned int* curScanInt = (unsigned int*)scanlineStart;
	unsigned int blank = cgb.isCGB ? 0xFFFFFFFF : dmgPalette[0];
	for (int i = 0; i < 160; i++) {
		*(curScanInt++) = blank;
	}

#if !USEMEMCPY
#if SCANLINE_BUFFER == 2
	// special optimization doing 2 lines at a time, flush with each:
	curScan++;
	scanlineFlushFit();
	return;
#else
	if ((curScan & 1) == 1) {
		scanlineStart = &scanGroup[320 * (scanBufferOffset[curScan] + 1) + curScanBuffer*scanBufferSize];
		curScanInt = (unsigned int*)scanlineStart;
		for (int i = 0; i < 160; i++) {
			*(curScanInt++) = blank;
		}
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

	static unsigned int rtc_lastticks = 0;
	if (framecounter % 32 == 0) {
		int ticks = RTC_GetTicks();
		int tickdiff = ticks - rtc_lastticks;
		curfps = 40960 / tickdiff;
		rtc_lastticks = ticks;

		if (curfps != fps) {
			fps = curfps;
#if DEBUG
			// report frame rate:
			memset(ScopeTimer::debugString, 0, sizeof(ScopeTimer::debugString));
			sprintf(ScopeTimer::debugString, "FPS:%d.%d, Skip:%d", fps / 10, fps % 10, frameSkip);
#endif

			// auto frameskip adjustment
			if (frameSkip < 0) {
				if (fps > 638 && frameSkip != -1) {
					frameSkip++;
				}
				else if (fps && fps < 480 && frameSkip != -3) {
					frameSkip--;
				}
			}
		}
	}

	// TODO : not the best... only clamps speed to 64 FPS (7% too high)
	if (emulator.settings.clampSpeed) {
		static int lastClampTicks = 0;
		int curTicks = RTC_GetTicks();
		while (curTicks == lastClampTicks || curTicks == lastClampTicks + 1) { 
			condSoundUpdate();
			curTicks = RTC_GetTicks();
		}
		lastClampTicks = curTicks;
	}

	// determine frame skip
	skippingFrame = false;
	if (frameSkip && framecounter > 4) {
		if (frameSkip < 0) {
			// negative is automatic (frame skip amt stored in HOW negative it is)
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
		condSoundUpdate();
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