#pragma once

/*
 * One stop shop for additional functionality for Gameboy Color support
 */

struct cgb_type {
	// mode
	bool isCGB;			// whether the current emulation mode is CGB mode
	bool isDouble;		// whether the gameboy is in double speed or not

	// ram banks
	int selectedWRAM;	// currently selected work ram bank
	int selectedVRAM;	// currently selected vram bank

	// new dma
	bool hblankDmaActive;
	unsigned short dmaSrc;
	unsigned short dmaDest;
	unsigned short dmaLeft;
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
void cgbSelectWRAM(int ramBank);

// select the given video ram (only 0 or 1)
void cgbSelectVRAM(int ramBank);

// switched between double and normal speed mode
void cgbSpeedSwitch();

// executes DMA op (write to 0xFF55)
void cgbDMAOp(unsigned char value);

// executes hblank DMA (during HBlank period)
void cgbHBlankDMA();

// cleanup needed for cgb roms
void cgbCleanup();