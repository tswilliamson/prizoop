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
struct emulator_settings {
	char selectedRom[32];
	unsigned char overclock;
	unsigned char scaleToScreen;
	unsigned char useCGBColors;
	unsigned char clampSpeed;
	unsigned char keyMap[emu_button::MAX]; 
	char frameSkip;
	unsigned char bgColorPalette;
	unsigned char obj1ColorPalette;
	unsigned char obj2ColorPalette;
};

// color palette colors
struct colorpalette_type {
	unsigned short colors[4];
};

// Serializable state container for the emulator
struct emulator_state {
	unsigned short headerChecksum;		// checksum from 0x14E of the ROM 
	cpu_type cpu;						// full cpu structure state
	mbc_state mbc;						// full mbc structure state

	// saved ram blocks (variable size)
	unsigned long sramSize;				
	unsigned char* sram;

	unsigned long wramSize;				
	unsigned char* wram;							

	// saved display ram regions (fixed size)
	unsigned char* vram;							
	unsigned char* oam;
};

struct emulator_screen {
	int fKey;

	virtual void setup() {}
	virtual void select() {}
	virtual void deselect() {}

	// key press handles
	virtual void handleUp() {}
	virtual void handleDown() {}
	virtual void handleLeft() {}
	virtual void handleRight() {}
	virtual void handleSelect() {}

protected:
	// helper functions
	static void DrawBG(const char* filename, int x1 = 0, int y1 = 0, int x2 = 384, int y2 = 216);

	// Print a string at the given coordinates
	static void Print(int x, int y, const char* buffer, bool selected, unsigned short color = COLOR_WHITE);

	// Width of the given string in pixels
	static int PrintWidth(const char* buffer);
};

// The main emulator object
// Maintains the state of the emulator but the individual components remain independent
struct emulator_type {
	emulator_settings settings;

	// menu
	int curScreen;
	emulator_screen* screens[6];

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