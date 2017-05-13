
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
#include "keys.h"
#include "ptune2_simple/Ptune2_direct.h"

int framecounter = 0;

static int frameSkip = 0;
static bool skippingFrame = false;			// whether the current frame is being skipped, determined by frameSkip value

// default render callbacks to 0
void(*renderScanline)(void) = 0;
void(*renderBlankScanline)(void) = 0;
void(*resolveRenderedLine)(void) = 0;
void(*drawFramebuffer)(void) = 0;

#include "tilerow.inl"
#include "dmg_scanline.inl"
#include "cgb_scanline.inl"
#include "scanline_resolve.inl"

#define LCD_GRAM	0x202
#define LCD_BASE	0xB4000000
#define SYNCO() __asm__ volatile("SYNCO\n\t":::"memory");

// DMA0 operation register
#define DMA0_DMAOR	(volatile unsigned short*)0xFE008060
#define DMA0_CHCR_0	(volatile unsigned*)0xFE00802C
#define DMA0_SAR_0	(volatile unsigned*)0xFE008020
#define DMA0_DAR_0	(volatile unsigned*)0xFE008024
#define DMA0_TCR_0	(volatile unsigned*)0xFE008028

// scanline buffer
#define scanGroup ((unsigned short*) 0xE5007000)
static int curScanBuffer = 0;

// current line within the scanline buffer
static int curScan = 0;

int* lineBuffer = ((int*)0xE5017000);
int* prevLineBuffer = ((int*)0xE5017400);

void DmaWaitNext(void) {
	// enable burst mode now that we are waiting
	// *DMA0_CHCR_0 |= 0x20;

	while (1) {
		if ((*DMA0_DMAOR) & 4)//Address error has occurred stop looping
			break;
		if ((*DMA0_CHCR_0) & 2)//Transfer is done
			break;

		condSoundUpdate();
	}

	SYNCO();
	*DMA0_CHCR_0 &= ~1;
	*DMA0_DMAOR = 0;
}

void DmaDrawStrip(void* srcAddress, unsigned int size) {
	// disable dma so we can issue new command
	*DMA0_CHCR_0 &= ~1;
	*DMA0_DMAOR = 0;

	*((volatile unsigned*)MSTPCR0) &= ~(1 << 21);//Clear bit 21
	*DMA0_SAR_0 = ((unsigned int)srcAddress);           //Source address is VRAM
	*DMA0_DAR_0 = LCD_BASE & 0x1FFFFFFF;				//Destination is LCD
	*DMA0_TCR_0 = size / 32;							//Transfer count bytes/32
	*DMA0_CHCR_0 = 0x00101400;
	*DMA0_DMAOR |= 1;//Enable DMA on all channels
	*DMA0_DMAOR &= ~6;//Clear flags

	*DMA0_CHCR_0 |= 1;//Enable channel0 DMA
}

inline void flushScanBuffer(int startX, int endX, int startY, int endY, int scanBufferSize) {
	TIME_SCOPE();

	DmaWaitNext();

	Bdisp_WriteDDRegister3_bit7(1);
	Bdisp_DefineDMARange(startX, endX, startY, endY);
	Bdisp_DDRegisterSelect(LCD_GRAM);

	DmaDrawStrip(&scanGroup[curScanBuffer * scanBufferSize], scanBufferSize);
	curScanBuffer = 1 - curScanBuffer;

	curScan = 0;
}

void resolveScanline_NONE() {
	TIME_SCOPE();

	const int bufferLines = 8;	// 320 bytes * 8 lines = 2560
	const int scanBufferSize = bufferLines * 160 * 2;

	curScan++;
	if (curScan == bufferLines) {
		// we've rendered as much as we can buffer, resolve to pixels and DMA it:
		lineBuffer = ((int*)0xE5017000);
		unsigned int* scanline = (unsigned int*)&scanGroup[curScanBuffer*scanBufferSize];
		for (int i = 0; i < bufferLines; i++) {
			DirectScanline16(scanline);
			scanline += 80;
			lineBuffer += 176;

			condSoundUpdate();
		}

		// send DMA
		flushScanBuffer(118, 277, 36 + cpu.memory.LY_lcdline - bufferLines + 1, 36 + cpu.memory.LY_lcdline, scanBufferSize);

		// move line buffer to front
		lineBuffer = ((int*)0xE5017000);
	} else {
		// move line buffer down a line
		lineBuffer += 176;
	}
}

static const int scaledScanline[16] = {
	0,  1,  3, 4,
	6,  7,  9, 10,
	12, 13, 15, 16,
	18, 19, 21, 22
};

void resolveScanline_LO_200(void) {
	TIME_SCOPE();

	const int bufferLines = 4;	// 960 bytes * 4 lines = 3840
	const int scanBufferSize = bufferLines * 160 * 4 * 3 / 2;
	
	curScan++;
	if (curScan == bufferLines) {
		// we've rendered as much as we can buffer, resolve to pixels and DMA it:
		prevLineBuffer = ((int*)0xE5017000);
		lineBuffer = ((int*)0xE5017000) + 176;
		unsigned int* scanline = (unsigned int*)&scanGroup[curScanBuffer*scanBufferSize];
		for (int i = 0; i < bufferLines; i += 2) {
			DirectTripleScanline32(scanline, scanline + 160, scanline + 320);
			scanline += 480;
			lineBuffer += 176 * 2;
			prevLineBuffer += 176 * 2;

			condSoundUpdate();
		}

		// send DMA
		int startLine = (cpu.memory.LY_lcdline - bufferLines + 1) * 3 / 2;
		flushScanBuffer(38, 357, startLine, startLine + bufferLines * 3 / 2 - 1, scanBufferSize);

		// move line buffer to front
		lineBuffer = ((int*)0xE5017000);
	} else {
		// move line buffer down a line
		lineBuffer += 176;
	}
}

void resolveScanline_HI_200(void) {
	TIME_SCOPE();

	const int bufferLines = 4;	// 960 bytes * 4 lines = 3840
	const int scanBufferSize = bufferLines * 160 * 4 * 3 / 2;
	
	curScan++;
	if (curScan == bufferLines) {
		// we've rendered as much as we can buffer, resolve to pixels and DMA it:
		prevLineBuffer = ((int*)0xE5017000);
		lineBuffer = ((int*)0xE5017000) + 176;
		unsigned int* scanline = (unsigned int*)&scanGroup[curScanBuffer*scanBufferSize];
		for (int i = 0; i < bufferLines; i += 2) {
			BlendTripleScanline32(scanline, scanline + 160, scanline + 320);
			scanline += 480;
			lineBuffer += 176 * 2;
			prevLineBuffer += 176 * 2;

			condSoundUpdate();
		}

		// send DMA
		int startLine = (cpu.memory.LY_lcdline - bufferLines + 1) * 3 / 2;
		flushScanBuffer(38, 357, startLine, startLine + bufferLines * 3 / 2 - 1, scanBufferSize);

		// move line buffer to front
		lineBuffer = ((int*)0xE5017000);
	} else {
		// move line buffer down a line
		lineBuffer += 176;
	}
}

void resolveScanline_LO_150(void) {
	TIME_SCOPE();

	const int bufferLines = 6;	// 720 bytes * 6 lines = 4320 (this somehow works... I'm not gonna mess with it)
	const int scanBufferSize = bufferLines * 480 * 3 / 2;

	curScan++;
	if (curScan == bufferLines) {
		// we've rendered as much as we can buffer, resolve to pixels and DMA it:
		prevLineBuffer = ((int*)0xE5017000);
		lineBuffer = ((int*)0xE5017000) + 176;
		unsigned int* scanline = (unsigned int*)&scanGroup[curScanBuffer*scanBufferSize];
		for (int i = 0; i < bufferLines; i += 2) {
			DirectTripleScanline24(scanline, scanline + 120, scanline + 240);
			scanline += 360;
			lineBuffer += 176 * 2;
			prevLineBuffer += 176 * 2;

			condSoundUpdate();
		}

		// send DMA
		int startLine = (cpu.memory.LY_lcdline - bufferLines + 1) * 3 / 2;
		flushScanBuffer(78, 317, startLine, startLine + bufferLines * 3 / 2 - 1, scanBufferSize);

		// move line buffer to front
		lineBuffer = ((int*)0xE5017000);
	} else {
		// move line buffer down a line
		lineBuffer += 176;
	}
}

void resolveScanline_HI_150(void) {
	TIME_SCOPE();

	const int bufferLines = 6;	// 720 bytes * 6 lines = 4320 (this somehow works... I'm not gonna mess with it)
	const int scanBufferSize = bufferLines * 480 * 3 / 2;

	curScan++;
	if (curScan == bufferLines) {
		// we've rendered as much as we can buffer, resolve to pixels and DMA it:
		prevLineBuffer = ((int*)0xE5017000);
		lineBuffer = ((int*)0xE5017000) + 176;
		unsigned int* scanline = (unsigned int*)&scanGroup[curScanBuffer*scanBufferSize];
		for (int i = 0; i < bufferLines; i += 2) {
			BlendTripleScanline24(scanline, scanline + 120, scanline + 240);
			scanline += 360;
			lineBuffer += 176 * 2;
			prevLineBuffer += 176 * 2;

			condSoundUpdate();
		}

		// send DMA
		int startLine = (cpu.memory.LY_lcdline - bufferLines + 1) * 3 / 2;
		flushScanBuffer(78, 317, startLine, startLine + bufferLines * 3 / 2 - 1, scanBufferSize);

		// move line buffer to front
		lineBuffer = ((int*)0xE5017000);
	} else {
		// move line buffer down a line
		lineBuffer += 176;
	}
}

void scanlineDMG(void) {
	if (skippingFrame)
		return;

	TIME_SCOPE();

	RenderDMGScanline();
	resolveRenderedLine();
}

void scanlineCGB() {
	if (skippingFrame)
		return;

	TIME_SCOPE();

	RenderCGBScanline();

	if (cgb.dirtyPalette) {
		cgbResolvePalette();
	}

	resolveRenderedLine();
}

void scanlineBlank(void) {
	skippingFrame = false;

	TIME_SCOPE();

	memset(lineBuffer, 0, sizeof(int) * 167);
	resolveRenderedLine();
}

void drawFramebufferMain(void) {
	TIME_SCOPE();

	// frame counter
	framecounter++;

	// use TMU1 to establish time between frames
	static unsigned int counterStart = 0x7FFFFFFF;		// max int
	unsigned int counterRegs = 0x0004;
	unsigned int tmu1Clocks = 0;
	if (REG_TMU_TCR_1 != counterRegs || (REG_TMU_TSTR & 2) == 0) {
		// tmu1 needs to be set up
		REG_TMU_TSTR &= ~(1 << 1);

		REG_TMU_TCOR_1 = counterStart;   // max int
		REG_TMU_TCNT_1 = counterStart;
		REG_TMU_TCR_1 = counterRegs;    // max division, no interrupt needed

		// enable TMU1
		REG_TMU_TSTR |= (1 << 1);

		tmu1Clocks = 0;
	} else {
		// expected TMU1 based sim frame time (for 59.7 FPS)
		unsigned int simFrameTime = Ptune2_GetPLLFreq() * 241 >> Ptune2_GetPFCDiv();
		tmu1Clocks = counterStart - REG_TMU_TCNT_1;

		// auto frameskip adjustment based on pre clamped time
		static unsigned int collectedTime = 0;
		static unsigned int collectedFrames = 0;
		collectedTime += tmu1Clocks;
		collectedFrames++;
		if (frameSkip < 0 && collectedFrames > 12) {
			int speedUpTime = simFrameTime * 95 / 100;
			int slowDownTime = simFrameTime * 105 / 100;
			int avgTime = collectedTime / collectedFrames;
			collectedTime = 0;
			collectedFrames = 0;

			if (avgTime < speedUpTime && frameSkip != -1) {
				frameSkip++;
			} else if (avgTime > slowDownTime && frameSkip != -25) {
				frameSkip--;
			}
		}

		// clamp speed by waiting for frame time
		if (emulator.settings.clampSpeed) {
			int rtcBackup = RTC_GetTicks();
			while (tmu1Clocks < simFrameTime && RTC_GetTicks() - rtcBackup < 3) {
				condSoundUpdate();
				// sleep .1 milliseconds at a time til we are ready for the frame
				CMT_Delay_micros(100);
				tmu1Clocks = counterStart - REG_TMU_TCNT_1;
			}
		}

		counterStart = REG_TMU_TCNT_1;
	}

	// determine frame rate based on last framebuffer call:
#if DEBUG
	static unsigned int totalClocks = 0;
	totalClocks += tmu1Clocks;
	if (framecounter % 256 == 0) {
		static int fps = 0;
		int curfps = ((143994 * 256 * Ptune2_GetPLLFreq()) >> Ptune2_GetPFCDiv()) / totalClocks;

		if (curfps != fps) {
			fps = curfps;
			// report frame rate:
			memset(ScopeTimer::debugString, 0, sizeof(ScopeTimer::debugString));
			sprintf(ScopeTimer::debugString, "FPS:%d.%d, Skip:%d", fps / 10, fps % 10, frameSkip);
		}

		totalClocks = 0;
	}
#endif

	// determine frame skip
	skippingFrame = false;
	if (frameSkip && framecounter > 4) {
		if (frameSkip < 0) {
			// reverse the bits of the previous frame counter xor'd with this one and
			// compare against our ratio:
			int f = (framecounter % 32) ^ ((framecounter + 1) % 32);
			f = ((f & 0x10) >> 4) | ((f & 0x8) >> 2) | ((f & 0x2) << 2) | ((f & 0x01) << 4);

			// negative is automatic (frame skip amt stored in HOW negative it is)
			skippingFrame = f < -frameSkip;
		} else {
			skippingFrame = (framecounter % (frameSkip + 1)) != 0;
		}
	}

	if (emulator.settings.sound) sndUpdate();

	if (skippingFrame) {
		refreshKeys(false);
		return;
	}

	// frame end.. kill DMA operations to make sure they stay in sync
	DmaWaitNext();
	*((volatile unsigned*)MSTPCR0) &= ~(1 << 21);//Clear bit 21

	// good time to refresh keys and check for os requests and such
	refreshKeys(true);
}

void SetupDisplayDriver(char withFrameskip) {
	frameSkip = withFrameskip;

	drawFramebuffer = drawFramebufferMain;

	renderScanline = cgb.isCGB ? scanlineCGB : scanlineDMG;
	renderBlankScanline = scanlineBlank;

	lineBuffer = ((int*)0xE5017000);
	prevLineBuffer = ((int*)0xE5017400);

	switch (emulator.settings.scaleMode) {
		case emu_scale::LO_150:
			resolveRenderedLine = resolveScanline_LO_150;
			break;
		case emu_scale::HI_150:
			resolveRenderedLine = resolveScanline_HI_150;
			break;
		case emu_scale::LO_200:
			resolveRenderedLine = resolveScanline_LO_200;
			break;
		case emu_scale::HI_200:
			resolveRenderedLine = resolveScanline_HI_200;
			break;
		default:
			resolveRenderedLine = resolveScanline_NONE;
	}
}

#endif