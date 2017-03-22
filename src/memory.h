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
extern unsigned char vram[0x2000] ALIGN(256);			// video ram, on gameboy
extern unsigned char sram[0x2000] ALIGN(256);			// cartridge RAM area, may be disabled
extern unsigned char wram[0x2000] ALIGN(256);			// work RAM, on gameboy
extern unsigned char oam[0x100] ALIGN(256);				// sprite memory, on gameboy

// disabled RAM/ROM area
extern unsigned char disabledArea[0x100] ALIGN(256);

// maps high byte to different spots in memory
extern unsigned char* memoryMap[256] ALIGN(256);

// Specific bits used for different special mapping purposes:
// Bit 0 : for high memory, specific bytes that need a special read
// Bit 1 : for high memory, specific bytes that need a special write
// Bit 2 : for most signicant memory byte, whether a write requires a tile update (TODO, use mem directly in display code)
extern const unsigned char specialMap[256] ALIGN(256);

void resetMemoryMaps();

void copy(unsigned short destination, unsigned short source, size_t length);

unsigned char readByteSpecial(unsigned short address);
void writeByteSpecial(unsigned short address, unsigned char value);

// called when a write attempt occurs for rom
void mbcWrite(unsigned short address, unsigned char value);

inline unsigned char readByte(unsigned short address) {
	return ((address >> 8) == 0xff && (specialMap[address & 0xFF] & 0x01)) ? readByteSpecial(address) : memoryMap[address >> 8][address & 0xFF];
}

inline unsigned short readShort(unsigned short address) {
	return readByte(address) | (readByte(address + 1) << 8);
}

inline unsigned short readShortFromStack(void) {
	unsigned short value = readShort(cpu.registers.sp);
	cpu.registers.sp += 2;
	return value;
}

// branch avoidance. instructions will just have bad "reads" if somehow we are executing code off the on chip registers
inline unsigned char readInstrByte(unsigned short address) {
	return memoryMap[address >> 8][address & 0xFF];
}
inline unsigned short readInstrShort(unsigned short address) {
	return readInstrByte(address) | (readInstrByte(address + 1) << 8);
}

inline void writeByte(unsigned short address, unsigned char value) {
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
	// tile update.. possibly removed soon
	else if (specialMap[address >> 8] & 0x04) {
		memoryMap[address >> 8][address & 0xFF] = value;
		updateTile(address, value);
	}
	// else a normal byte write
	else {
		memoryMap[address >> 8][address & 0xFF] = value;
	}

	// for debugging, usually compiles out
	DebugWrite(address);
}

inline void writeShort(unsigned short address, unsigned short value) {
	writeByte(address, (unsigned char)(value & 0x00ff));
	writeByte(address + 1, (unsigned char)((value & 0xff00) >> 8));
}

inline void writeShortToStack(unsigned short value) {
	cpu.registers.sp -= 2;
	writeShort(cpu.registers.sp, value);
}
