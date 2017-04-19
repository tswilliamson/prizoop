
#include "platform.h"

#include "emulator.h"
#include "debug.h"

#include "rom.h"
#include "cgb_bootstrap.h"
#include "memory.h"
#include "cgb.h"

#include "zx7/zx7.h"

void emulator_screen::DrawBG(const char* filepath) {
	// try to load the file
	unsigned short fileAsName[512];
	Bfile_StrToName_ncpy(fileAsName, filepath, strlen(filepath) + 1);

	int file = Bfile_OpenFile_OS(fileAsName, READ, 0);
	if (file < 0) {
		memset(GetVRAMAddress(), 0x00, 2 * LCD_HEIGHT_PX * LCD_WIDTH_PX);
		return;
	}

	unsigned char header[54];
	Bfile_ReadFile_OS(file, header, 54, -1); // read the 54-byte header

											 // extract image height and width from header
	signed short width = header[18] + 256 * header[19];
	signed short height = header[22] + 256 * header[23];
	signed short depth = header[28] + 256 * header[29];
	if (height < 0) height = -height;

	// currently only support full screen images
	DebugAssert(width == 384 && height == 216);

	if (depth != 16) {
		Bfile_CloseFile_OS(file);
		memset(GetVRAMAddress(), 0x00, 2 * LCD_HEIGHT_PX * LCD_WIDTH_PX);
		return;
	}

	// each row:
	unsigned short color[384];
	for (int y = 0; y < 216; y++) {
		Bfile_ReadFile_OS(file, color, 2 * width, -1);
		// assuming format is endian flipped BGR:
#ifdef BIG_E
		for (int x = 0; x < width; x++) {
			color[x] = ((color[x] & 0xFF00) >> 8) | ((color[x] & 0x00FF) << 8);
		}
#endif
		for (int x = 0; x < width; x++) {
			color[x] =
				((color[x] & 0x001F) << 0) |
				((color[x] & 0x03E0) << 1) |
				((color[x] & 0x7C00) << 1);
		}
		memcpy(
			((short*)GetVRAMAddress()) + y * LCD_WIDTH_PX,
			color,
			2 * 384);
	}

	Bfile_CloseFile_OS(file);

#if TARGET_WINSIM
	unsigned char* outData;
	int compressedSize = ZX7Compress((unsigned char*)GetVRAMAddress(), 384 * 216 * 2, &outData);
	
	// output embedded code to file
	OutputLog("const unsigned char %s[] =\n", filepath);
	OutputLog("{\n\t");
	for (int i = 0; i < compressedSize; i++) {
		OutputLog("%d,", outData[i]);
		if (i % 64 == 63) {
			OutputLog("\n\t");
		}
	}
	OutputLog("};\n");

	free(outData);
#endif

	DrawFrame(0);
}

void emulator_screen::DrawBGEmbedded(unsigned char* compressedData) {
	ZX7Decompress(compressedData, (unsigned char*) GetVRAMAddress(), 384 * 216 * 2);

#ifdef BIG_E
	unsigned short* data = (unsigned short*)GetVRAMAddress();
	for (int x = 0; x < 384*216; x++) {
		*data = ((*data & 0xFF00) >> 8) | ((*data & 0x00FF) << 8);
		data++;
	}
#endif
}

void emulator_screen::Print(int x, int y, const char* buffer, bool selected, unsigned short color) {
	int srcX = x;
	if (selected) {
		int srcY = y;

		x--;
		PrintMini(&x, &y, buffer, 0x42, 0xffffffff, 0, 0, COLOR_LIGHTGREEN, COLOR_BLACK, 1, 0);
		x = srcX;
		y = srcY - 1;
		PrintMini(&x, &y, buffer, 0x42, 0xffffffff, 0, 0, COLOR_LIGHTGREEN, COLOR_BLACK, 1, 0);
		x = srcX + 1;
		y = srcY;
		PrintMini(&x, &y, buffer, 0x42, 0xffffffff, 0, 0, COLOR_SPRINGGREEN, COLOR_BLACK, 1, 0);
		x = srcX;
		y = srcY + 1;
		PrintMini(&x, &y, buffer, 0x42, 0xffffffff, 0, 0, COLOR_SPRINGGREEN, COLOR_BLACK, 1, 0);
		x = srcX;
		y = srcY;
	}

	PrintMini(&x, &y, buffer, 0x42, 0xffffffff, 0, 0, color, 0x0001, 1, 0);
}

int emulator_screen::PrintWidth(const char* buffer) {
	int x = 0, y = 0;
	PrintMini(&x, &y, buffer, 0x42, 0xffffffff, 0, 0, COLOR_BLACK, COLOR_BLACK, 0, 0);
	return x;
}

void emulator_screen::DrawPausePreview() {

	if (!emulator.pausePreviewValid)
		return;

	// construct palette
	unsigned int palette[64];
	if (cgb.isCGB) {
		memcpy(palette, cgb.palette, sizeof(palette));
	} else {
		if (!emulator.settings.useCGBColors || !getCGBTableEntry(&memoryMap[0][ROM_OFFSET_NAME], palette)) {
			colorpalette_type pal;
			emulator.getPalette(emulator.settings.bgColorPalette, pal);
			for (int i = 0; i < 4; i++) {
				palette[i] = pal.colors[i] | (pal.colors[i] << 16);
			}

			emulator.getPalette(emulator.settings.obj1ColorPalette, pal);
			for (int i = 0; i < 4; i++) {
				palette[i + 4] = pal.colors[i] | (pal.colors[i] << 16);
			}

			emulator.getPalette(emulator.settings.obj2ColorPalette, pal);
			for (int i = 0; i < 4; i++) {
				palette[i + 8] = pal.colors[i] | (pal.colors[i] << 16);
			}
		}
	}

	const int xStart = 300;
	const int yStart = 22;
	unsigned char* pause = &emulator.pausePreview[0];
	unsigned short* vramStart = ((unsigned short*)GetVRAMAddress()) + xStart + yStart * LCD_WIDTH_PX;
	for (int y = 0; y < 72; y++) {
		for (int x = 0; x < 80; x++, pause++) {
			unsigned int color = palette[*pause] & 0xFFFF;
			vramStart[x + y * LCD_WIDTH_PX] = color;
		}
	}
}