#ifndef PS4
	#include <stdio.h>
	#include <string.h>
	#include <math.h>
#endif

#include "platform.h"

#include "memory.h"
#include "debug.h"

#include "rom.h"

#ifdef DS3
	Handle fileHandle;
#endif

const char *internal_romTypeString[256] = {0};

void initializeRomTypes() {
	internal_romTypeString[ROM_PLAIN] = "ROM_PLAIN";
	internal_romTypeString[ROM_MBC1] = "ROM_MBC1";
	internal_romTypeString[ROM_MBC1_RAM] = "ROM_MBC1";
	internal_romTypeString[ROM_MBC1_RAM_BATT] = "ROM_MBC1_RAM_BATT";
	internal_romTypeString[ROM_MBC2] = "ROM_MBC2";
	internal_romTypeString[ROM_MBC2_BATTERY] = "ROM_MBC2_BATTERY";
	internal_romTypeString[ROM_RAM] = "ROM_RAM";
	internal_romTypeString[ROM_RAM_BATTERY] = "ROM_RAM_BATTERY";
	internal_romTypeString[ROM_MMM01] = "ROM_MMM01";
	internal_romTypeString[ROM_MMM01_SRAM] = "ROM_MMM01_SRAM";
	internal_romTypeString[ROM_MMM01_SRAM_BATT] = "ROM_MMM01_SRAM_BATT";
	internal_romTypeString[ROM_MBC3_TIMER_BATT] = "ROM_MBC3_TIMER_BATT";
	internal_romTypeString[ROM_MBC3_TIMER_RAM_BATT] = "ROM_MBC3_TIMER_RAM_BATT";
	internal_romTypeString[ROM_MBC3] = "ROM_MBC3";
	internal_romTypeString[ROM_MBC3_RAM] = "ROM_MBC3_RAM";
	internal_romTypeString[ROM_MBC3_RAM_BATT] = "ROM_MBC3_RAM_BATT";
	internal_romTypeString[ROM_MBC5] = "ROM_MBC5";
	internal_romTypeString[ROM_MBC5_RAM] = "ROM_MBC5_RAM";
	internal_romTypeString[ROM_MBC5_RAM_BATT] = "ROM_MBC5_RAM_BATT";
	internal_romTypeString[ROM_MBC5_RUMBLE] = "ROM_MBC5_RUMBLE";
	internal_romTypeString[ROM_MBC5_RUMBLE_SRAM] = "ROM_MBC5_RUMBLE_SRAM";
	internal_romTypeString[ROM_MBC5_RUMBLE_SRAM_BATT] = "ROM_MBC5_RUMBLE_SRAM_BATT";
	internal_romTypeString[ROM_POCKET_CAMERA] = "ROM_POCKET_CAMERA";
	internal_romTypeString[ROM_BANDAI_TAMA5] = "ROM_BANDAI_TAMA5";
	internal_romTypeString[ROM_HUDSON_HUC3] = "ROM_HUDSON_HUC3";
	internal_romTypeString[ROM_HUDSON_HUC1] = "ROM_HUDSON_HUC1";
}

const char* romTypeString(int index) {
	if (internal_romTypeString[0] == NULL) {
		initializeRomTypes();
	}
	return internal_romTypeString[index];
}

unsigned char loadROM(const char *filename) {
	char name[17];
	enum romType type;
	int romSize;
	int ramSize;
	int i;
	
	int length;
	
	unsigned char header[0x180];

	int hFile;
	unsigned short pFile[256];
	Bfile_StrToName_ncpy(pFile, (const char*)filename, strlen(filename)+2);

	//char buffer[64];
	//Bfile_NameToStr_ncpy(buffer, pFile, sizeof(pFile) / 2);
	//printf("Filename '%s'", buffer);

	hFile = Bfile_OpenFile_OS(pFile, READ, 0); // Get handle
	if (hFile < 0) {
		printf("Not found! %d\n", hFile);
		return 0;
	}

	length = Bfile_GetFileSize_OS(hFile);
	printf("found! %d bytes", length);

	if(length < 0x180) {
		printf("ROM is too small!\n");
		Bfile_CloseFile_OS(hFile);
		return 0;
	}

	Bfile_ReadFile_OS(hFile, header, 0x180, 0);
	
	memset(name, '\0', 17);
	for(i = 0; i < 16; i++) {
		if(header[i + ROM_OFFSET_NAME] == 0x80 || header[i + ROM_OFFSET_NAME] == 0xc0) name[i] = '\0';
		else name[i] = header[i + ROM_OFFSET_NAME];
	}
	
	printf("Internal ROM name: %s\n", name);

	type = (romType) header[ROM_OFFSET_TYPE];
	
	if(!romTypeString(type)) {
		printf("Unknown ROM type: %#02x\n", type);
		Bfile_CloseFile_OS(hFile);
		return 0;
	}
	
	printf("ROM type: %s\n", romTypeString(type));
	
	if(type != ROM_PLAIN) {
		printf("Only 32KB games with no mappers are supported!\n");
		//Bfile_CloseFile_OS(hFile);
		//return 0;
	}
	
	romSize = header[ROM_OFFSET_ROM_SIZE];
	
	//#ifndef PSP
		//if((romSize & 0xF0) == 0x50) romSize = (int)pow(2.0, (double)(((0x52) & 0xF) + 1)) + 64;
		//else romSize = (int)pow(2.0, (double)(romSize + 1));
	//#else
		// PSP doesn't support pow...
		romSize = 2;
	//#endif
	
	printf("ROM size: %dKB\n", romSize * 16);
	
	if(romSize * 16 != 32) {
		printf("Only 32KB games with no mappers are supported!\n");
		Bfile_CloseFile_OS(hFile);
		return 0;
	}
	
	if(length != romSize * 16 * 1024) {
		printf("ROM filesize does not equal ROM size!\n");
		//fclose(f);
		//return 0;
	}

	ramSize = header[ROM_OFFSET_RAM_SIZE];
	
	//#ifndef PSP
		//ramSize = (int)pow(4.0, (double)(ramSize)) / 2;
	//#else
		// PSP doesn't support pow...
		ramSize = 0;
	//#endif
	
	printf("RAM size: %dKB\n", ramSize);
	
	// ramSize = ceil(ramSize / 8.0f);
	
	/*cart = malloc(length);
	if(!cart) {
		printf("Could not allocate memory!\n");
		fclose(f);
		return 0;
	}*/

	int read = Bfile_ReadFile_OS(hFile, cart, length, 0);
	printf("Read bytes : %d\n", read);

	Bfile_CloseFile_OS(hFile);
		
	return 1;
}

void unloadROM(void) {
	//free(cart);
}
