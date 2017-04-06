
#include "emulator.h"
#include "memory.h"

#include "screen_rom.h"
#include "screen_settings.h"
#include "screen_play.h"

emulator_type emulator;

// Main emulator functionality
void emulator_type::startUp() {
	curScreen = 0;

	if (!loadSettings()) {
		defaultSettings();
	}

	// set up each menu screen
	memset(screens, 0, sizeof(screens));
	screens[0] = new screen_rom;
	screens[1] = new screen_settings;
	screens[2] = new screen_play;

	screens[0]->fKey = 1;
	screens[1]->fKey = 2;
	screens[2]->fKey = 6;

	for (int i = 0; i < 6; i++) {
		if (screens[i]) {
			screens[i]->setup();
		}
	}

	curScreen = 0;
}

void emulator_type::shutDown() {
	screens[curScreen]->deselect();
}

void emulator_type::run() {
	screens[curScreen]->select();

	do {
		int key;
		GetKey(&key);

		switch (key) {
			case KEY_CTRL_LEFT:
				screens[curScreen]->handleLeft();
				break;
			case KEY_CTRL_RIGHT:
				screens[curScreen]->handleRight();
				break;
			case KEY_CTRL_UP:
				screens[curScreen]->handleUp();
				break;
			case KEY_CTRL_DOWN:
				screens[curScreen]->handleDown();
				break;
			case KEY_CTRL_SHIFT:
			case KEY_CTRL_EXE:
				screens[curScreen]->handleSelect();
				break;
			case KEY_CTRL_F1:
				tryScreenChange(1);
				break;
			case KEY_CTRL_F2:
				tryScreenChange(2);
				break;
			case KEY_CTRL_F3:
				tryScreenChange(3);
				break;
			case KEY_CTRL_F4:
				tryScreenChange(4);
				break;
			case KEY_CTRL_F5:
				tryScreenChange(5);
				break;
			case KEY_CTRL_F6:
				tryScreenChange(6);
				break;
		}
	} while (1);
}

void emulator_type::tryScreenChange(int targetFKey) {
	if (targetFKey == screens[curScreen]->fKey)
		return;

	for (int i = 0; i < 6; i++) {
		if (screens[i] && screens[i]->fKey == targetFKey) {
			screens[curScreen]->deselect();
			curScreen = i;
			screens[curScreen]->select();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Custom color palettes

static const char* settingsDir = "Prizoop";
static const char* settingsFile = "Settings";

void emulator_type::defaultSettings() {
	// default settings
	settings.selectedRom[0] = 0;
	settings.overclock = false;
	settings.scaleToScreen = true;
	settings.frameSkip = 1;

	settings.clampSpeed = true;
	settings.useCGBColors = true;

	settings.keyMap[emu_button::A] = 78;			// SHIFT
	settings.keyMap[emu_button::B] = 68;			// OPTN
	settings.keyMap[emu_button::SELECT] = 39;		// F5
	settings.keyMap[emu_button::START] = 29;		// F6
	settings.keyMap[emu_button::RIGHT] = 27;
	settings.keyMap[emu_button::LEFT] = 38;
	settings.keyMap[emu_button::UP] = 28;
	settings.keyMap[emu_button::DOWN] = 37;
	settings.keyMap[emu_button::STATE_SAVE] = 53;	// 'S'
	settings.keyMap[emu_button::STATE_LOAD] = 25;   // 'L'

	// simulator only defaults
#if TARGET_WINSIM
	settings.frameSkip = 0;
	settings.keyMap[emu_button::STATE_SAVE] = 59;	// maps to F3
	settings.keyMap[emu_button::STATE_LOAD] = 49;	// maps to F4
#endif
}

bool emulator_type::loadSettings() {
	int len;
	if (!MCSGetDlen2((unsigned char*) settingsDir, (unsigned char*) settingsFile, &len) && len == sizeof(emulator.settings)) {
		if (!MCSGetData1(0, sizeof(emulator.settings), &emulator.settings)) {
			return true;
		}
	}
	return false;
}

void emulator_type::saveSettings() {
	MCS_CreateDirectory((unsigned char*) settingsDir);
	MCS_WriteItem((unsigned char*) settingsDir, (unsigned char*) settingsFile, 0, sizeof(emulator.settings), (int)(&emulator.settings));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Custom color palettes

static colorpalette_type palettes[] = {
		{ { COLOR_LIGHTCYAN,	COLOR_CYAN,				COLOR_DARKCYAN,		COLOR_BLACK		} },
		{ { COLOR_WHITE,		COLOR_CYAN,				COLOR_MEDIUMBLUE,	COLOR_BLACK		} },
		{ { COLOR_WHITE,		COLOR_LIGHTGRAY,		COLOR_DARKGRAY,		COLOR_BLACK		} },
		{ { COLOR_WHITE,		COLOR_LIGHTGRAY,		COLOR_SLATEGRAY,	COLOR_BLACK		} },
		{ { COLOR_LIGHTGREEN, COLOR_MEDIUMSEAGREEN,	COLOR_DARKGREEN,	COLOR_BLACK			} },
		{ { COLOR_WHITE,		COLOR_MEDIUMSEAGREEN,	COLOR_DARKGREEN,	COLOR_BLACK		} },
		{ { COLOR_WHITE,		COLOR_YELLOW,			COLOR_SANDYBROWN,	COLOR_BROWN		} },
		{ { COLOR_WHITE,		COLOR_ORANGE,			COLOR_RED,			COLOR_BLACK		} },
		{ { COLOR_LIGHTCYAN,	COLOR_CYAN,				COLOR_DARKCYAN,		COLOR_BLACK		} },
		{ { COLOR_WHITE,		COLOR_YELLOW,			COLOR_ORANGE,		COLOR_DARKRED	} },
};

unsigned char emulator_type::numPalettes() {
	return sizeof(palettes) / sizeof(colorpalette_type);
}

void emulator_type::getPalette(unsigned char paletteNum, colorpalette_type& intoColors) {
	memcpy(&intoColors, &palettes[paletteNum], sizeof(colorpalette_type));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Save states

void fillSaveStatePath(unsigned short* pFile) {
	// replace .gb with .prz
	char filepath[256];
	strcpy(filepath, "\\\\fls0\\");
	strcat(filepath, emulator.settings.selectedRom);
	filepath[strlen(filepath) - 2] = 0;
	strcat(filepath, "prz");

	Bfile_StrToName_ncpy(pFile, (const char*)filepath, strlen(filepath) + 2);
}

int getSaveStateSize(unsigned int& withRAMSize) {
	int spaceNeeded = 2 + sizeof(cpu_type) + sizeof(mbc_state);			// main types
	spaceNeeded += sizeof(wram) + sizeof(oam) + sizeof(vram);			// various work RAMS

	withRAMSize = getRAMSize();
	// we only save state RAMS <= 8 KB to save space
	if (withRAMSize <= 8 * 1024) {
		spaceNeeded += getRAMSize();										// RAM
	}

	// 4 byte align
	if (spaceNeeded % 4 != 0) {
		spaceNeeded += 4 - (spaceNeeded % 4);
	}

	return spaceNeeded;
}

void emulator_type::saveState() {
#if !TARGET_WINSIM
	// flush DMA before making OS calls
	DmaWaitNext();
#endif

	// calculate space needed for state
	unsigned int RAMSize;
	int spaceNeeded = getSaveStateSize(RAMSize);

	unsigned short pFile[256];
	fillSaveStatePath(pFile);

	int hFile = Bfile_OpenFile_OS(pFile, WRITE, 0); // Get handle
	if (hFile < 0) {
		// attempt to create if it doesn't exist
		if (Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, (size_t*)&spaceNeeded))
			return;

		hFile = Bfile_OpenFile_OS(pFile, WRITE, 0); // Get handle
		if (hFile < 0) {
			// create didn't work!
			return;
		}
	}

	// write rom bytes 0x14E-F (checksum) for sanity
	Bfile_WriteFile_OS(hFile, &cart[0x14E], 2);

	// write cpu and mbc state
	Bfile_WriteFile_OS(hFile, &cpu, sizeof(cpu_type));
	Bfile_WriteFile_OS(hFile, &mbc, sizeof(mbc_state));

	// write various work rams
	Bfile_WriteFile_OS(hFile, &wram[0], sizeof(wram));
	Bfile_WriteFile_OS(hFile, &vram[0], sizeof(vram));
	Bfile_WriteFile_OS(hFile, &oam[0], sizeof(oam));

	// only write sram for small ram sizes
	if (RAMSize <= 8 * 1024) {
		Bfile_WriteFile_OS(hFile, &sram[0], RAMSize);
	}

	// done!
	Bfile_CloseFile_OS(hFile);
}

bool emulator_type::loadState() {
#if !TARGET_WINSIM
	// flush DMA before making OS calls
	DmaWaitNext();
#endif

	// calculate space needed for state
	unsigned int RAMSize;
	int spaceNeeded = getSaveStateSize(RAMSize);

	unsigned short pFile[256];
	fillSaveStatePath(pFile);

	int hFile = Bfile_OpenFile_OS(pFile, READ, 0); // Get handle
	if (hFile < 0) {
		// not found
		return false;
	}

	if (Bfile_GetFileSize_OS(hFile) != spaceNeeded) {
		// wrong size (format must have changed, or a bad write)
		return false;
	}

	// read checksum and check it
	unsigned char checkSum[2];
	Bfile_ReadFile_OS(hFile, &checkSum[0], 2, -1);
	if (checkSum[0] != cart[0x14E] || checkSum[1] != cart[0x14F]) {
		// different ROM somehow
		return false;
	}

	// write cpu and mbc state (preserve mbc rom file handle)
	int romFile = mbc.romFile;
	Bfile_ReadFile_OS(hFile, &cpu, sizeof(cpu_type), -1);
	Bfile_ReadFile_OS(hFile, &mbc, sizeof(mbc_state), -1);
	mbc.romFile = romFile;

	// write various work rams
	Bfile_ReadFile_OS(hFile, &wram[0], sizeof(wram), -1);
	Bfile_ReadFile_OS(hFile, &vram[0], sizeof(vram), -1);
	Bfile_ReadFile_OS(hFile, &oam[0], sizeof(oam), -1);

	// only write sram for small ram sizes
	if (RAMSize <= 8 * 1024) {
		Bfile_ReadFile_OS(hFile, &sram[0], RAMSize, -1);
	}

	Bfile_CloseFile_OS(hFile);

	// memory bus controller has to invalidate some stuff, etc
	mbcOnStateLoad();

	// "touch" all of tile memory until we nix that cache
	for (unsigned short addr = 0x8000; addr < 0x9800; addr++) {
		updateTile(addr, vram[addr - 0x8000]);
	}

	return false;
}
