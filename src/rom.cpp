
#include "platform.h"

#include "memory.h"
#include "debug.h"
#include "mbc.h"

#include "rom.h"

unsigned char loadROM(const char *filename) {
	char name[17];
	enum mbcType type;
	int i;
	
	int length;
	int key;
	
	unsigned char header[0x180];

	int hFile;
	unsigned short pFile[256];
	Bfile_StrToName_ncpy(pFile, (const char*)filename, strlen(filename)+2);
	
	hFile = Bfile_OpenFile_OS(pFile, READ, 0); // Get handle
	if (hFile < 0) {
		printf("Not found! %d\n", hFile);
		return 0;
	}

	length = Bfile_GetFileSize_OS(hFile);
	if(length < 0x180) {
		printf("ROM is too small! (%d bytes)\n", length);
		Bfile_CloseFile_OS(hFile);
		return 0;
	}

	// read in the header and print out info
	Bfile_ReadFile_OS(hFile, header, 0x180, 0);
	
	memset(name, '\0', 17);
	for(i = 0; i < 16; i++) {
		if(header[i + ROM_OFFSET_NAME] == 0x80 || header[i + ROM_OFFSET_NAME] == 0xc0) name[i] = '\0';
		else name[i] = header[i + ROM_OFFSET_NAME];
	}
	printf("ROM name: %s\n", name);

	// determine mbc controller support and initialize
	type = (mbcType) header[ROM_OFFSET_TYPE];
	unsigned char romSizeByte = header[ROM_OFFSET_ROM_SIZE];
	unsigned char ramSizeByte = header[ROM_OFFSET_RAM_SIZE];

	if (!setupMBCType(type, romSizeByte, ramSizeByte, hFile)) {
		printf("Unsupported MBC: %s, (%02x ROM, %02x RAM)\n", getMBCTypeString(type), romSizeByte, ramSizeByte);
		unloadROM();
		GetKey(&key);
		return 0;
	}

	printf("MBC type: %s\n", getMBCTypeString(type));
	printf("RAM type: %s\n", getRAMTypeString(mbc.ramType));
	printf("Num ROM Banks: %d\n", mbc.numRomBanks);
		
	// read permanent ROM Area in
	int read = Bfile_ReadFile_OS(hFile, cart, min(length, 0x4000), 0);

	GetKey(&key);

	return 1;
}

void unloadROM(void) {
	if (mbc.romFile) {
		Bfile_CloseFile_OS(mbc.romFile);
		mbc.romFile = 0;
	}
}
