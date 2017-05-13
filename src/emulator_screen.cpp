
#include "platform.h"

#include "emulator.h"
#include "debug.h"

#include "rom.h"
#include "cgb_bootstrap.h"
#include "memory.h"
#include "cgb.h"
#include "gpu.h"

#include "../../prizm-zx7/zx7.h"
#include "../../calctype/fonts/arial_small/arial_small.h"

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
		CalcType_Draw(&arial_small, buffer, x, y, COLOR_LIGHTGREEN, 0, 0);
		x = srcX;
		y = srcY - 1;
		CalcType_Draw(&arial_small, buffer, x, y, COLOR_LIGHTGREEN, 0, 0);
		x = srcX + 1;
		y = srcY;
		CalcType_Draw(&arial_small, buffer, x, y, COLOR_SPRINGGREEN, 0, 0);
		x = srcX;
		y = srcY + 1;
		CalcType_Draw(&arial_small, buffer, x, y, COLOR_SPRINGGREEN, 0, 0);
		x = srcX;
		y = srcY;
	}

	CalcType_Draw(&arial_small, buffer, x, y, color, 0, 0);
}

int emulator_screen::PrintWidth(const char* buffer) {
	return CalcType_Width(&arial_small, buffer);
}

void emulator_screen::DrawPausePreview() {

	if (!emulator.pausePreviewValid)
		return;

	// construct palette
	if (!cgb.isCGB) {
		if (!emulator.settings.useCGBColors || !getCGBTableEntry(&memoryMap[0][ROM_OFFSET_NAME], &ppuPalette[12])) {
			colorpalette_type pal;
			emulator.getPalette(emulator.settings.bgColorPalette, pal);
			for (int i = 0; i < 4; i++) {
				ppuPalette[i + 12] = pal.colors[i] | (pal.colors[i] << 16);
			}

			emulator.getPalette(emulator.settings.obj1ColorPalette, pal);
			for (int i = 0; i < 4; i++) {
				ppuPalette[i + 16] = pal.colors[i] | (pal.colors[i] << 16);
			}

			emulator.getPalette(emulator.settings.obj2ColorPalette, pal);
			for (int i = 0; i < 4; i++) {
				ppuPalette[i + 20] = pal.colors[i] | (pal.colors[i] << 16);
			}
		}

		SetupDisplayPalette();
	}

	const int xStart = 300;
	const int yStart = 22;
	unsigned char* pause = &emulator.pausePreview[0];
	unsigned short* vramStart = ((unsigned short*)GetVRAMAddress()) + xStart + yStart * LCD_WIDTH_PX;
	for (int y = 0; y < 72; y++) {
		if (cgb.isCGB) {
			for (int x = 0; x < 80; x++, pause++) {
				unsigned int color = ppuPalette[*pause] & 0xFFFF;
				vramStart[x + y * LCD_WIDTH_PX] = color;
			}
		} else {
			for (int x = 0; x < 80; x++, pause++) {
				unsigned int color1 = ppuPalette[(*pause) & 0x0F] & 0xFFFF;
				unsigned int color2 = ppuPalette[((*pause) & 0xF0) >> 4] & 0xFFFF;
				vramStart[x + y * LCD_WIDTH_PX] = mix565(color1, color2);
			}
		}
	}
}

static const unsigned char* curBGData = 0;
static int overwrittenBits = 0;
void emulator_screen::ResolveBG(const unsigned char* data) {
	unsigned short* vram = (unsigned short*) GetVRAMAddress();

	bool needsLoad = false;
	if (data != curBGData) {
		needsLoad = true;
	}

	// load saved vram (so we can check expected bit consistency early)
	if (!needsLoad) {
		LoadVRAM_1();

		// check for expected low bits in saved ram
		needsLoad = (overwrittenBits == (vram[0] & 0x0003));
	}

	if (needsLoad) {
		DrawBGEmbedded((unsigned char*) data);
		SaveVRAM_1();
	}

	// now we increment the blue channel by a single point in vram directly
	// if this is ever what is loaded, then we know the vram was saved without our permission (OS overwrite)
	overwrittenBits = (vram[0] + 1) & 3;
	vram[0] = overwrittenBits | (vram[0] & 0xFFFC);

	curBGData = data;
	DrawFrame(0);
}