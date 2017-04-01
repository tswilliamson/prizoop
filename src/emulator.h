#pragma once

#include "cpu.h"
#include "mbc.h"

enum emu_button {
	A = 0,
	B = 1,
	SELECT = 2,
	START = 3,
	RIGHT = 4,
	LEFT = 5,
	UP = 6, 
	DOWN = 7,
	MAX = 8
};

// Emulation settings
struct emulator_settings {
	char selectedRom[32];
	bool overclock : 8;
	bool scaleToScreen : 8;
	bool useCGBColors : 8;
	bool clampSpeed : 8;
	unsigned char keyMap[emu_button::MAX]; 
	char frameSkip;
	unsigned char bgColorPalette;
	unsigned char obj1ColorPalette;
	unsigned char obj2ColorPalette;
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
	virtual void setup();
	virtual void run();
	virtual void cleanup();
};

// The main emulator object
// Maintains the state of the emulator but the individual components remain independent
struct emulator_type {
	emulator_settings settings;

	// menu
	int curScreen;
	emulator_screen* screens[6];
	void changeScreen(emulator_screen* newScreen);

	// overall application
	void startUp();

	// options display and handling
	bool loadSettings();			// returns false if not found
	void saveSettings();

	// load the given rom filename, returns false on error (unsupported MBC, etc)
	bool loadRom(const char* filename);

	// handling currently loaded rom
	void bootLoadedRom();			// boots loaded rom from 0x100 address and reset Gameboy state
	void saveState();
	bool loadState();				// returns true if a state was found
};

extern emulator_type emulator;