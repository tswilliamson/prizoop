#pragma once

#include "cpu.h"
#include "mbc.h"

// Emulation settings
struct emulator_settings {

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

// The main emulator object
// Maintains the state of the emulator but the individual components remain independent
struct emulator {
	// options display and handling
	void loadSettings();
	void saveSettings();
	void showSettings();

	// Displays a list of available ROM files (looks in either the root folder, in "ROMS" folder, or "Games" folder)
	const char* selectRom();

	// load the given rom filename, returns false on error (unsupported MBC, etc)
	bool loadRom(const char* filename);

	// handling currently loaded rom
	void bootLoadedRom();
	void setRomState(emulator_state* fromState);
	void getRomState(emulator_state* toState);
};