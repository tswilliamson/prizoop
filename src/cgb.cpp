
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

	// we are now in cgb mode!
	cgb.isCGB = true;
}

void cgbSelectWRAM(int ramBank) {
	DebugAssert(ramBank >= 0 && ramBank <= 7);

	if (ramBank == 0) {
		ramBank = 1;
	}

	if (ramBank != cgb.selectedWRAM) {
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

void cgbSelectVRAM(int ramBank) {
	DebugAssert(ramBank == 0 || ramBank == 1);

	if (ramBank != cgb.selectedVRAM) {
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

	if (cgb.dmaSrc >= 0xE000) {
		// maps 0xe000 to sram instead:
		memcpy(&memoryMap[cgb.dmaDest >> 8][cgb.dmaDest & 0xFF], &memoryMap[(cgb.dmaSrc - 0x4000) >> 8][cgb.dmaSrc & 0xFF], 16);
	} else {
		memcpy(&memoryMap[cgb.dmaDest >> 8][cgb.dmaDest & 0xFF], &memoryMap[cgb.dmaSrc >> 8][cgb.dmaSrc & 0xFF], 16);
	}

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
	DebugAssert((value & 80) == 0);
	if (value & 0x80) {
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
	}
}

void cgbHBlankDMA() {
	DebugAssert(false);
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