#pragma once

// Shared between platforms, renders a scanline based on the unsigned short color palette to different types
// (we speed up 2x scaling by directly rendering a scanline to unsigned int)

#include "cgb.h"
#include "cpu.h"
#include "gpu.h"
#include "memory.h"

inline void RenderCGBScanline() {
	// background/window
	{
		int i;

		// tile offset and palette shared for window
		const int tileOffset = (cpu.memory.LCDC_ctl & LCDC_TILESET) ? 0 : 256;

		// draw background
		if (cpu.memory.LCDC_ctl & LCDC_BGENABLE)
		{
			int y = (cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 7;

			int yVal[2];
			yVal[0] = y * 2;
			yVal[1] = (7-y) * 2;

			int mapOffset = (cpu.memory.LCDC_ctl & LCDC_BGTILEMAP) ? 0x1c00 : 0x1800;
			mapOffset += (((cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5;

			// start to the left of pixel 7 (our leftmost linebuffer pixel) based on scrollx
			int* scanline = (int*)&lineBuffer[7 - (cpu.memory.SCX_bgscrollx & 7)];

			for (i = 0; i < 168; i += 8) {
				int lineOffset = ((unsigned char)(cpu.memory.SCX_bgscrollx + i)) >> 3;
				int tile = vram[mapOffset + lineOffset];
				int attr = vram[mapOffset + lineOffset + 0x2000];
				if (!(tile & 0x80)) tile += tileOffset;

				// attribute bit 3 is vram bank number, bit 6 is yflip
				int tileRow = *((unsigned short*)&vram[tile * 16 + yVal[(attr & 0x40) >> 6] + ((attr & 0x8) << 10)]);
				ShortSwap(tileRow);

				// attribute bit 0-2 is bg palette
				int paletteMask = (attr & 0x07) << 2;

				// bit 5 is hflip
				if (attr & 0x20) {
					scanline[0] = paletteMask | ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
					scanline[1] = paletteMask | ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
					scanline[2] = paletteMask | ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
					scanline[3] = paletteMask | ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
					scanline[4] = paletteMask | ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
					scanline[5] = paletteMask | ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
					scanline[6] = paletteMask | ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
					scanline[7] = paletteMask | (tileRow >> 15)       | ((tileRow >> 6) & 2);
				} else {
					scanline[0] = paletteMask | (tileRow >> 15)       | ((tileRow >> 6) & 2);
					scanline[1] = paletteMask | ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
					scanline[2] = paletteMask | ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
					scanline[3] = paletteMask | ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
					scanline[4] = paletteMask | ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
					scanline[5] = paletteMask | ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
					scanline[6] = paletteMask | ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
					scanline[7] = paletteMask | ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
				}
				scanline += 8;
			}
		}

		// draw window
		if (cpu.memory.LCDC_ctl & LCDC_WINDOWENABLE)
		{
			int wx = cpu.memory.WX_windowx;
			int y = cpu.memory.LY_lcdline - cpu.memory.WY_windowy;

			if (wx <= 166 && y >= 0) {
				// select map offset row
				int mapOffset = (cpu.memory.LCDC_ctl & LCDC_WINDOWTILEMAP) ? 0x1c00 : 0x1800;
				mapOffset += (y >> 3) << 5;

				int lineOffset = -1;
				y &= 0x07;
				int yVal[2];
				yVal[0] = y * 2;
				yVal[1] = (7-y) * 2;

				int* scanline = (int*)&lineBuffer[wx];

				for (; wx < 167; wx += 8) {
					lineOffset = lineOffset + 1;
					int tile = vram[mapOffset + lineOffset];
					int attr = vram[mapOffset + lineOffset + 0x2000];

					if (!(tile & 0x80)) tile += tileOffset;

					// attribute bit 3 is vram bank number, bit 6 is yflip
					int tileRow = *((unsigned short*)&vram[tile * 16 + yVal[(attr & 0x40) >> 6] + ((attr & 0x8) << 10)]);
					ShortSwap(tileRow);

					// attribute bit 0-2 is bg palette
					int paletteMask = (attr & 0x07) << 2;

					// bit 5 is hflip
					if (attr & 0x20) {
						scanline[0] = paletteMask | ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
						scanline[1] = paletteMask | ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
						scanline[2] = paletteMask | ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
						scanline[3] = paletteMask | ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
						scanline[4] = paletteMask | ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
						scanline[5] = paletteMask | ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
						scanline[6] = paletteMask | ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
						scanline[7] = paletteMask | (tileRow >> 15)       | ((tileRow >> 6) & 2);
					} else {
						scanline[0] = paletteMask | (tileRow >> 15)       | ((tileRow >> 6) & 2);
						scanline[1] = paletteMask | ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
						scanline[2] = paletteMask | ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
						scanline[3] = paletteMask | ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
						scanline[4] = paletteMask | ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
						scanline[5] = paletteMask | ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
						scanline[6] = paletteMask | ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
						scanline[7] = paletteMask | ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
					}
					scanline += 8;
				}
			}
		}
	}

	// if sprites enabled
	if (cpu.memory.LCDC_ctl & LCDC_SPRITEENABLE)
	{
		int spriteSize = (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) ? 16 : 8;

		for (int i = 39; i >= 0; i--) {
			const sprite& sprite = ((struct sprite *)oam)[i];

			if (sprite.x && sprite.x < 168) {
				int sy = sprite.y - 16;

				if (sy <= cpu.memory.LY_lcdline && (sy + spriteSize) > cpu.memory.LY_lcdline) {
					// sprite position
					int* scanline = (int*)&lineBuffer[sprite.x - 1];

					int y;
					int tile = sprite.tile;
					if (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) {
						if (OAM_ATTR_YFLIP(sprite.attr)) y = 15 - (cpu.memory.LY_lcdline - sy);
						else y = cpu.memory.LY_lcdline - sy;
						tile &= 0xFE;
					}
					else {
						if (OAM_ATTR_YFLIP(sprite.attr)) y = 7 - (cpu.memory.LY_lcdline - sy);
						else y = cpu.memory.LY_lcdline - sy;
					}

					// bit 3 is vram bank #
					int tileIndex = tile * 16 + y * 2 + (OAM_ATTR_BANK(sprite.attr) << 10);
					int tileRow = *((unsigned short*)&vram[tileIndex]);
					ShortSwap(tileRow);

					int colors[8];
					if (!OAM_ATTR_XFLIP(sprite.attr)) {
						colors[0] = ((tileRow >> 15) & 1) | ((tileRow >> 6) & 2);
						colors[1] = ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
						colors[2] = ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
						colors[3] = ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
						colors[4] = ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
						colors[5] = ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
						colors[6] = ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
						colors[7] = ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
					} else {
						colors[0] = ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
						colors[1] = ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
						colors[2] = ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
						colors[3] = ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
						colors[4] = ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
						colors[5] = ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
						colors[6] = ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
						colors[7] = ((tileRow >> 15) & 1) | ((tileRow >> 6) & 2);
					}

					// bit 0-2 are palette #'s for CGB
					int paletteBase = 32 + (OAM_ATTR_PAL_NUM(sprite.attr) << 2);
					if (OAM_ATTR_PRIORITY(sprite.attr)) {
						if (!(scanline[0] & 3) && colors[0]) scanline[0] = paletteBase | colors[0];
						if (!(scanline[1] & 3) && colors[1]) scanline[1] = paletteBase | colors[1];
						if (!(scanline[2] & 3) && colors[2]) scanline[2] = paletteBase | colors[2];
						if (!(scanline[3] & 3) && colors[3]) scanline[3] = paletteBase | colors[3];
						if (!(scanline[4] & 3) && colors[4]) scanline[4] = paletteBase | colors[4];
						if (!(scanline[5] & 3) && colors[5]) scanline[5] = paletteBase | colors[5];
						if (!(scanline[6] & 3) && colors[6]) scanline[6] = paletteBase | colors[6];
						if (!(scanline[7] & 3) && colors[7]) scanline[7] = paletteBase | colors[7];
					} else {
						if (colors[0]) scanline[0] = paletteBase | colors[0];
						if (colors[1]) scanline[1] = paletteBase | colors[1];
						if (colors[2]) scanline[2] = paletteBase | colors[2];
						if (colors[3]) scanline[3] = paletteBase | colors[3];
						if (colors[4]) scanline[4] = paletteBase | colors[4];
						if (colors[5]) scanline[5] = paletteBase | colors[5];
						if (colors[6]) scanline[6] = paletteBase | colors[6];
						if (colors[7]) scanline[7] = paletteBase | colors[7];
					}
				}
			}
		}
	}
}

template<class Type> inline void ResolveCGBScanline(void* scanlineStart) {
	// resolve to colors
	if (cgb.dirtyPalette) {
		cgbResolvePalette();
	}

	Type* scanline = (Type*)scanlineStart;
	// middle 160 bytes of linebuffer go into scanline
	for (int i = 7; i < 167; i++) {
		*(scanline++) = ToScanType<Type>(cgb.palette[lineBuffer[i]]);
	}
}

template<class Type> inline void DoubleResolveCGBScanline(void* scanlineStart1, void* scanlineStart2) {
	// resolve to colors
	if (cgb.dirtyPalette) {
		cgbResolvePalette();
	}

	Type* scanline1 = (Type*)scanlineStart1;
	Type* scanline2 = (Type*)scanlineStart2;
	// middle 160 bytes of linebuffer go into scanline
	for (int i = 7; i < 167; i++) {
		*(scanline1++) = ToScanType<Type>(cgb.palette[lineBuffer[i]]);
		*(scanline2++) = ToScanType<Type>(cgb.palette[lineBuffer[i]]);
	}
}