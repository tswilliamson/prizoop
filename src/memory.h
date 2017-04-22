#pragma once

#ifndef PS4
	#include <stdlib.h>
#endif

#include "debug.h"

#include "gpu.h"
#include "cpu.h"

extern const unsigned char ioReset[0x100];

// various memory areas
extern unsigned char cart[0x4000] ALIGN(256);			// cartridge permanent ROM area
extern unsigned char* vram;								// video ram, variable size: 0x2000 on DMG, 0x4000 on CGB
extern unsigned char sram[0x2000] ALIGN(256);			// cartridge RAM area, may be disabled
extern unsigned char wram_perm[0x1000] ALIGN(256);		// permanent work RAM (same on CGB and DMG)
extern unsigned char wram_gb[0x1000] ALIGN(256);		// GB permanent page, page 1 of 7 for CGB
extern unsigned char oam[0x100] ALIGN(256);				// sprite memory, on gameboy

// disabled RAM/ROM area
extern unsigned char disabledArea[0x100] ALIGN(256);

// maps high byte to different spots in memory
extern unsigned char* memoryMap[256] ALIGN(256);

// Specific bits used for different special mapping purposes:
// Bit 0 : for high memory, specific bytes that need a special read
// Bit 1 : for high memory, specific bytes that need a special write
// Bit 2 : for most significant memory byte, whether a write requires a tile update (TODO, use mem directly in display code)
// Bit 4 : for most significant memory byte, whether a read must validate the area first (switched bank)
extern unsigned int specialMap[256] ALIGN(256);

void resetMemoryMaps(bool isCGB);

void copy(unsigned short destination, unsigned short source, size_t length);

unsigned char readByteSpecial(unsigned int address);
void writeByteSpecial(unsigned int address, unsigned char value);

// called when a write attempt occurs for rom
void mbcWrite(unsigned short address, unsigned char value);
unsigned char mbcRead(unsigned short address);

inline unsigned char readByte(unsigned int address) {
	return (((address >> 8) == 0xff && (specialMap[address & 0xFF] & 0x01)) || (specialMap[address >> 8] & 0x10)) ? readByteSpecial(address) : memoryMap[address >> 8][address & 0xFF];
}

inline unsigned short readShort(unsigned int address) {
	return readByte(address) | (readByte(address + 1) << 8);
}

inline unsigned int readShortFromStack(void) {
	unsigned short value = readShort(cpu.registers.sp);
	cpu.registers.sp += 2;
	return value;
}

// branch avoidance. instructions will just have bad "reads" if somehow we are executing code off the on chip registers
inline unsigned char* getInstrByte(unsigned int address) {
	const unsigned char topBit = address >> 8;
	if (specialMap[topBit] & 0x10) mbcRead(address);			// force flush of cached ROM page
	return memoryMap[topBit] + (address & 0xFF);
}

inline void writeByte(unsigned int address, unsigned char value) {
	// special write cases happen at upper 256 bytes
	if ((address >> 8) == 0xff) {
		if (specialMap[address & 0xFF] & 0x02)
			writeByteSpecial(address, value);
		else
			cpu.memory.all[address & 0xFF] = value;
	}
	// rom "write" goes to memory bank controller instead
	else if ((address & 0x8000) == 0) {
		mbcWrite(address, value);
	}
	// else a normal byte write
	else {
		memoryMap[address >> 8][address & 0xFF] = value;
	}

	// for debugging, usually compiles out
	DebugWrite(address);
}

inline void writeShort(unsigned int address, unsigned short value) {
	writeByte(address, (unsigned char)(value & 0x00ff));
	writeByte(address + 1, (unsigned char)(value >> 8));
}

inline void writeShortToStack(unsigned short value) {
	cpu.registers.sp -= 2;
	writeShort(cpu.registers.sp, value);
}
