
#include "platform.h"

#include "bmp.h"

void PutBMP(const char* filepath, int atX, int atY, int srcY, int destHeight) {
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

	if (depth != 16 || width > 384) {
		Bfile_CloseFile_OS(file);
		memset(GetVRAMAddress(), 0x00, 2 * LCD_HEIGHT_PX * LCD_WIDTH_PX);
		return;
	}

	if (srcY != 0) {
		Bfile_SeekFile_OS(file, 54 + 2 * width * srcY);
	}

	int endY = height;
	if (destHeight != -1) {
		endY = srcY + destHeight;
	}

	// each row:
	unsigned short color[384];
	for (int y = srcY; y < endY; y++) {
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
			((short*)GetVRAMAddress()) + atX + (y - srcY + atY) * LCD_WIDTH_PX,
			color,
			2 * width);
	}

	Bfile_CloseFile_OS(file);
}