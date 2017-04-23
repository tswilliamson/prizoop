#pragma once

inline void RenderDMGScanline() {
	int curLine = cpu.memory.LY_lcdline;

	// background/window
	{
		int i;

		// tile offset and palette shared for window
		const int tileOffset = ((~cpu.memory.LCDC_ctl) & LCDC_TILESET) << 4;

		// draw background
		if (cpu.memory.LCDC_ctl & LCDC_BGENABLE)
		{
			int y = ((curLine + cpu.memory.SCY_bgscrolly) & 7) * 2;

			int mapOffset = ((cpu.memory.LCDC_ctl & LCDC_BGTILEMAP) ? 0x1c00 : 0x1800) +
						    ((((curLine + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5);

			// start to the left of pixel 7 (our leftmost linebuffer pixel) based on scrollx
			int* scanline = (int*) &lineBuffer[7 - (cpu.memory.SCX_bgscrollx & 7)];

			int lineOffset = cpu.memory.SCX_bgscrollx >> 3;

			for (i = 0; i < 168; i += 8) {
				int tile = vram[mapOffset + lineOffset];
				if (!(tile & 0x80)) tile += tileOffset;
				unsigned int tileRow = *((unsigned short*)&vram[tile * 16 + y]);
				ShortSwap(tileRow);

				scanline[0] = (tileRow >> 15) | ((tileRow >> 6) & 2);
				scanline[1] = ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
				scanline[2] = ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
				scanline[3] = ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
				scanline[4] = ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
				scanline[5] = ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
				scanline[6] = ((tileRow >> 9) & 1) | ((tileRow >> 0) & 2);
				scanline[7] = ((tileRow >> 8) & 1) | ((tileRow << 1) & 2);
				scanline += 8;
				lineOffset = (lineOffset + 1) & 0x1F;
			}
		}

		// draw window
		if (cpu.memory.LCDC_ctl & LCDC_WINDOWENABLE)
		{
			int wx = cpu.memory.WX_windowx;
			int y = curLine - cpu.memory.WY_windowy;

			if (wx <= 166 && y >= 0) {
				// select map offset row
				int mapOffset = (cpu.memory.LCDC_ctl & LCDC_WINDOWTILEMAP) ? 0x1c00 : 0x1800;
				mapOffset += (y >> 3) << 5;

				int lineOffset = -1;
				y = (y & 0x07) * 2;

				int* scanline = (int*) &lineBuffer[wx];

				for (; wx < 167; wx += 8) {
					lineOffset = lineOffset + 1;
					int tile = vram[mapOffset + lineOffset];
					if (!(tile & 0x80)) tile += tileOffset;
					unsigned int tileRow = *((unsigned short*)&vram[tile * 16 + y]);
					ShortSwap(tileRow);

					scanline[0] = ((tileRow >> 15) & 1) | ((tileRow >> 6) & 2);
					scanline[1] = ((tileRow >> 14) & 1) | ((tileRow >> 5) & 2);
					scanline[2] = ((tileRow >> 13) & 1) | ((tileRow >> 4) & 2);
					scanline[3] = ((tileRow >> 12) & 1) | ((tileRow >> 3) & 2);
					scanline[4] = ((tileRow >> 11) & 1) | ((tileRow >> 2) & 2);
					scanline[5] = ((tileRow >> 10) & 1) | ((tileRow >> 1) & 2);
					scanline[6] = ((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2);
					scanline[7] = ((tileRow >> 8) & 1)  | ((tileRow << 1) & 2);
					scanline += 8;
				}
			}
		}
	}

	// if sprites enabled
	if (cpu.memory.LCDC_ctl & LCDC_SPRITEENABLE)
	{
		int spriteSize;
		int tileMask;
		if (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) {
			spriteSize = 16;
			tileMask = 0xFE;
		} else {
			spriteSize = 8;
			tileMask = 0xFF;
		}

		const sprite_type* sprite = (const sprite_type*)(&oam[156]);
		for (int i = 39; i >= 0; i--, sprite--) {
			if (sprite->x && sprite->x < 168) {
				int sy = sprite->y - 16;

				if (sy <= curLine && (sy + spriteSize) > curLine) {

					int y;
					int tile = sprite->tile & tileMask;
					if (OAM_ATTR_YFLIP(sprite->attr)) y = spriteSize - 1 - (curLine - sy);
					else y = curLine - sy;

					// determine tile row colors
					unsigned int tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
					ShortSwap(tileRow);

					int colors[8];
					if (!OAM_ATTR_XFLIP(sprite->attr)) {
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

					// sprite position on scanline
					int* scanline = (int*)&lineBuffer[sprite->x - 1];
					int paletteBase = OAM_ATTR_PALETTE(sprite->attr) ? 8 : 4;
					if (OAM_ATTR_PRIORITY(sprite->attr)) {
						if (!scanline[0] && colors[0]) scanline[0] = paletteBase | colors[0];
						if (!scanline[1] && colors[1]) scanline[1] = paletteBase | colors[1];
						if (!scanline[2] && colors[2]) scanline[2] = paletteBase | colors[2];
						if (!scanline[3] && colors[3]) scanline[3] = paletteBase | colors[3];
						if (!scanline[4] && colors[4]) scanline[4] = paletteBase | colors[4];
						if (!scanline[5] && colors[5]) scanline[5] = paletteBase | colors[5];
						if (!scanline[6] && colors[6]) scanline[6] = paletteBase | colors[6];
						if (!scanline[7] && colors[7]) scanline[7] = paletteBase | colors[7];
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