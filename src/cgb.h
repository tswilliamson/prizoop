#pragma once

/*
 * One stop shop for additional functionality for Gameboy Color support
 */

struct cgb_type {
	// mode
	unsigned char isCGB;				// whether the current emulation mode is CGB mode
	unsigned char isDouble;				// whether the gameboy is in double speed or not
	unsigned char hblankDmaActive;
	unsigned char dirtyPalette;

	// ram banks
	int selectedWRAM;	// currently selected work ram bank
	int selectedVRAM;	// currently selected vram bank

	// new dma
	unsigned int dmaSrc;
	unsigned int dmaDest;
	unsigned int dmaLeft;

	// cgb palette is stored here in two forms (one resolved color, one not. A color resolve is necessary since the color values aren't direct RGB)
	unsigned int curPalTarget;
	unsigned int palette[64];
	unsigned char paletteMemory[128];
};

extern cgb_type cgb;

struct cgbworkram_type {
	unsigned char data[0x1000];
};

// indices 2-7 of CGB work ram banks (bank 1 is wram_gb)
extern cgbworkram_type* cgb_wram[6];

// additional ROM/memory init needed for cgb
void cgbInitROM();

// select the given work ram (indicating 0 will select bank 1)
void cgbSelectWRAM(int ramBank, bool force = false);

// select the given video ram (only 0 or 1)
void cgbSelectVRAM(int ramBank, bool force = false);

// switched between double and normal speed mode
void cgbSpeedSwitch();

// executes DMA op (write to 0xFF55)
void cgbDMAOp(unsigned char value);

// executes hblank DMA (during HBlank period)
void cgbHBlankDMA();

// resolves the palette colors from palette memory
void cgbResolvePalette();

// fixes up cgb state after save state load
void cgbOnStateLoad();

// cleanup needed for cgb roms
void cgbCleanup();