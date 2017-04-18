
/*
 * One stop shop for additional functionality for Gameboy Color support
 */

#include "platform.h"
#include "cgb.h"
#include "cpu.h"
#include "memory.h"
#include "debug.h"

cgb_type cgb;
cgbworkram_type* cgb_wram[6] = { 0 };

void cgbInitROM() {
	memset(&cgb, 0, sizeof(cgb));

	cpu.registers.a = 0x11;

	// set up additional work RAM banks
	for (int i = 0; i < 6; i++) {
		cgb_wram[i] = new cgbworkram_type;
		memset(&cgb_wram[i]->data, 0, sizeof(0x1000));
	}

	// various IO memory defaults
	cpu.memory.all[0x4D] = 0x7E;			// speed switch
	cpu.memory.all[0x4F] = 0xFE;			// vram bank 0 selected
	cpu.memory.all[0x6C] = 0xFE;
	cpu.memory.all[0x70] = 0xF9;			// RAM select, top 5 bits are never touches
	cpu.memory.all[0x72] = 0x00;
	cpu.memory.all[0x73] = 0x00;
	cpu.memory.all[0x74] = 0x00;
	cpu.memory.all[0x75] = 0x8F;
	cpu.memory.all[0x76] = 0x00;
	cpu.memory.all[0x77] = 0x00;

	// TEMP! to get something on screen
	writeByte(0xFF47, 0xE4);
	writeByte(0xFF48, 0xE4);
	writeByte(0xFF49, 0xE4);

	// defaults to rambank 1
	cgb.selectedWRAM = 1;

	// dma always targets vram
	cgb.dmaDest = 0x8000;

	// initially all cgb colors are white
	memset(cgb.palette, 0xFF, sizeof(cgb.palette));
	memset(cgb.paletteMemory, 0xFF, sizeof(cgb.paletteMemory));

	// we are now in cgb mode!
	cgb.isCGB = true;
}

void cgbSelectWRAM(int ramBank, bool force) {
	DebugAssert(ramBank >= 0 && ramBank <= 7);

	if (ramBank == 0) {
		ramBank = 1;
	}

	if (ramBank != cgb.selectedWRAM || force) {
		cgb.selectedWRAM = ramBank;

		unsigned char* selected;
		if (ramBank == 1) {
			// selecting ram bank 1 selects the normal wram_gb
			selected = wram_gb;
		} else {
			selected = cgb_wram[ramBank - 2]->data;
		}

		for (int i = 0xd0; i <= 0xdf; i++) {
			memoryMap[i] = &selected[(i - 0xd0) << 8];
		}
		// echo area of above ram
		for (int i = 0xf0; i <= 0xfd; i++) {
			memoryMap[i] = &selected[(i - 0xe0) << 8];
		}
	}
}

void cgbSelectVRAM(int ramBank, bool force) {
	DebugAssert(ramBank == 0 || ramBank == 1);

	if (ramBank != cgb.selectedVRAM || force) {
		cgb.selectedVRAM = ramBank;

		for (int i = 0x80; i <= 0x9f; i++) {
			memoryMap[i] = &vram[(i - 0x80 + 0x20 * ramBank) << 8];
		}
	}
}

void cgbSpeedSwitch() {
	cgb.isDouble = !cgb.isDouble;

	if (cgb.isDouble) {
		cpu.memory.KEY1_cgbspeed = 0xFE;	// enable bit 7 = double speed
		for (int i = 0; i < 5; i++) {
			gpuTimes[i] *= 2;
		}
	} else {
		cpu.memory.KEY1_cgbspeed = 0x7E;	// disable bit 7 = normal speed
		for (int i = 0; i < 5; i++) {
			gpuTimes[i] /= 2;
		}
	}
}

inline void cgbDmaCopyBits() {
	DebugAssert(cgb.dmaDest >= 0x8000 && cgb.dmaDest <= 0x9FF0);

	int source = cgb.dmaSrc;
	if (source >= 0xE000) {
		// maps 0xe000 to sram instead:
		source -= 0x4000;
	}

	if (specialMap[source >> 8] & 0x10) {
		// needs mbc validation
		mbcRead(source);
	}

	memcpy(&memoryMap[cgb.dmaDest >> 8][cgb.dmaDest & 0xFF], &memoryMap[source >> 8][source & 0xFF], 16);

	cgb.dmaLeft -= 16;
	cgb.dmaSrc += 16;
	cgb.dmaDest += 16;

	// keep dest in vram
	if (cgb.dmaDest >= 0xA000) {
		cgb.dmaDest -= 0x2000;
	}
}

void cgbDMAOp(unsigned char value) {
	// only support general DMA right now:
	if (value & 0x80) {
		cgb.hblankDmaActive = true;
		cpu.memory.HDMA5_cgbstat = value & 0x7F;
		cgb.dmaLeft = ((value & 0x7F) + 1) << 4;

		// do first copy if during hblank
		// and some weird behavior.. when LCD is turned off, it'll immediately do at least one:
		if ((cpu.memory.STAT_lcdstatus & 3) == GPU_MODE_HBLANK || !(cpu.memory.LCDC_ctl & 0x80)) {
			cgbHBlankDMA();
		}
	} else if (cgb.hblankDmaActive) {
		// just cancel the dma
		cpu.memory.HDMA5_cgbstat = 0x80 | value;
		cgb.hblankDmaActive = false;
		cgb.dmaLeft = 0;
	} else {
		// general purpose DMA
		cgb.dmaLeft = (value + 1) << 4;
		while (cgb.dmaLeft) {
			cgbDmaCopyBits();
		}

		// cpu clocks cost varies based on cpu speed
		if (cgb.isDouble) {
			cpu.clocks += 8 + 64 * value;
		} else {
			cpu.clocks += 8 + 32 * value;
		}

		// HDMA5 reads 0xFF after completing a general dma
		cpu.memory.HDMA5_cgbstat = 0xFF;
	}
}

void cgbHBlankDMA() {
	cgbDmaCopyBits();
	cpu.memory.HDMA5_cgbstat--;

	// cpu clocks cost varies based on cpu speed
	if (cgb.isDouble) {
		cpu.clocks += 68;
	} else {
		cpu.clocks += 36;
	}

	// did the dma finish?
	if (cpu.memory.HDMA5_cgbstat == 0xFF) {
		DebugAssert(cgb.dmaLeft == 0);
		cgb.hblankDmaActive = false;
	}
}

const int translateColor[32] = 
{
	0, 2, 3, 5, 6, 8, 9, 11,
	12, 13, 15, 16, 18, 19, 21, 23,
	25, 26, 26, 26, 27, 27, 27, 28,
	28, 28, 29, 29, 29, 30, 30, 30
};

void cgbResolvePalette() {
	for (int i = 0; i < 64; i++) {
		// simply shuffling the high bit to the bottom for now
		int palColor = cgb.paletteMemory[i * 2] | (cgb.paletteMemory[i * 2 + 1] << 8);
		int trx = 
			(translateColor[palColor & 0x001F] << 11) |			// red
			(translateColor[(palColor & 0x03E0) >> 5] << 6) |			// green
			(translateColor[(palColor & 0x7C00) >> 10]);			// blue

		cgb.palette[i] = trx | (trx << 16);
	}

	cgb.dirtyPalette = false;
}

void cgbOnStateLoad() {
	if (cgb.isDouble) {
		cgb.isDouble = false;
		cgbSpeedSwitch();
	}

	cgbSelectVRAM(cgb.selectedVRAM, true);
	cgbSelectWRAM(cgb.selectedWRAM, true);
}

void cgbCleanup() {
	DebugAssert(cgb.isCGB);

	// switch back to single speed
	if (cgb.isDouble) {
		cgbSpeedSwitch();
	}

	// clean up additional work RAM banks
	for (int i = 0; i < 6; i++) {
		delete cgb_wram[i];
		cgb_wram[i] = NULL;
	}

	// no longer in CGB mode!
	cgb.isCGB = false;
}