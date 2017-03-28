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

template<class Type> inline void RenderScanline(void* scanlineStart) {
	// background/window
	{
		int i;

		// tile offset and palette shared for window
		const int tileOffset = (cpu.memory.LCDC_ctl & LCDC_TILESET) ? 0 : 256;

		const Type palette[4] = {
			ToScanType((Type)colorPalette[backgroundPalette[0]]),
			ToScanType((Type)colorPalette[backgroundPalette[1]]),
			ToScanType((Type)colorPalette[backgroundPalette[2]]),
			ToScanType((Type)colorPalette[backgroundPalette[3]])
		};

		// draw background
		if (cpu.memory.LCDC_ctl & LCDC_BGENABLE)
		{
			int x = cpu.memory.SCX_bgscrollx & 7;
			int y = (cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 7;

			int mapOffset = (cpu.memory.LCDC_ctl & LCDC_BGTILEMAP) ? 0x1c00 : 0x1800;
			mapOffset += (((cpu.memory.LY_lcdline + cpu.memory.SCY_bgscrolly) & 255) >> 3) << 5;

			// finish/draw left tile
			int lineOffset = (cpu.memory.SCX_bgscrollx >> 3);
			unsigned short tile = (unsigned short)vram[mapOffset + lineOffset];
			if (!(tile & 0x80)) tile += tileOffset;
			const unsigned char* tileRow = tiles->data[tile][y];

			Type* scanline = (Type*)scanlineStart;

			for (i = 0; i < 160; i++) {
				// stretch crashing here for some reason
				scanline[i] = palette[tileRow[x]];

				x++;
				if (x == 8) {
					x = 0;
					lineOffset = (lineOffset + 1) & 31;
					tile = vram[mapOffset + lineOffset];
					if (!(tile & 0x80)) tile += tileOffset;
					tileRow = tiles->data[tile][y];
				}
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
				const unsigned char* tileRow = tiles->data[tile][y];

				Type* scanline = (Type*)scanlineStart;

				int x = 0;
				for (; wx < 0; wx++, x++) {
				}
				for (; wx < 160; wx++) {
					// stretch crashing here for some reason
					scanline[wx] = palette[tileRow[x]];

					x++;
					if (x == 8) {
						x = 0;
						lineOffset = (lineOffset + 1) & 31;
						tile = vram[mapOffset + lineOffset];
						if (!(tile & 0x80)) tile += tileOffset;
						tileRow = tiles->data[tile][y];
					}
				}
			}
		}
	}

	Type* scanline = (Type*)scanlineStart;

	// if sprites enabled
	{
		const Type palette[8] = {
			ToScanType((Type) colorPalette[4+spritePalette[0][0]]),
			ToScanType((Type) colorPalette[4+spritePalette[0][1]]),
			ToScanType((Type) colorPalette[4+spritePalette[0][2]]),
			ToScanType((Type) colorPalette[4+spritePalette[0][3]]),
			ToScanType((Type) colorPalette[8+spritePalette[1][0]]),
			ToScanType((Type) colorPalette[8+spritePalette[1][1]]),
			ToScanType((Type) colorPalette[8+spritePalette[1][2]]),
			ToScanType((Type) colorPalette[8+spritePalette[1][3]]),
		};

		Type bgColor = ToScanType((Type)colorPalette[backgroundPalette[0]]);

		if (cpu.memory.LCDC_ctl & LCDC_SPRITEVDOUBLE) {
			for (int i = 0; i < 40; i++) {
				const sprite& sprite = ((struct sprite *)oam)[i];

				if (sprite.x) {
					int sy = sprite.y - 16;

					if (sy <= cpu.memory.LY_lcdline && (sy + 16) > cpu.memory.LY_lcdline) {
						int sx = sprite.x - 8;

						int pixelOffset = sx;

						unsigned char tileRow;
						if (OAM_ATTR_YFLIP(sprite.attr)) tileRow = 15 - (cpu.memory.LY_lcdline - sy);
						else tileRow = cpu.memory.LY_lcdline - sy;

						unsigned char paletteBase = OAM_ATTR_PALETTE(sprite.attr) ? 4 : 0;

						int tile = (tileRow < 8) ? (sprite.tile & 0xFE) : (sprite.tile | 0x01);
						int x;
						if (OAM_ATTR_PRIORITY(sprite.attr)) {
							if (OAM_ATTR_XFLIP(sprite.attr)) {
								for (x = 7; x >= 0; x--, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == bgColor) {
										unsigned char colour = tiles->data[tile][tileRow & 0x07][x];

										if (colour) {
											scanline[pixelOffset] = palette[paletteBase + colour];
										}
									}
								}
							}
							else {
								for (x = 0; x < 8; x++, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == bgColor) {
										unsigned char colour = tiles->data[tile][tileRow & 0x07][x];

										if (colour) {
											scanline[pixelOffset] = palette[paletteBase + colour];
										}
									}
								}
							}
						}
						else {
							if (OAM_ATTR_XFLIP(sprite.attr)) {
								for (x = 7; x >= 0; x--, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160) {
										unsigned char colour = tiles->data[tile][tileRow & 0x07][x];

										if (colour) {
											scanline[pixelOffset] = palette[paletteBase + colour];
										}
									}
								}
							}
							else {
								for (x = 0; x < 8; x++, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160) {
										unsigned char colour = tiles->data[tile][tileRow & 0x07][x];

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
		} else {
			for (int i = 0; i < 40; i++) {
				const sprite& sprite = ((struct sprite *)oam)[i];

				if (sprite.x) {
					int sy = sprite.y - 16;

					if (sy <= cpu.memory.LY_lcdline && (sy + 8) > cpu.memory.LY_lcdline) {
						int sx = sprite.x - 8;

						int pixelOffset = sx;

						unsigned char tileRow;
						if (OAM_ATTR_YFLIP(sprite.attr)) tileRow = 7 - (cpu.memory.LY_lcdline - sy);
						else tileRow = cpu.memory.LY_lcdline - sy;

						unsigned char paletteBase = OAM_ATTR_PALETTE(sprite.attr) ? 4 : 0;

						int x;
						if (OAM_ATTR_PRIORITY(sprite.attr)) {
							if (OAM_ATTR_XFLIP(sprite.attr)) {
								for (x = 7; x >= 0; x--, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == bgColor) {
										unsigned char colour = tiles->data[sprite.tile][tileRow][x];

										if (colour) {
											scanline[pixelOffset] = palette[paletteBase + colour];
										}
									}
								}
							}
							else {
								for (x = 0; x < 8; x++, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160 && scanline[pixelOffset] == bgColor) {
										unsigned char colour = tiles->data[sprite.tile][tileRow][x];

										if (colour) {
											scanline[pixelOffset] = palette[paletteBase + colour];
										}
									}
								}
							}
						}
						else {
							if (OAM_ATTR_XFLIP(sprite.attr)) {
								for (x = 7; x >= 0; x--, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160) {
										unsigned char colour = tiles->data[sprite.tile][tileRow][x];

										if (colour) {
											scanline[pixelOffset] = palette[paletteBase + colour];
										}
									}
								}
							}
							else {
								for (x = 0; x < 8; x++, pixelOffset++) {
									if (pixelOffset >= 0 && pixelOffset < 160) {
										unsigned char colour = tiles->data[sprite.tile][tileRow][x];

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
}