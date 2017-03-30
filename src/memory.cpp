#ifndef PS4
	#include <stdio.h>
#endif

#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "keys.h"
#include "debug.h"

#include "memory.h"

const unsigned char ioReset[0x100] = {
	0x0F, 0x00, 0x7C, 0xFF, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
	0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,
	0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
	0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFF, 0xC1, 0x00, 0xFE, 0xFF, 0xFF, 0xFF,
	0xF8, 0xFF, 0x00, 0x00, 0x00, 0x8F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	0x45, 0xEC, 0x52, 0xFA, 0x08, 0xB7, 0x07, 0x5D, 0x01, 0xFD, 0xC0, 0xFF, 0x08, 0xFC, 0x00, 0xE5,
	0x0B, 0xF8, 0xC2, 0xCE, 0xF4, 0xF9, 0x0F, 0x7F, 0x45, 0x6D, 0x3D, 0xFE, 0x46, 0x97, 0x33, 0x5E,
	0x08, 0xEF, 0xF1, 0xFF, 0x86, 0x83, 0x24, 0x74, 0x12, 0xFC, 0x00, 0x9F, 0xB4, 0xB7, 0x06, 0xD5,
	0xD0, 0x7A, 0x00, 0x9E, 0x04, 0x5F, 0x41, 0x2F, 0x1D, 0x77, 0x36, 0x75, 0x81, 0xAA, 0x70, 0x3A,
	0x98, 0xD1, 0x71, 0x02, 0x4D, 0x01, 0xC1, 0xFF, 0x0D, 0x00, 0xD3, 0x05, 0xF9, 0x00, 0x0B, 0x00
};

// Special read bytes (bit 0):
//		0x00 : keyboard
//		0x04 : DIV, upper 8 bits of internal cpu counter
//      0x05 : TIMA, needs to be updated before being read
//      0x08 - 0x0e : games are known to read from these and expect 0xFF for some reason
//		0x0f : interrupt flags (top 3 bits are always set when read)
//		0x41 : STAT is alway 1 in the high bit

// Special write bytes (bit 1):
//		0x04 : DIV, any writes reset it
//		0x05 : TIMA, writes to it need to adjust our internal timer also
//		0x07 : TAC, writes to it MAY need to adjust our internal timer also
//		0x41 : writes to STAT causes interrupt flags in certain situations
//		0x44 : gpu scanline (read only)
//		0x46 : sprite DMA register (TODO, check clock cycles on this)
//		0x47 : backgroundPalette (TODO, shouldn't need)
//		0x48 : spritePalette 0 (TODO, shouldn't need)
//		0x49 : spritePalette 1 (TODO, shouldn't need)

// Tile memory update area (bit 2)
//		0x8000 - 0x97ff
const unsigned char specialMap[256] ALIGN(256) =
{
	0x01, 0x00, 0x00, 0x00,  0x03, 0x03, 0x00, 0x02,  0x01, 0x01, 0x01, 0x01,  0x01, 0x01, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

	0x00, 0x03, 0x00, 0x00,  0x03, 0x00, 0x02, 0x02,  0x02, 0x02, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

	0x04, 0x04, 0x04, 0x04,  0x04, 0x04, 0x04, 0x04,  0x04, 0x04, 0x04, 0x04,  0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04,  0x04, 0x04, 0x04, 0x04,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00
};

// forced alignment allows us to simply bitwise or in the memory map
unsigned char cart[0x4000] ALIGN(256) = { 0 };
unsigned char vram[0x2000] ALIGN(256) = { 0 };
unsigned char sram[0x2000] ALIGN(256) = { 0 };
unsigned char wram[0x2000] ALIGN(256) = { 0 };
unsigned char oam[0x100] ALIGN(256) = { 0 };

unsigned char disabledArea[0x100] ALIGN(256);

unsigned char* memoryMap[256] ALIGN(256) = { 0 };

void resetMemoryMaps() {
	// disabled RAM/ROM area should return all '1's
	memset(disabledArea, 0xFF, sizeof(disabledArea));

	// permanent rom area
	for (int i = 0x00; i <= 0x3f; i++) {
		memoryMap[i] = &cart[i << 8];
	}

	// extra rom area starts out disabled
	for (int i = 0x40; i <= 0x7f; i++) {
		memoryMap[i] = &disabledArea[0];
	}

	for (int i = 0x80; i <= 0x9f; i++) {
		memoryMap[i] = &vram[(i - 0x80) << 8];
	}

	// Sram starts out disabled
	for (int i = 0xa0; i <= 0xbf; i++) {
		memoryMap[i] = &disabledArea[0];
	}

	// work ram available on cart
	for (int i = 0xc0; i <= 0xdf; i++) {
		memoryMap[i] = &wram[(i - 0xc0) << 8];
	}
	// echo area of work ram
	for (int i = 0xe0; i <= 0xfd; i++) {
		memoryMap[i] = &wram[(i - 0xe0) << 8];
	}

	// on-chip/PPU memory
	memoryMap[0xfe] = oam;
	memoryMap[0xff] = cpu.memory.all;	// on chip memory
}

void copy(unsigned short destination, unsigned short source, size_t length) {
	unsigned int i;
	for(i = 0; i < length; i++) writeByte(destination + i, readByte(source + i));
}

// this only gets called on 0xFF** addresses
unsigned char readByteSpecial(unsigned short address) {
	unsigned char byte = address & 0x00FF;
	switch (byte) {
		case 0x00:
		{
			// keyboard read
			if (!(cpu.memory.P1_joypad & 0x20)) {
				return (unsigned char)(0xc0 | keys.keys1 | 0x10);
			}
			else if (!(cpu.memory.P1_joypad & 0x10)) {
				return (unsigned char)(0xc0 | keys.keys2 | 0x20);
			}
			else return 0xff;
		}
		case 0x04: {
			updateDiv();
			return (cpu.div & 0xFF00) >> 8;
		}
		case 0x05: {
			updateTimer();
			return cpu.memory.TIMA_timerctr;
		}
		// these are unmapped and MUST return 0xFF
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
			return 0xff;
		case 0x0f: return cpu.memory.IF_intflag | 0xE0;	// top 3 bits are always set when reading interrupt flags
		case 0x41: return cpu.memory.STAT_lcdstatus | 0x80;	// high bit always set in STAT
		default:
			return cpu.memory.all[byte];
	}
}

// this only gets called on 0xFF** addresses
void writeByteSpecial(unsigned short address, unsigned char value) {
	unsigned char byte = address & 0x00FF;
	switch (byte) {
		case 0x04:
			// always resets DIV when written to
			cpu.div = 0;
			cpu.divBase = cpu.clocks;
			break;
		case 0x05:
			writeTIMA(value);
			break;
		case 0x07:
			writeTAC(value);
			break;
		case 0x41:
			cpu.memory.STAT_lcdstatus = (value & 0x78) | (cpu.memory.STAT_lcdstatus & 0x7);
			// This may be a DMG only thing?
			if ((GET_LCDC_MODE() == GPU_MODE_HBLANK || GET_LCDC_MODE() == GPU_MODE_VBLANK) && (cpu.memory.LCDC_ctl & 0x80)) {
				cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
			}
			break;
		case 0x44: // read only
			break;
		case 0x46:
			copy(0xfe00, value << 8, 160); // OAM DMA
			break;
		case 0x47:
		{
			// write only
			int i;
			for (i = 0; i < 4; i++) backgroundPalette[i] = (value >> (i * 2)) & 3;
			break;
		}
		case 0x48:
		{
			// write only
			int i;
			for (i = 0; i < 4; i++) spritePalette[0][i] = (value >> (i * 2)) & 3;
			break;
		}
		case 0x49:
		{
			// write only
			int i;
			for (i = 0; i < 4; i++) spritePalette[1][i] = (value >> (i * 2)) & 3;
			break;
		}
	}
}
