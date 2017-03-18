#pragma once

#include "platform.h"
#include "display.h"

#define GPU_CONTROL_BGENABLE (1 << 0)
#define GPU_CONTROL_SPRITEENABLE (1 << 1)
#define GPU_CONTROL_SPRITEVDOUBLE (1 << 2)
#define GPU_CONTROL_TILEMAP (1 << 3)
#define GPU_CONTROL_TILESET (1 << 4)
#define GPU_CONTROL_WINDOWENABLE (1 << 5)
#define GPU_CONTROL_WINDOWTILEMAP (1 << 6)
#define GPU_CONTROL_DISPLAYENABLE (1 << 7)

// gpu mode stored in bits 1-0 of STAT
enum gpuMode {
	GPU_MODE_HBLANK = 0,					// 00 : horizontal blank
	GPU_MODE_VBLANK = 1,					// 01 : vertical blank
	GPU_MODE_OAM = 2,						// 10 : OAM inaccessible
	GPU_MODE_VRAM = 3,						// 11 : OAM and VRAM inaccessible
};


#define SET_LCDC_MODE(x) cpu.memory.STAT_lcdstatus = (cpu.memory.STAT_lcdstatus & 0xFC) | (x)
#define GET_LCDC_MODE() (cpu.memory.STAT_lcdstatus & 0x03)

struct gpu_type {
	unsigned long tick;
} extern gpu;

struct sprite {
	#ifdef LITTLE_E
		unsigned char y;
		unsigned char x;
		unsigned char tile;
		unsigned char priority : 1;
		unsigned char vFlip : 1;
		unsigned char hFlip : 1;
		unsigned char palette : 1;
	#else
		unsigned char y;
		unsigned char x;
		unsigned char tile;
		unsigned char palette : 1;
		unsigned char hFlip : 1;
		unsigned char vFlip : 1;
		unsigned char priority : 1;
	#endif
};

struct tilestype {
	unsigned char data[384][8][8];
};

extern tilestype* tiles;

extern unsigned char backgroundPalette[4];
extern unsigned char spritePalette[2][4];

void gpuStep(void);

void hblank(void);

void updateTile(unsigned short address, unsigned char value);
