#pragma once

#include "platform.h"
#include "display.h"
#include "cpu.h"

#define LCDC_BGENABLE (1 << 0)
#define LCDC_SPRITEENABLE (1 << 1)
#define LCDC_SPRITEVDOUBLE (1 << 2)
#define LCDC_BGTILEMAP (1 << 3)
#define LCDC_TILESET (1 << 4)
#define LCDC_WINDOWENABLE (1 << 5)
#define LCDC_WINDOWTILEMAP (1 << 6)
#define LCDC_DISPLAYENABLE (1 << 7)

#define STAT_LYCCHECK (1 << 6)
#define STAT_OAMCHECK (1 << 5)
#define STAT_VBLANKCHECK (1 << 4)
#define STAT_HBLANKCHECK (1 << 3)
#define STAT_LYCSIGNAL (1 << 2)
#define STAT_MODE (3)

// gpu mode stored in bits 1-0 of STAT
enum gpuMode {
	GPU_MODE_HBLANK = 0,					// 00 : horizontal blank
	GPU_MODE_VBLANK = 1,					// 01 : vertical blank
	GPU_MODE_OAM = 2,						// 10 : OAM inaccessible
	GPU_MODE_VRAM = 3,						// 11 : OAM and VRAM inaccessible
};

// clock times for each of the above modes (vblank is just for one line, and index 5 is the special gap difference for LY=0)
extern unsigned int gpuTimes[5];

#define SET_LCDC_MODE(x) cpu.memory.STAT_lcdstatus = (cpu.memory.STAT_lcdstatus & 0xFC) | (x)
#define GET_LCDC_MODE() (cpu.memory.STAT_lcdstatus & STAT_MODE)

inline bool gpuCheck() {
	return (cpu.clocks >= cpu.gpuTick || (cpu.halted && cpu.IME));
}

struct sprite {
	unsigned char y;
	unsigned char x;
	unsigned char tile;
	unsigned char attr;
};

#define OAM_ATTR_PRIORITY(x) (x & 0x80)
#define OAM_ATTR_YFLIP(x) (x & 0x40)
#define OAM_ATTR_XFLIP(x) (x & 0x20)
#define OAM_ATTR_PALETTE(x) (x & 0x10)
#define OAM_ATTR_BANK(x) (x & 0x08)			    // CGB only
#define OAM_ATTR_PAL_NUM(x) (x & 07)			// CGB only

extern void(*gpuStep)(void);

// different gpu steps based on LCD status
extern void stepLCDOn_OAM(void);
extern void stepLCDOn_VRAM(void);
extern void stepLCDOn_HBLANK(void);
extern void stepLCDOn_VBLANK(void);

// shared color palette (the colors are two pixels wide to make stretching code faster)
extern unsigned int ppuPalette[64];

void hblank(void);

void enablePausePreview();

extern int framecounter;

// line buffer rendered too during scanline render functions
#if TARGET_WINSIM
extern int lineBuffer[176] ALIGN(32);
extern int prevLineBuffer[168] ALIGN(32);
#else
extern int* lineBuffer;
extern int* prevLineBuffer;
#endif

inline void resolveDMGBGPalette() {
	ppuPalette[0] = ppuPalette[12 + ((cpu.memory.BGP_bgpalette & 0x03) >> 0)];
	ppuPalette[1] = ppuPalette[12 + ((cpu.memory.BGP_bgpalette & 0x0C) >> 2)];
	ppuPalette[2] = ppuPalette[12 + ((cpu.memory.BGP_bgpalette & 0x30) >> 4)];
	ppuPalette[3] = ppuPalette[12 + ((cpu.memory.BGP_bgpalette & 0xC0) >> 6)];
}

inline void resolveDMGOBJ0Palette() {
	ppuPalette[4] = ppuPalette[16 + ((cpu.memory.OBP0_spritepal0 & 0x03) >> 0)];
	ppuPalette[5] = ppuPalette[16 + ((cpu.memory.OBP0_spritepal0 & 0x0C) >> 2)];
	ppuPalette[6] = ppuPalette[16 + ((cpu.memory.OBP0_spritepal0 & 0x30) >> 4)];
	ppuPalette[7] = ppuPalette[16 + ((cpu.memory.OBP0_spritepal0 & 0xC0) >> 6)];
}

inline void resolveDMGOBJ1Palette() {
	ppuPalette[8] =  ppuPalette[20 + ((cpu.memory.OBP1_spritepal1 & 0x03) >> 0)];
	ppuPalette[9] =  ppuPalette[20 + ((cpu.memory.OBP1_spritepal1 & 0x0C) >> 2)];
	ppuPalette[10] = ppuPalette[20 + ((cpu.memory.OBP1_spritepal1 & 0x30) >> 4)];
	ppuPalette[11] = ppuPalette[20 + ((cpu.memory.OBP1_spritepal1 & 0xC0) >> 6)];
}

// color mixing helpers
inline unsigned short mix565(unsigned int X, unsigned int Y) {
	// RGB565 color mix
	// thanks to http://www.slack.net/~ant/info/rgb_mixing.html
	return (X + Y - ((X ^ Y) & 0x0821)) >> 1;
}

inline unsigned int mix565_32(unsigned int X, unsigned int Y) {
	// RGB565 color mix with two simultaneous colors
	return ((X + Y - ((X ^ Y) & 0x08210821)) >> 1) | ((X & Y & 0x80000000));
}