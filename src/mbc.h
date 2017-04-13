#pragma once

// requires 4k per bank, allocated on stack on the Prizm
#define NUM_CACHED_BANKS 48

enum mbcType {
	ROM_PLAIN = 0x00,
	ROM_MBC1 = 0x01,
	ROM_MBC1_RAM = 0x02,
	ROM_MBC1_RAM_BATT = 0x03,
	ROM_MBC2 = 0x05,
	ROM_MBC2_BATTERY = 0x06,
	ROM_RAM = 0x08,
	ROM_RAM_BATTERY = 0x09,
	ROM_MMM01 = 0x0B,
	ROM_MMM01_SRAM = 0x0C,
	ROM_MMM01_SRAM_BATT = 0x0D,
	ROM_MBC3_TIMER_BATT = 0x0F,
	ROM_MBC3_TIMER_RAM_BATT = 0x10,
	ROM_MBC3 = 0x11,
	ROM_MBC3_RAM = 0x12,
	ROM_MBC3_RAM_BATT = 0x13,
	ROM_MBC5 = 0x19,
	ROM_MBC5_RAM = 0x1A,
	ROM_MBC5_RAM_BATT = 0x1B,
	ROM_MBC5_RUMBLE = 0x1C,
	ROM_MBC5_RUMBLE_SRAM = 0x1D,
	ROM_MBC5_RUMBLE_SRAM_BATT = 0x1E,
	ROM_POCKET_CAMERA = 0x1F,
	ROM_BANDAI_TAMA5 = 0xFD,
	ROM_HUDSON_HUC3 = 0xFE,
	ROM_HUDSON_HUC1 = 0xFF,
};

enum ramSizeType {
	RAM_NONE = 0x00,
	RAM_2KB = 0x01,
	RAM_8KB = 0x02,
	RAM_32KB = 0x03,			// 4 banks of 8 KB
	RAM_128KB = 0x04,			// 16 banks of 8 KB
	RAM_64KB = 0x05,			// 8 banks of 8 KB
	RAM_MBC2 = 0xFF,			// special case for MBC2 (512 x 4 bit)
};

struct mbc_state {
	int romFile;					// rom File ID

	mbcType type;					// active type
	ramSizeType ramType;			// active ram type
	unsigned short numRomBanks;		// number of 16 KB ROM banks
	unsigned char numRamBanks;		// number of 8 KB RAM Banks
	unsigned char batteryBacked;	// 1 if rom battery backup is used for RAM
	unsigned char rumblePack;		// 1 if rumble pack is part of the ROM

	unsigned char romBank;			// selected rom bank
	unsigned char ramBank;			// selected ram bank
	unsigned char bankMode;			// various mbc types use changable bank modes
	unsigned char sramEnabled;		// whether sram is currently enabled
};

// each cache has 2 extra bytes in it to account for instruction overlap
struct mbc_bankcache {
	unsigned char bank[0x1002];
};

extern mbc_state mbc;

// returns false if type is not supported
bool setupMBCType(mbcType type, unsigned char romSizeByte, unsigned char ramSizeByte, int fileID);

// attempts to load SRAM from the given file path, false on error
bool tryLoadSRAM(const char* filepath);

// attempts to save SRAM for a game to the given file path
void trySaveSRAM(const char* filepath);

// given a mbcType returns a string description
const char* getMBCTypeString(mbcType type);

// given a ramSizeType returns a string description
const char* getRAMTypeString(ramSizeType type);

// returns current mbc ram size in bytes
unsigned int getRAMSize();

// call after state save load for proper handling
void mbcOnStateLoad();

// called when play begins or file I/O happens during gameplay
void mbcFileUpdate();

// pointers to each cached bank
extern mbc_bankcache* cachedBanks[NUM_CACHED_BANKS];

// cached ROM banks is alloc'd in main() on the stack
#define ALLOCATE_CACHED_BANKS() mbc_bankcache stackCachedBanks[NUM_CACHED_BANKS]; for (int r = 0; r < NUM_CACHED_BANKS; r++) { cachedBanks[r] = &stackCachedBanks[r]; }