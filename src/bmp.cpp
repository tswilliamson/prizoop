
#include "platform.h"

#include "bmp.h"
#include "emulator.h"
#include "debug.h"

void PutBMP(const char* filepath, int x1, int y1, int x2, int y2) {
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

	if (y1 != 0) {
		Bfile_SeekFile_OS(file, 54 + 2 * width * y1);
	}

	// each row:
	unsigned short color[384];
	for (int y = y1; y < y2; y++) {
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
			((short*)GetVRAMAddress()) + x1 + y * LCD_WIDTH_PX,
			color,
			2 * (x2 - x1));
	}

	Bfile_CloseFile_OS(file);

	DrawFrame(0);
}

void emulator_screen::DrawBG(const char* filename, int x1, int y1, int x2, int y2) {
	PutBMP(filename, x1, y1, x2, y2);
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