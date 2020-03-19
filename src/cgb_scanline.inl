#pragma once

// Shared between platforms, renders a scanline based on the unsigned short color palette to different types
// (we speed up 2x scaling by directly rendering a scanline to unsigned int)

#include "cgb.h"
#include "cpu.h"
#include "gpu.h"
#include "memory.h"

template<bool priorityBG>
inline bool RenderCGBScanline_BG() {
	bool hasPriority = false;
	int* swapLine;
	int priorityLine[8];

	// background/window
	{
		int i;

		// tile offset and palette shared for window
		const int tileOffset = (cpu.memory.LCDC_ctl & LCDC_TILESET) ? 0 : 256;

		// draw background
		{
			int y = (cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 7;

			int yVal[2];
			yVal[0] = y * 2;
			yVal[1] = (7 - y) * 2;

			int mapOffset = (cpu.memory.LCDC_ctl & LCDC_BGTILEMAP) ? 0x1c00 : 0x1800;
			mapOffset += (((cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5;

			// start to the left of pixel 7 (our leftmost linebuffer pixel) based on scrollx
			int* scanline = (int*)&lineBuffer[7 - (cpu.memory.SCX_bgscrollx & 7)];

			for (i = 0; i < 168; i += 8) {
				int lineOffset = ((unsigned char)(cpu.memory.SCX_bgscrollx + i)) >> 3;

				int attr = vram[mapOffset + lineOffset + 0x2000];
				if (((attr & 0x80) == 0) == priorityBG) {
					// priority background
					if (priorityBG) {
						scanline += 8;
						continue;
					} else {
						hasPriority = true;
					}
				}

				int tile = vram[mapOffset + lineOffset];
				if (!(tile & 0x80)) tile += tileOffset;

				// attribute bit 3 is vram bank number, bit 6 is yflip
				unsigned int tileRow = *((unsigned short*)&vram[tile * 16 + yVal[(attr & 0x40) >> 6] + ((attr & 0x8) << 10)]);
				ShortSwap(tileRow);

				// attribute bit 0-2 is bg palette
				int paletteMask = (attr & 0x07) << 4;

				if (priorityBG) {
					swapLine = scanline;
					scanline = priorityLine;
				}

				// bit 5 is hflip
				if (attr & 0x20) {
					resolveTileRowReversePal(paletteMask, scanline, tileRow);
				} else {
					resolveTileRowPal(paletteMask, scanline, tileRow);
				}

				if (priorityBG) {
					if (scanline[0] & 3) swapLine[0] = scanline[0];
					if (scanline[1] & 3) swapLine[1] = scanline[1];
					if (scanline[2] & 3) swapLine[2] = scanline[2];
					if (scanline[3] & 3) swapLine[3] = scanline[3];
					if (scanline[4] & 3) swapLine[4] = scanline[4];
					if (scanline[5] & 3) swapLine[5] = scanline[5];
					if (scanline[6] & 3) swapLine[6] = scanline[6];
					if (scanline[7] & 3) swapLine[7] = scanline[7];
					scanline = swapLine;
				}

				scanline += 8;
			}
		}

		// draw window
		if (cpu.memory.LCDC_ctl & LCDC_WINDOWENABLE)
		{
			int wx = cpu.memory.WX_windowx;
			int y = cpu.memory.LY_lcdline - cpu.memory.WY_windowy + windowLineOffset;

			if (wx <= 166 && y >= 0) {
				// select map offset row
				int mapOffset = (cpu.memory.LCDC_ctl & LCDC_WINDOWTILEMAP) ? 0x1c00 : 0x1800;
				mapOffset += (y >> 3) << 5;

				int lineOffset = -1;
				y &= 0x07;
				int yVal[2];
				yVal[0] = y * 2;
				yVal[1] = (7 - y) * 2;

				int* scanline = (int*)&lineBuffer[wx];

				for (; wx < 167; wx += 8) {
					lineOffset = lineOffset + 1;

					int attr = vram[mapOffset + lineOffset + 0x2000];
					if (((attr & 0x80) == 0) == priorityBG) {
						if (!priorityBG) {
							hasPriority = true;
						}
					}

					int tile = vram[mapOffset + lineOffset];
					if (!(tile & 0x80)) tile += tileOffset;

					// attribute bit 3 is vram bank number, bit 6 is yflip
					unsigned int tileRow = *((unsigned short*)&vram[tile * 16 + yVal[(attr & 0x40) >> 6] + ((attr & 0x8) << 10)]);
					ShortSwap(tileRow);

					// attribute bit 0-2 is bg palette
					int paletteMask = (attr & 0x07) << 4;

					if (priorityBG) {
						swapLine = scanline;
						scanline = priorityLine;
					}

					// bit 5 is hflip
					if (attr & 0x20) {
						resolveTileRowReversePal(paletteMask, scanline, tileRow);
					} else {
						resolveTileRowPal(paletteMask, scanline, tileRow);
					}

					if (priorityBG) {
						if (scanline[0] & 3) swapLine[0] = scanline[0];
						if (scanline[1] & 3) swapLine[1] = scanline[1];
						if (scanline[2] & 3) swapLine[2] = scanline[2];
						if (scanline[3] & 3) swapLine[3] = scanline[3];
						if (scanline[4] & 3) swapLine[4] = scanline[4];
						if (scanline[5] & 3) swapLine[5] = scanline[5];
						if (scanline[6] & 3) swapLine[6] = scanline[6];
						if (scanline[7] & 3) swapLine[7] = scanline[7];
						scanline = swapLine;
					}

					scanline += 8;
				}
			}
		}
	}

	return hasPriority;
}

inline void RenderCGBScanline() {
	bool hasPriority = RenderCGBScanline_BG<false>();

	// if sprites enabled
	if (cpu.memory.LCDC_ctl & LCDC_SPRITEENABLE)
	{
		int spriteSize;
		int tileMask;
		if (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) {
			spriteSize = 15;
			tileMask = 0xFE;
		} else {
			spriteSize = 7;
			tileMask = 0xFF;
		}

		// BG enable flag for CGB means allowng BG over sprite priority
		bool forceSpritePriority = (cpu.memory.LCDC_ctl & LCDC_BGENABLE);

		sprite_type* sprite = (struct sprite_type *) &oam[156];
		for (int i = 39; i >= 0; i--, sprite--) {

			if (sprite->x && sprite->x < 168) {
				int sy = sprite->y - 16;

				if (sy <= cpu.memory.LY_lcdline && (sy + spriteSize) >= cpu.memory.LY_lcdline) {
					// sprite position
					int* scanline = (int*)&lineBuffer[sprite->x - 1];

					int y;
					int tile = sprite->tile;

					if (OAM_ATTR_YFLIP(sprite->attr)) y = spriteSize - (cpu.memory.LY_lcdline - sy);
					else y = cpu.memory.LY_lcdline - sy;
					tile &= tileMask;

					// bit 3 is vram bank #
					int tileIndex = tile * 16 + y * 2 + (OAM_ATTR_BANK(sprite->attr) << 10);
					unsigned int tileRow = *((unsigned short*)&vram[tileIndex]);
					ShortSwap(tileRow);

					int colors[8];
					if (!OAM_ATTR_XFLIP(sprite->attr)) {
						resolveTileRow(colors, tileRow);
					} else {
						resolveTileRowReverse(colors, tileRow);
					}

					// bit 0-2 are palette #'s for CGB
					int paletteBase = (32 + (OAM_ATTR_PAL_NUM(sprite->attr) << 2)) << 2;

					if (OAM_ATTR_PRIORITY(sprite->attr) && forceSpritePriority) {
						if (!(scanline[0] & 12) && colors[0]) scanline[0] = paletteBase | colors[0];
						if (!(scanline[1] & 12) && colors[1]) scanline[1] = paletteBase | colors[1];
						if (!(scanline[2] & 12) && colors[2]) scanline[2] = paletteBase | colors[2];
						if (!(scanline[3] & 12) && colors[3]) scanline[3] = paletteBase | colors[3];
						if (!(scanline[4] & 12) && colors[4]) scanline[4] = paletteBase | colors[4];
						if (!(scanline[5] & 12) && colors[5]) scanline[5] = paletteBase | colors[5];
						if (!(scanline[6] & 12) && colors[6]) scanline[6] = paletteBase | colors[6];
						if (!(scanline[7] & 12) && colors[7]) scanline[7] = paletteBase | colors[7];
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

	// render priority background tiles
	if (hasPriority) {
		RenderCGBScanline_BG<true>();
	}
}
