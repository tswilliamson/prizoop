
#include "platform.h"

#include "mbc.h"
#include "memory.h"

// cached rom banks
mbc_rombank* cachedRom[CACHE_ROM_BANK_SIZE];
unsigned char cachedRomIndex[CACHE_ROM_BANK_SIZE];
unsigned int lastCacheRequestIndex[CACHE_ROM_BANK_SIZE];
int cacheIndex;

// the memory bus controller object
mbc_state mbc;

const char* getMBCTypeString(mbcType type) {
	switch (type) {
		case ROM_PLAIN: return "ROM_PLAIN";
		case ROM_MBC1: return "ROM_MBC1";
		case ROM_MBC1_RAM: return "ROM_MBC1_RAM";
		case ROM_MBC1_RAM_BATT: return "ROM_MBC1_RAM_BATT";
		case ROM_MBC2: return "ROM_MBC2";
		case ROM_MBC2_BATTERY: return "ROM_MBC2_BATTERY";
		case ROM_RAM: return "ROM_RAM";
		case ROM_RAM_BATTERY: return "ROM_RAM_BATTERY";
		case ROM_MMM01: return "ROM_MMM01";
		case ROM_MMM01_SRAM: return "ROM_MMM01_SRAM";
		case ROM_MMM01_SRAM_BATT: return "ROM_MMM01_SRAM_BATT";
		case ROM_MBC3_TIMER_BATT: return "ROM_MBC3_TIMER_BATT";
		case ROM_MBC3_TIMER_RAM_BATT: return "ROM_MBC3_TIMER_RAM_BATT";
		case ROM_MBC3: return "ROM_MBC3";
		case ROM_MBC3_RAM: return "ROM_MBC3_RAM";
		case ROM_MBC3_RAM_BATT: return "ROM_MBC3_RAM_BATT";
		case ROM_MBC5: return "ROM_MBC5";
		case ROM_MBC5_RAM: return "ROM_MBC5_RAM";
		case ROM_MBC5_RAM_BATT: return "ROM_MBC5_RAM_BATT";
		case ROM_MBC5_RUMBLE: return "ROM_MBC5_RUMBLE";
		case ROM_MBC5_RUMBLE_SRAM: return "ROM_MBC5_RUMBLE_SRAM";
		case ROM_MBC5_RUMBLE_SRAM_BATT: return "ROM_MBC5_RUMBLE_SRAM_BATT";
		case ROM_POCKET_CAMERA: return "ROM_POCKET_CAMERA";
		case ROM_BANDAI_TAMA5: return "ROM_BANDAI_TAMA5";
		case ROM_HUDSON_HUC3: return "ROM_HUDSON_HUC3";
		case ROM_HUDSON_HUC1: return "ROM_HUDSON_HUC1";
		default: return "UNKNOWN";
	}
}

// given a ramSizeType returns a string description
const char* getRAMTypeString(ramSizeType type) {
	switch (type) {
		case RAM_NONE: return "RAM_NONE";
		case RAM_2KB: return "RAM_2KB";
		case RAM_8KB: return "RAM_8KB";
		case RAM_32KB: return "RAM_32KB";
		case RAM_128KB: return "RAM_128KB";
		case RAM_64KB: return "RAM_64KB";
		case RAM_MBC2: return "RAM_MBC2";
		default:  return "RAM_UNKNOWN";
	}
}

// we don't support large RAM sizes yet..
bool supportedRAM(ramSizeType type) {
	// not yet supporting RAM bank switching:
	switch (type) {
		case RAM_NONE: 
		case RAM_2KB:
		case RAM_8KB:
		case RAM_MBC2:
			return true;
	}

	return false;
}

// number of RAM nibbles to allocate to the memory mapper when using SRAM
unsigned char ramNibbleCount(ramSizeType type) {
	switch (type) {
		case RAM_NONE: return 0;
		case RAM_2KB: return 8;
		case RAM_8KB: return 32;
		case RAM_MBC2: return 2;
	}

	// unsupported
	return 0;
}

// returns the cached rom bank (or caches it) with the given index
mbc_rombank* cacheBank(unsigned char index) {
	cacheIndex++;

	int minSlot = 0;
	for (int i = 0; i < CACHE_ROM_BANK_SIZE; i++) {
		if (cachedRomIndex[i] == index) {
			lastCacheRequestIndex[i] = cacheIndex;
			return cachedRom[i];
		} 
		if (lastCacheRequestIndex[i] < lastCacheRequestIndex[minSlot]) {
			minSlot = i;
		}
	}

	// uncached! using minimum cache request index, read into slot from file and return
	int read = Bfile_ReadFile_OS(mbc.romFile, cachedRom[minSlot]->bank, 0x4000, index * 0x4000);
	cachedRomIndex[minSlot] = index;
	lastCacheRequestIndex[minSlot] = cacheIndex;
	return cachedRom[minSlot];
}

// total number of rom banks calculation (16k per bank)
unsigned char numSwitchableBanksFromType(unsigned char romSizeByte) {
	unsigned int numTotalBanks = 2 << romSizeByte;
	return numTotalBanks;
}

// selects the given rom bank
void selectRomBank(unsigned char bankNum) {
	if (bankNum < mbc.numRomBanks && bankNum != mbc.romBank) {
		TIME_SCOPE();

		mbc_rombank* bank = cacheBank(bankNum);
		mbc.romBank = bankNum;

		// map the bank to 0x4000 to 0x7fff:
		for (int i = 0x40; i <= 0x7f; i++) {
			memoryMap[i] = &bank->bank[(i - 0x40) << 8];
		}
	}
}

// selects the given ram bank
void selectRamBank(unsigned char bankNum) {
	// does nothing currently
}

// enable sram by updating memory map
void enableSRAM() {
	int nibbleCount = ramNibbleCount(mbc.ramType);
	for (int i = 0; i < nibbleCount; i++) {
		memoryMap[0xa0 + i] = &sram[i << 8];
	}
}

// disable sram by updating memory map
void disableSRAM() {
	for (int i = 0xa0; i <= 0xbf; i++) {
		memoryMap[i] = &disabledArea[0];
	}
}

bool setupMBCType(mbcType type, unsigned char romSizeByte, unsigned char ramSizeByte, int fileID) {
	memset(&mbc, 0, sizeof(mbc));
	memset(cachedRomIndex, 0, sizeof(cachedRomIndex));
	memset(lastCacheRequestIndex, 0, sizeof(lastCacheRequestIndex));
	cacheIndex = 0;

	mbc.romFile = fileID;
	mbc.type = type;
	mbc.romBank = 0;

	switch (type) {
		case ROM_PLAIN:
			// sram is sometimes in use with these, just enable it:
			mbc.ramType = RAM_8KB;
			mbc.numRomBanks = 2;
			selectRomBank(1);
			enableSRAM();
			return true;
		case ROM_MBC1_RAM_BATT:
			mbc.batteryBacked = 1;
		case ROM_MBC1_RAM:
			mbc.ramType = (ramSizeType)ramSizeByte;
		case ROM_MBC1:
			mbc.numRomBanks = numSwitchableBanksFromType(romSizeByte);
			selectRomBank(1);
			break;
		case ROM_MBC2_BATTERY:
			mbc.batteryBacked = 1;
		case ROM_MBC2:
			mbc.ramType = RAM_MBC2;
			mbc.numRomBanks = numSwitchableBanksFromType(romSizeByte);
			selectRomBank(1);
			break;
		default:
			return false;
	}

	// support if we support the ram bank type
	return supportedRAM(mbc.ramType);
}

// called when a write attempt occurs for rom
void mbcWrite(unsigned short address, unsigned char value) {
	unsigned char upperNibble = address >> 12;
	switch (mbc.type) {
		case ROM_PLAIN:
			// does nada
			return;
		case ROM_MBC1:
		case ROM_MBC1_RAM:
		case ROM_MBC1_RAM_BATT:
			// switch bank mode
			if (upperNibble >= 0x06) {
				mbc.bankMode = value & 1;
				if (mbc.bankMode == 0 && mbc.ramBank != 0) {
					selectRamBank(0);
				}
				else if (mbc.bankMode == 1 && mbc.romBank >= 0x20) {
					selectRomBank(mbc.romBank & 0x1F);
				}
			}
			// upper bank / RAM bank select
			else if (upperNibble >= 0x04) {
				if (mbc.bankMode == 0) {
					selectRomBank(((value & 3) << 5) | (mbc.romBank & 0x1F));
				}
				else {
					selectRamBank(value & 3);
				}
			}
			// ROM bank select (lower 5 bits)
			else if (upperNibble >= 0x02) {
				if ((value & 0x1F) <= 1) {
					// 1 or 0 always selects rom 1
					selectRomBank((mbc.romBank & 0xE0) | 1);
				}
				else {
					// make lower 5 bits into existing rom bank selection
					selectRomBank((mbc.romBank & 0xE0) | (value & 0x1F));
				}
			}
			// RAM enable
			else {
				if ((value & 0x0F) == 0x0A) {
					enableSRAM();
				}
				else {
					disableSRAM();
				}
			}
			return;
		case ROM_MBC2_BATTERY:
		case ROM_MBC2:
			// RAM enable
			if (upperNibble <= 0x01) {
				if ((address & 0x0100) == 0) {
					if ((value & 0x0F) == 0x0A) {
						enableSRAM();
					}
					else {
						disableSRAM();
					}
				}
			} 
			// ROM select
			else if (upperNibble <= 0x03) {
				if ((address & 0x0100) != 0) {
					if ((value & 0x0F) != 0) {
						// make lower 4 bits into existing rom bank selection
						selectRomBank(value & 0x0F);
					}
				}
			}
			return;
	}
}