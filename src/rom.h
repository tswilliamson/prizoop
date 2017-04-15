#pragma once

#define ROM_OFFSET_NAME 0x134
#define ROM_OFFSET_GBC 0x143
#define ROM_OFFSET_TYPE 0x147
#define ROM_OFFSET_ROM_SIZE 0x148
#define ROM_OFFSET_RAM_SIZE 0x149

unsigned char loadROM(const char *filename);
void unloadROM(void);
void saveRAM(void);