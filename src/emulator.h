#pragma once

#include "cpu.h"
#include "mbc.h"

namespace emu_button {
	enum {
		A = 0,
		B = 1,
		SELECT = 2,
		START = 3,
		RIGHT = 4,
		LEFT = 5,
		UP = 6,
		DOWN = 7,
		STATE_SAVE = 8,
		STATE_LOAD = 9,
		MAX = 10
	};
}

// Emulation settings
#define SETTINGS_VERSION 2
struct emulator_settings {
	int version;
	char selectedRom[32];
	unsigned char overclock;
	unsigned char scaleToScreen;
	unsigned char useCGBColors;
	unsigned char clampSpeed;
	unsigned char keyMap[12];	// padding past emu_button::MAX to make it 4 byte aligned
	char frameSkip;
	unsigned char bgColorPalette;
	unsigned char obj1ColorPalette;
	unsigned char obj2ColorPalette;
	unsigned char sound;
	unsigned char padding[2];
};

// color palette colors
struct colorpalette_type {
	unsigned short colors[4];
};

extern const unsigned char* bg_menu;
extern const unsigned char* bg_1x1;
extern const unsigned char* bg_fit;

struct emulator_screen {
	int fKey;

	virtual void setup() {}
	virtual void select() {}
	virtual void deselect() {}
	virtual void postStateChange() {}

	// key press handles
	virtual void handleUp() {}
	virtual void handleDown() {}
	virtual void handleLeft() {}
	virtual void handleRight() {}
	virtual void handleSelect() {}

protected:
	// helper functions
	static void DrawBG(const char* filename);
	static void DrawBGEmbedded(unsigned char* compressedData);

	// Print a string at the given coordinates
	static void Print(int x, int y, const char* buffer, bool selected, unsigned short color = COLOR_WHITE);

	// Width of the given string in pixels
	static int PrintWidth(const char* buffer);

	// Draws the pause preview (if valid) to the VRAM
	static void DrawPausePreview();
};

// The main emulator object
// Maintains the state of the emulator but the individual components remain independent
struct emulator_type {
	emulator_settings settings;

	// menu
	int curScreen;
	emulator_screen* screens[6];

	// pause preview window (4 bits per pixel)
	unsigned char pausePreview[80 * 72 / 2];
	bool pausePreviewValid;

	// overall application
	void startUp();
	void run();
	void shutDown();

	// options display and handling
	bool loadSettings();			// returns false if not found
	void saveSettings();
	void defaultSettings();

	// load the given rom filename, returns false on error (unsupported MBC, etc)
	bool loadRom(const char* filename);

	// handling currently loaded rom
	void bootLoadedRom();			// boots loaded rom from 0x100 address and reset Gameboy state
	void saveState();
	bool loadState();				// returns true if a state was found

	// custom color palettes
	static unsigned char numPalettes();
	static void getPalette(unsigned char paletteNum, colorpalette_type& intoColors);

	void tryScreenChange(int targetFKey);
};

extern emulator_type emulator;