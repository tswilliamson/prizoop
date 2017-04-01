
#include "emulator.h"

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
}

bool emulator_type::loadSettings() {
	return false;
}