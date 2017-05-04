
#include "platform.h"

#include "mbc.h"
#include "keys.h"
#include "memory.h"
#include "zx7\zx7.h"

// cached banks
mbc_bankcache* cachedBanks[NUM_CACHED_BANKS];
unsigned int cachedBankIndex[NUM_CACHED_BANKS];
unsigned int lastCacheRequestIndex[NUM_CACHED_BANKS];
int cacheIndex = 0;

// for higher RAM requirements, we use cached banks to store our ram, the rest our stored starting with firstRomCache, as indicated by this var
unsigned int firstRomCache = 0;

// sram hash (for checking dirty state, need to save)
unsigned int sramHash = 0;

// the memory bus controller object
mbc_state mbc;

// the real time clock object
rtc_state rtc;
unsigned char rtcMap[256] ALIGN(256);

// compressed page locations for each rom file page
int* compressedPages = NULL;

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

unsigned int getRAMSize() {
	switch (mbc.ramType) {
		case RAM_NONE: return 0;
		case RAM_2KB: return 1024 * 2;
		case RAM_8KB: return 1024 * 8;
		case RAM_32KB: return 1024 * 32;
		case RAM_128KB: return 1024 * 128;
		case RAM_64KB: return 1024 * 64;
		case RAM_MBC2: return 512;
		default:  return 0;
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
			// sram will do
			mbc.ramBank = 0;
			mbc.numRamBanks = 1;
			firstRomCache = 0;
			return true;
		case RAM_32KB:
			// need 2 pages:
			mbc.ramBank = 0;
			mbc.numRamBanks = 4;
			firstRomCache = 8;
			return true;
	}

	return false;
}

// number of RAM nibbles to allocate to the memory mapper when using SRAM
unsigned char ramNibbleCount(ramSizeType type) {
	switch (type) {
		case RAM_NONE: return 0;
		case RAM_2KB: return 8;
		case RAM_32KB:
		case RAM_8KB: return 32;
		case RAM_MBC2: return 2;
	}

	// unsupported
	return 0;
}

// la dee da implementing my own syscall
#if !TARGET_WINSIM
#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80020070
typedef int(*sc_ipiii)(int,int,unsigned int*);
const unsigned int sc1DAA[] = { SCA, SCB, SCE, 0x1DAA };
#define Bfile_GetBlockAddress (*(sc_ipiii)sc1DAA)

// support up to a 4 MB ROM 
static unsigned int BlockAddresses[1024] = { 0 };
void mbcFileUpdate() {
	int numBlocks = (Bfile_GetFileSize_OS(mbc.romFile) + 4095) / 4096; // 16k ROM banks means 4 4k blocks a piece
	for (int i = 0; i < numBlocks; i++) {
		int ret = Bfile_GetBlockAddress(mbc.romFile, i * 0x1000, &BlockAddresses[i]);
		if (ret < 0)
			// error!
			return;
	}
}
#else
void mbcFileUpdate() {
}
#endif

static int mbcReadFile(unsigned char* into, unsigned int size, unsigned int offset) {
#if !TARGET_WINSIM
	// use block addresses to do direct copy
	int sizeLeft = size;
	int block = offset / 4096;
	int blockAddr = offset % 4096;
	while (sizeLeft) {
		int blockRem = min(4096 - blockAddr, sizeLeft);
		memcpy(into, (const void*)(BlockAddresses[block]+blockAddr), blockRem);
		into += blockRem;
		sizeLeft -= blockRem;

		block++;
		blockAddr = 0;
	}
	return size;
#else
	return Bfile_ReadFile_OS(mbc.romFile, into, size, offset);
#endif
}

bool mbcReadPage(unsigned int bankIndex, unsigned char* target, bool instrOverlap) {
	unsigned int overlapBytes = instrOverlap ? 2 : 0;

	if (mbc.compressed) {
		int compSize = compressedPages[bankIndex + 1] - compressedPages[bankIndex];

		if (compSize == 4098) {
			mbcReadFile(target, 0x1000 + overlapBytes, compressedPages[bankIndex]);
		} else {
			unsigned char compBuffer[4098];
			mbcReadFile(compBuffer, compSize, compressedPages[bankIndex]);
			if (instrOverlap) {
				ZX7Decompress(compBuffer, target, 4098);
			} else {
				unsigned char uncompBuffer[4098];
				ZX7Decompress(compBuffer, uncompBuffer, 4098);
				memcpy(target, uncompBuffer, 4096);
			}
		}

		return true;
	} else {
		unsigned int read = mbcReadFile(target, 0x1000 + overlapBytes, bankIndex * 0x1000);
		DebugAssert(read == 0x1000 + overlapBytes);
		if (read == 0x1000 + overlapBytes) {
			return true;
		}
	}

	return false;
}

// returns the cached rom bank (or caches it) with the given index (which is a a factor of the number of caches per rom bank)
mbc_bankcache* cacheBank(unsigned int index) {
	cacheIndex++;

	// rare case: on overflow, the cache system needs a reset
	if (cacheIndex == 0) {
		for (int i = firstRomCache; i < NUM_CACHED_BANKS; i++) {
			lastCacheRequestIndex[i] = 0;
		}
	}

	int minSlot = firstRomCache;
	for (int i = firstRomCache; i < NUM_CACHED_BANKS; i++) {
		if (cachedBankIndex[i] == index) {
			lastCacheRequestIndex[i] = cacheIndex;
			return cachedBanks[i];
		} 
		if (lastCacheRequestIndex[i] < lastCacheRequestIndex[minSlot]) {
			minSlot = i;
		}
	}

	// uncached! using minimum cache request index, read into slot from file and return
	if (!mbcReadPage(index, cachedBanks[minSlot]->bank,  index != (mbc.numRomBanks * 4 - 1))) {
		// attempt to escape
		keys.exit = true;
		return cachedBanks[0];
	} else {
		cachedBankIndex[minSlot] = index;
		lastCacheRequestIndex[minSlot] = cacheIndex;
		return cachedBanks[minSlot];
	}
}

// total number of rom banks calculation (16k per bank)
unsigned short numSwitchableBanksFromType(unsigned char romSizeByte) {
	unsigned int numTotalBanks = 2 << romSizeByte;
	return numTotalBanks;
}

// selects the given rom bank
void selectRomBank(unsigned char bankNum) {
	if (bankNum < mbc.numRomBanks && bankNum != mbc.romBank) {
		TIME_SCOPE();
		
		mbc.romBank = bankNum;

		// invalidate addresses in memory map for reading
		for (int i = 0x40; i <= 0x7f; i++) {
			specialMap[i] |= 0x10;
		}
	}
}

unsigned char mbcRead(unsigned short address) {
	DebugAssert(address >= 0x4000 && address <= 0x7FFF);

	unsigned char highNibble = address >> 12;
	mbc_bankcache* cache = cacheBank(mbc.romBank * 4 + highNibble - 4);

	for (int i = 0; i < 16; i++) {
		// map the cache
		memoryMap[(highNibble << 4) + i] = &cache->bank[256 * i];
		// we've validated the highest nibble
		specialMap[(highNibble << 4) + i] &= ~0x10;
	}

	return memoryMap[address >> 8][address & 0xFF];
}

// selects the given ram bank
void selectRamBank(unsigned char bankNum, bool force = false) {
	if ((bankNum != mbc.ramBank || force) && bankNum < mbc.numRamBanks) {
		// map memory to our usurped rom cache area
		for (int i = 0; i < 16; i++) {
			memoryMap[0xa0 + i] = &cachedBanks[bankNum * 2]->bank[i << 8];
		}
		for (int i = 0; i < 16; i++) {
			memoryMap[0xb0 + i] = &cachedBanks[bankNum * 2+1]->bank[i << 8];
		}
		mbc.ramBank = bankNum;
	}
}

// enable sram by updating memory map
void enableSRAM() {
	mbc.sramEnabled = 1;
	if (mbc.numRamBanks <= 1) {
		int nibbleCount = ramNibbleCount(mbc.ramType);
		for (int i = 0; i < nibbleCount; i++) {
			memoryMap[0xa0 + i] = &sram[i << 8];
		}
	} else {
		selectRamBank(mbc.ramBank, true);
	}
}

// disable sram by updating memory map
void disableSRAM() {
	mbc.sramEnabled = 0;
	for (int i = 0xa0; i <= 0xbf; i++) {
		memoryMap[i] = &disabledArea[0];
	}
}


// MBC3 RTC support

#if TARGET_PRIZM
inline int FromBCD(int x) {
	return (x & 0xF) + ((x & 0xF0) >> 4) * 10 + ((x & 0xF00) >> 8) * 100 + ((x & 0xF000) >> 12) * 1000;
}
#define RTCSeconds() FromBCD(*((unsigned char*) (RTC_port + 2)))
#define RTCMinute() FromBCD(*((unsigned char*) (RTC_port + 4)))
#define RTCHour() FromBCD(*((unsigned char*) (RTC_port + 6)))
#define RTCDay() FromBCD(*((unsigned char*) (RTC_port + 10)))
#define RTCMonth() FromBCD(*((unsigned char*) (RTC_port + 12)))
#define RTCYear() FromBCD(*((unsigned short*) (RTC_port + 14)))
#endif

// converts current rtc values to seconds
unsigned int rtcToSeconds() {
	unsigned char sec, min, hour, day, month;
	unsigned short year;

#if TARGET_PRIZM
	sec = RTCSeconds();
	min = RTCMinute();
	hour = RTCHour();
	day = RTCDay();
	month = RTCMonth();
	year = RTCYear();
#else
	time_t t = time(0);
	tm* cal = localtime(&t);
	sec = cal->tm_sec;
	min = cal->tm_min;
	hour = cal->tm_hour;
	day = cal->tm_mday;
	month = cal->tm_mon + 1;
	year = cal->tm_year + 1900;
#endif

	// calculate days given years and month
	// accurate to 16 years is fine (and avoids inaccuracy)
	year %= 16;
	if (year) {
		day += (year - 1) * 1461 / 4;		// 365.25
	}
	switch (month) {
		case 12: day += 30;
		case 11: day += 31;
		case 10: day += 30;
		case  9: day += 31;
		case  8: day += 31;
		case  7: day += 30;
		case  6: day += 31;
		case  5: day += 30;
		case  4: day += 31;
		case  3: day += 28; if (year % 4 == 0) day++;
		case  2: day += 31;
	}
	
	// calculate seconds and return
	return sec + min * 60 + hour * 60 * 60 + day * 60 * 60 * 24;
}

bool mbcIsRTC() {
	return mbc.type == ROM_MBC3_TIMER_BATT || mbc.type == ROM_MBC3_TIMER_RAM_BATT;
}

#define RTC_SEC 0x08
#define RTC_MIN 0x09
#define RTC_HOUR 0x0A
#define RTC_DAYS 0x0B
#define RTC_CTL 0x0C

void rtcLatch() {
	// latch
	rtc.curRTC = rtcToSeconds() - rtc.rtcBase;
}

int rtcGetLatched(int forReg) {
	switch (forReg) {
		case RTC_SEC:
			return rtc.curRTC % 60;
		case RTC_MIN:
			return (rtc.curRTC / 60) % 60;
		case RTC_HOUR:
			return (rtc.curRTC / 3600) % 24;
		case RTC_DAYS:
			return (rtc.curRTC / (3600 * 24)) % 256;
		case RTC_CTL:
			// bit 0 : hi day bit
			// bit 6 : halt
			// bit 7 : day carry
			return
				(((rtc.curRTC / (3600 * 24)) / 256) ? 1 : 0) |
				(rtc.isHalted ? (1 << 6) : 0) |
				(((rtc.curRTC / (3600 * 24)) / 512) ? (1 << 7) : 0);
	}

	return 0;
}

void rtcCheckDirty() {
	// check for dirty rtc value if a register is selected (it's about to change)
	if (rtc.rtcReg) {
		unsigned char newValue = rtc.rtcValue;

		for (int i = 0; i < 256; i++) {
			if (rtcMap[i] != rtc.rtcValue) {
				newValue = rtcMap[i];
				break;
			}
		}

		if (newValue != rtc.rtcValue) {
			// update rtc base (or halt) according to type of value selected
			int diff = newValue - (int) rtc.rtcValue;
			switch (rtc.rtcReg) {
				case RTC_SEC:
					rtc.curRTC += diff;
					rtc.rtcBase -= diff;
					break;
				case RTC_MIN:
					rtc.curRTC += diff * 60;
					rtc.rtcBase -= diff * 60;
					break;
				case RTC_HOUR:
					rtc.curRTC += diff * 3600;
					rtc.rtcBase -= diff * 3600;
					break;
				case RTC_DAYS:
					rtc.curRTC += diff * 3600 * 24;
					rtc.rtcBase -= diff * 3600 * 24;
					break;
				case RTC_CTL:
				{
					// remove 512 day carry flag?
					if ((rtc.rtcValue & 0x80) && (newValue & 0x80) == 0) {
						int newVal = rtc.curRTC % (3600 * 24 * 512);
						int diff = newVal - rtc.curRTC;
						rtc.curRTC += diff;
						rtc.rtcBase -= diff;
					}
					// remove 256 day flag?
					if ((rtc.rtcValue & 0x01) != (newValue & 0x01)) {
						int newVal;
						if (newValue & 0x01) {
							newVal = rtc.curRTC + 3600 * 24 * 256;
						} else {
							newVal = rtc.curRTC % (3600 * 24 * 256);
						}
						int diff = newVal - rtc.curRTC;
						rtc.curRTC += diff;
						rtc.rtcBase -= diff;
					}
					// halt flag
					if ((rtc.rtcValue & 0x40) != (newValue & 0x40)) {
						if (newValue & 0x40) {
							// halt
							rtc.isHalted = true;
						} else {
							// unhalt
							rtc.isHalted = false;
							rtc.rtcBase = rtcToSeconds() - rtc.curRTC;
						}
					}
				}
			}

			rtc.rtcValue = newValue;
		}
	}
}

void rtcSelectReg(unsigned char reg) {
	rtc.rtcReg = reg;

	rtc.rtcValue = rtcGetLatched(reg);
	
	// update rtcmap and point memory to it
	for (int i = 0; i < 256; i++) {
		rtcMap[i] = rtc.rtcValue;
	}
	for (int i = 0xa0; i <= 0xbf; i++) {
		memoryMap[i] = &rtcMap[0];
	}
}

bool setupMBCType(mbcType type, unsigned char romSizeByte, unsigned char ramSizeByte, int fileID) {
	memset(&mbc, 0, sizeof(mbc));
	memset(&rtc, 0, sizeof(rtc));
	memset(cachedBankIndex, 0, sizeof(cachedBankIndex));
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
			mbc.numRamBanks = 1;
			selectRomBank(1);
			enableSRAM();
			return true;
		case ROM_MBC2_BATTERY:
			mbc.batteryBacked = 1;
		case ROM_MBC2:
			mbc.ramType = RAM_MBC2;
			mbc.numRomBanks = numSwitchableBanksFromType(romSizeByte);
			selectRomBank(1);
			break;
		// support for MBC 1/3/5
		case ROM_MBC1_RAM_BATT:
		case ROM_MBC3_RAM_BATT:
		case ROM_MBC3_TIMER_BATT:
		case ROM_MBC3_TIMER_RAM_BATT:
		case ROM_MBC5_RAM_BATT:
		case ROM_MBC5_RUMBLE_SRAM_BATT:
			mbc.batteryBacked = 1;
		case ROM_MBC1_RAM:
		case ROM_MBC3_RAM:
		case ROM_MBC5_RAM:
		case ROM_MBC5_RUMBLE_SRAM:
			mbc.ramType = (ramSizeType)ramSizeByte;
		case ROM_MBC1:
		case ROM_MBC3:
		case ROM_MBC5:
		case ROM_MBC5_RUMBLE:
			mbc.numRomBanks = numSwitchableBanksFromType(romSizeByte);
			selectRomBank(1);
			break;
		default:
			return false;
	}

	if (type == ROM_MBC5_RUMBLE || type == ROM_MBC5_RUMBLE_SRAM || type == ROM_MBC5_RUMBLE_SRAM_BATT) {
		mbc.rumblePack = 1;
	}

	if (mbcIsRTC()) {
		// give it at least 1 second
		rtc.rtcBase = rtcToSeconds() - 1;
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
		case ROM_MBC3:
		case ROM_MBC3_RAM:
		case ROM_MBC3_RAM_BATT:
		case ROM_MBC3_TIMER_BATT:
		case ROM_MBC3_TIMER_RAM_BATT:
			if (upperNibble <= 0x01) {
				rtcCheckDirty();

				if ((value & 0x0F) == 0x0A) {
					enableSRAM();
				} else {
					disableSRAM();
					rtc.rtcReg = 0;
				}
			}
			else if (upperNibble <= 0x03) {
				// lower 7 bits
				value &= 0x7F;
				if (value == 0) {
					selectRomBank(1);
				}
				else {
					selectRomBank(value);
				}
			}
			else if (upperNibble <= 0x05) {
				rtcCheckDirty();
				rtc.rtcReg = 0;

				if (value < 0x08 || !mbcIsRTC()) {
					selectRamBank(value);
				} else {
					rtcSelectReg(value);
				}
			}
			else if (upperNibble <= 0x07 && mbcIsRTC()) {
				if (rtc.lastLatch == 0 && value == 1) {
					rtcLatch();
				}
				rtc.lastLatch = value;
			}
			return;
		case ROM_MBC5:
		case ROM_MBC5_RAM:
		case ROM_MBC5_RAM_BATT:
		case ROM_MBC5_RUMBLE:
		case ROM_MBC5_RUMBLE_SRAM:
		case ROM_MBC5_RUMBLE_SRAM_BATT:
			if (upperNibble <= 0x01) {
				if ((value & 0x0F) == 0x0A) {
					enableSRAM();
				} else {
					disableSRAM();
				}
			}
			// TODO : no support for ROM's greater > 256 banks
			else if (upperNibble <= 0x03) {
				if (value == 0) {
					selectRomBank(1);
				} else {
					selectRomBank(value);
				}
			}
			else if (upperNibble <= 0x05) {
				if (mbc.rumblePack) {
					selectRamBank(value & 0x7);
				} else {
					selectRamBank(value);
				}
			}
			return;
	}
}

// SRAM saving / loading
static unsigned int curRAMHash(int size) {
	unsigned int ret = 0;

	if (size <= 8192) {
		for (int i = 0; i < size; i++) {
			ret = ((ret << 5) + (ret >> 27)) ^ sram[i];
		}
	} else {
		for (int b = 0; b <= size / 0x1000; b++) {
			for (int i = 0; i < 0x1000; i++) {
				ret = ((ret << 5) + (ret >> 27)) ^ cachedBanks[b]->bank[i];
			}
		}
	}

	return ret;
}

// attempts to load SRAM from the given file path, false on error
bool tryLoadSRAM(const char* filepath) {
	int sramSize = ramNibbleCount(mbc.ramType) * 256;
	int rtcSize = mbcIsRTC() ? sizeof(rtc_state) : 0;

	if (mbc.numRamBanks > 1) {
		sramSize = 8192 * mbc.numRamBanks;
	}

	if (sramSize + rtcSize) {
		unsigned short pFile[256];
		Bfile_StrToName_ncpy(pFile, (const char*)filepath, strlen(filepath) + 2);

		int hFile = Bfile_OpenFile_OS(pFile, READ, 0); // Get handle
		if (hFile < 0) {
			// not found
			return false;
		}

		if (sramSize <= 8192) {
			if (Bfile_ReadFile_OS(hFile, sram, sramSize, 0) == sramSize) {
				sramHash = curRAMHash(sramSize);

				// read rtc is we need it
				if (!rtcSize || Bfile_ReadFile_OS(hFile, &rtc, rtcSize, -1) == rtcSize) {
					Bfile_CloseFile_OS(hFile);
					return true;
				}
			}
		} else {
			// read each bank
			bool failed = false;
			for (unsigned char i = 0; i < mbc.numRamBanks * 2; i++) {
				if (Bfile_ReadFile_OS(hFile, &cachedBanks[i]->bank[0], 0x1000, i * 0x1000) != 0x1000) {
					failed = true;
				}
			}
			if (!failed) {
				sramHash = curRAMHash(sramSize);

				// read rtc is we need it
				if (!rtcSize || Bfile_ReadFile_OS(hFile, &rtc, rtcSize, -1) == rtcSize) {
					Bfile_CloseFile_OS(hFile);
					return true;
				}
			}
		}

		Bfile_CloseFile_OS(hFile);

		// didn't read correctly
		return false;
	}
	else {
		// no sram
		return false;
	}
}

// attempts to save SRAM for a game to the given file path
void trySaveSRAM(const char* filepath) {
	int sramSize = ramNibbleCount(mbc.ramType) * 256;
	int rtcSize = mbcIsRTC() ? sizeof(rtc_state) : 0;

	if (mbc.numRamBanks > 1) {
		sramSize = 8192 * mbc.numRamBanks;
	}
	unsigned int curHash = curRAMHash(sramSize);

	if (curHash != sramHash || rtcSize) {
		unsigned short pFile[256];
		Bfile_StrToName_ncpy(pFile, (const char*)filepath, strlen(filepath) + 2);

		int hFile = Bfile_OpenFile_OS(pFile, WRITE, 0); // Get handle
		if (hFile < 0) {
			// attempt to create
			size_t wantedSize = sramSize + rtcSize;
			if (Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &wantedSize))
				return;

			hFile = Bfile_OpenFile_OS(pFile, WRITE, 0); // Get handle
			if (hFile < 0) {
				// create didn't work!
				return;
			}
		}

		if (sramSize <= 8192) {
			Bfile_WriteFile_OS(hFile, sram, sramSize);
		} else {
			// write each bank
			for (unsigned char i = 0; i < mbc.numRamBanks * 2; i++) {
				Bfile_WriteFile_OS(hFile, &cachedBanks[i]->bank[0], 0x1000);
			}
		}

		if (rtcSize) {
			Bfile_WriteFile_OS(hFile, &rtc, rtcSize);
		}

		Bfile_CloseFile_OS(hFile);

		// now in sync with file system
		sramHash = curHash;
	}
}

// call after state save load for proper handling
void mbcOnStateLoad() {
	if (mbc.sramEnabled) {
		enableSRAM();
	} else {
		disableSRAM();
	}

	// invalidate memory areas for ROM bank
	for (int i = 0x40; i <= 0x7f; i++) {
		specialMap[i] |= 0x10;
	}

	if (getRAMSize() <= 8 * 1024) {
		sramHash = curRAMHash(getRAMSize());
	}
}