
#include "emulator.h"
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

void emulator_type::saveState() {

}

bool emulator_type::loadState() {
	return false;
}
