#pragma once

// Shared between platforms, renders a scanline based on the unsigned short color palette to different types
// (we speed up 2x scaling by directly rendering a scanline to unsigned long)

template <class Type>
inline Type ToScanType(unsigned short color) {
	return color;
}

inline unsigned short ToScanType(unsigned short color) {
	return color;
}

inline unsigned int ToScanType(unsigned int color) {
	return color | (color << 16);
}

template<class Type, Type colorXor> inline void RenderScanline(void* scanlineStart) {
	// background/window
	{
		int i;

		// tile offset and palette shared for window
		const int tileOffset = (cpu.memory.LCDC_ctl & LCDC_TILESET) ? 0 : 256;

		const Type palette[4] = {
			ToScanType((Type)colorPalette[(cpu.memory.BGP_bgpalette & 0x03) >> 0]),
			ToScanType((Type)(colorPalette[(cpu.memory.BGP_bgpalette & 0x0C) >> 2] ^ colorXor)),
			ToScanType((Type)(colorPalette[(cpu.memory.BGP_bgpalette & 0x30) >> 4] ^ colorXor)),
			ToScanType((Type)(colorPalette[(cpu.memory.BGP_bgpalette & 0xC0) >> 6] ^ colorXor))
		};

		// draw background
		if (cpu.memory.LCDC_ctl & LCDC_BGENABLE)
		{
			int x = 7 - (cpu.memory.SCX_bgscrollx & 7);
			int y = (cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 7;

			int mapOffset = (cpu.memory.LCDC_ctl & LCDC_BGTILEMAP) ? 0x1c00 : 0x1800;
			mapOffset += (((cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5;

			// finish/draw left tile
			int lineOffset = (cpu.memory.SCX_bgscrollx >> 3);
			unsigned short tile = (unsigned short)vram[mapOffset + lineOffset];
			if (!(tile & 0x80)) tile += tileOffset;
			unsigned short tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
			EndianSwap(tileRow);

			Type* scanline = (Type*)scanlineStart;

			for (i = 0; x >= 0; i++, x--) {
				*(scanline++) = palette[((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1)];
			}
			for (; i < 153; i += 8) {
				lineOffset = (lineOffset + 1) & 31;
				tile = vram[mapOffset + lineOffset];
				if (!(tile & 0x80)) tile += tileOffset;
				tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
				EndianSwap(tileRow);

				scanline[0] = palette[((tileRow >> 15) & 1) | ((tileRow >> 6) & 2)];
				scanline[1] = palette[((tileRow >> 14) & 1) | ((tileRow >> 5) & 2)];
				scanline[2] = palette[((tileRow >> 13) & 1) | ((tileRow >> 4) & 2)];
				scanline[3] = palette[((tileRow >> 12) & 1) | ((tileRow >> 3) & 2)];
				scanline[4] = palette[((tileRow >> 11) & 1) | ((tileRow >> 2) & 2)];
				scanline[5] = palette[((tileRow >> 10) & 1) | ((tileRow >> 1) & 2)];
				scanline[6] = palette[((tileRow >> 9) & 1)  | ((tileRow >> 0) & 2)];
				scanline[7] = palette[((tileRow >> 8) & 1)  | ((tileRow << 1) & 2)];
				scanline += 8;
			}
			x = 7;
			lineOffset = (lineOffset + 1) & 31;
			tile = vram[mapOffset + lineOffset];
			if (!(tile & 0x80)) tile += tileOffset;
			tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
			EndianSwap(tileRow);
			for (; i < 160; i++, x--) {
				*(scanline++) = palette[((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1)];
			}
		}

		// draw window
		if (cpu.memory.LCDC_ctl & LCDC_WINDOWENABLE)
		{
			int wx = cpu.memory.WX_windowx - 7;
			int y = cpu.memory.LY_lcdline - cpu.memory.WY_windowy;

			if (wx >= -7 && wx <= 159 && y >= 0) {
				// select map offset row
				int mapOffset = (cpu.memory.LCDC_ctl & LCDC_WINDOWTILEMAP) ? 0x1c00 : 0x1800;
				mapOffset += (y >> 3) << 5;

				int lineOffset = 0;
				y &= 0x07;
				unsigned short tile = (unsigned short)vram[mapOffset + lineOffset];
				if (!(tile & 0x80)) tile += tileOffset;
				unsigned short tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
				EndianSwap(tileRow);

				Type* scanline = (Type*)scanlineStart + wx;

				int x = 7;
				for (; wx < 0; wx++, x--, scanline++) {
				}
				for (; x >= 0 && wx < 160; wx++, x--) {
					*(scanline++) = palette[((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1)];
				}
				for (; wx < 160; wx += 8) {
					lineOffset = (lineOffset + 1) & 31;
					tile = vram[mapOffset + lineOffset];
					if (!(tile & 0x80)) tile += tileOffset;
					tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
					EndianSwap(tileRow);

					scanline[0] = palette[((tileRow >> 15) & 1) | ((tileRow >> 6) & 2)];
					scanline[1] = palette[((tileRow >> 14) & 1) | ((tileRow >> 5) & 2)];
					scanline[2] = palette[((tileRow >> 13) & 1) | ((tileRow >> 4) & 2)];
					scanline[3] = palette[((tileRow >> 12) & 1) | ((tileRow >> 3) & 2)];
					scanline[4] = palette[((tileRow >> 11) & 1) | ((tileRow >> 2) & 2)];
					scanline[5] = palette[((tileRow >> 10) & 1) | ((tileRow >> 1) & 2)];
					scanline[6] = palette[((tileRow >> 9) & 1) | ((tileRow >> 0) & 2)];
					scanline[7] = palette[((tileRow >> 8) & 1) | ((tileRow << 1) & 2)];
					scanline += 8;
				}
				x = 7;
				lineOffset = (lineOffset + 1) & 31;
				tile = vram[mapOffset + lineOffset];
				if (!(tile & 0x80)) tile += tileOffset;
				tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
				EndianSwap(tileRow);
				for (; wx < 160; wx++, x--) {
					*(scanline++) = palette[((tileRow >> (x + 8)) & 1) | (((tileRow >> x)& 1) << 1)];
				}
			}
		}
	}

	Type* scanline = (Type*)scanlineStart;

	// if sprites enabled
	if (cpu.memory.LCDC_ctl & LCDC_SPRITEENABLE)
	{
		const Type palette[8] = {
			ToScanType((Type) colorPalette[4+((cpu.memory.OBP0_spritepal0 & 0x03) >> 0)]),
			ToScanType((Type) colorPalette[4+((cpu.memory.OBP0_spritepal0 & 0x0C) >> 2)]),
			ToScanType((Type) colorPalette[4+((cpu.memory.OBP0_spritepal0 & 0x30) >> 4)]),
			ToScanType((Type) colorPalette[4+((cpu.memory.OBP0_spritepal0 & 0xC0) >> 6)]),
			ToScanType((Type) colorPalette[8+((cpu.memory.OBP1_spritepal1 & 0x03) >> 0)]),
			ToScanType((Type) colorPalette[8+((cpu.memory.OBP1_spritepal1 & 0x0C) >> 2)]),
			ToScanType((Type) colorPalette[8+((cpu.memory.OBP1_spritepal1 & 0x30) >> 4)]),
			ToScanType((Type) colorPalette[8+((cpu.memory.OBP1_spritepal1 & 0xC0) >> 6)]),
		};

		Type bgColor = ToScanType((Type)colorPalette[cpu.memory.BGP_bgpalette & 0x03]);

		int spriteSize = (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) ? 16 : 8;

		for (int i = 0; i < 40; i++) {
			const sprite& sprite = ((struct sprite *)oam)[39-i];

			if (sprite.x) {
				int sy = sprite.y - 16;

				if (sy <= cpu.memory.LY_lcdline && (sy + spriteSize) > cpu.memory.LY_lcdline) {
					int sx = sprite.x - 8;

					int pixelOffset = sx;

					unsigned char y;
					int tile = sprite.tile;
					if (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) {
						if (OAM_ATTR_YFLIP(sprite.attr)) y = 15 - (cpu.memory.LY_lcdline - sy);
						else y = cpu.memory.LY_lcdline - sy;
						tile &= 0xFE;
					} else {
						if (OAM_ATTR_YFLIP(sprite.attr)) y = 7 - (cpu.memory.LY_lcdline - sy);
						else y = cpu.memory.LY_lcdline - sy;
					}

					unsigned char paletteBase = OAM_ATTR_PALETTE(sprite.attr) ? 4 : 0;
					unsigned short tileRow = *((unsigned short*)&vram[tile * 16 + y * 2]);
					EndianSwap(tileRow);

					int x;
					if (OAM_ATTR_PRIORITY(sprite.attr)) {
						if (!OAM_ATTR_XFLIP(sprite.attr)) {
							for (x = 7; x >= 0; x--, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == bgColor) {
									unsigned char colour = ((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1);

									if (colour) {
										scanline[pixelOffset] = palette[paletteBase + colour];
									}
								}
							}
						}
						else {
							for (x = 0; x < 8; x++, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == bgColor) {
									unsigned char colour = ((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1);

									if (colour) {
										scanline[pixelOffset] = palette[paletteBase + colour];
									}
								}
							}
						}
					}
					else {
						if (!OAM_ATTR_XFLIP(sprite.attr)) {
							for (x = 7; x >= 0; x--, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160) {
									unsigned char colour = ((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1);

									if (colour) {
										scanline[pixelOffset] = palette[paletteBase + colour];
									}
								}
							}
						}
						else {
							for (x = 0; x < 8; x++, pixelOffset++) {
								if (pixelOffset >= 0 && pixelOffset < 160) {
									unsigned char colour = ((tileRow >> (x + 8)) & 1) | (((tileRow >> x) & 1) << 1);

									if (colour) {
										scanline[pixelOffset] = palette[paletteBase + colour];
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