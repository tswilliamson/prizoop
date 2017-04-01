
#include "emulator.h"
#include "screen_rom.h"

emulator_type emulator;

// Main emulator functionality
void emulator_type::startUp() {
	curScreen = 0;

	if (!loadSettings()) {
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

		// simulator only defaults
#if TARGET_WINSIM
		settings.frameSkip = 0;
#endif
	}

	// set up each menu screen
	screen_rom* romScreen = new screen_rom;
	romScreen->fKey = -1;

	memset(screens, 0, sizeof(screens));
	screens[0] = romScreen;

	for (int i = 0; i < 6; i++) {
		if (screens[i]) {
			screens[i]->setup();
		}
	}

	curScreen = 0;
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

bool emulator_type::loadSettings() {
	return false;
}