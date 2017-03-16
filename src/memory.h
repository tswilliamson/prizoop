#pragma once

#ifndef PS4
	#include <stdlib.h>
#endif

#include "gpu.h"
#include "cpu.h"

extern const unsigned char ioReset[0x100];

// various memory areas
extern unsigned char cart[0x8000] ALIGN(256);
extern unsigned char vram[0x2000] ALIGN(256);
extern unsigned char sram[0x2000] ALIGN(256);
extern unsigned char wram[0x2000] ALIGN(256);
extern unsigned char oam[0x100] ALIGN(256);

// maps high byte to different spots in memory, guaranteed to be 0x100 aligned
extern unsigned char* memoryMap[256] ALIGN(256);

// Specific bits used for different special mapping purposes:
// Bit 0 : for high memory, specific bytes that need a special read
// Bit 1 : for high memory, specific bytes that need a special write
// Bit 2 : for most signicant memory byte, whether a write requires a tile update (TODO, use mem directly in display code)
extern const unsigned char specialMap[256] ALIGN(256);

void SetupMemoryMaps();

void copy(unsigned short destination, unsigned short source, size_t length);

unsigned char readByteSpecial(unsigned short address);
void writeByteSpecial(unsigned short address, unsigned char value);

inline unsigned char readByte(unsigned short address) {
	return ((address >> 8) == 0xff && (specialMap[address & 0xFF] & 0x01)) ? readByteSpecial(address) : memoryMap[address >> 8][address & 0xFF];
}

inline unsigned short readShort(unsigned short address) {
	return readByte(address) | (readByte(address + 1) << 8);
}

inline unsigned short readShortFromStack(void) {
	cpu.registers.sp += 2;
	unsigned short value = readShort(cpu.registers.sp-2);
	return value;
}

inline void writeByte(unsigned short address, unsigned char value) {
	((address >> 8) == 0xff && (specialMap[address & 0xFF] & 0x02)) ?
		writeByteSpecial(address, value) : (void) (memoryMap[address >> 8][address & 0xFF] = value);

	// tile update.. hopefully removed soon
	if (specialMap[address >> 8] & 0x04) updateTile(address, value);
}

inline void writeShort(unsigned short address, unsigned short value) {
	writeByte(address, (unsigned char)(value & 0x00ff));
	writeByte(address + 1, (unsigned char)((value & 0xff00) >> 8));
}

inline void writeShortToStack(unsigned short value) {
	cpu.registers.sp -= 2;
	writeShort(cpu.registers.sp, value);
}
