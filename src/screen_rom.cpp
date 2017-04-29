
#include "platform.h"
#include "screen_rom.h"

typedef struct
{
	unsigned short id, type;
	unsigned long fsize, dsize;
	unsigned int property;
	unsigned long address;
} file_type_t;

static void FindFiles(const char* path, foundFile* toArray, int& numFound) {
	unsigned short filter[0x100], found[0x100];
	int ret, handle;
	file_type_t info; // See Bfile_FindFirst for the definition of this struct

	Bfile_StrToName_ncpy(filter, path, 0x50); // Overkill

	ret = Bfile_FindFirst((const char*)filter, &handle, (char*)found, &info);

	while (ret == 0 && numFound < 64) {
		Bfile_NameToStr_ncpy(toArray[numFound++].path, found, 32);
		ret = Bfile_FindNext(handle, (char*)found, (char*)&info);
	};

	Bfile_FindClose(handle);
}

void screen_rom::discoverFiles() {
	files = new foundFile[64];
	numFiles = 0;
	FindFiles("\\\\fls0\\*.gbz", files, numFiles);			// compressed gameboy ROM with utility
	FindFiles("\\\\fls0\\*.gbc", files, numFiles);			// gameboy color
	FindFiles("\\\\fls0\\*.gb", files, numFiles);			// gameboy
	// TODO : These don't work on Prizm?
	// FindFiles("\\\\fls0\\Games\\*.gb", files, numFiles);
	// FindFiles("\\\\fls0\\ROMS\\*.gb", files, numFiles);
}

void screen_rom::drawFiles() {
	for (int i = 0; i < numFiles; i++) {
		int curY = i * 18 - curScroll;

		if (curY > -18 && curY < 180) {
			char buffer[16];
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "%d/%d", i + 1, numFiles);
			int width = PrintWidth(buffer);
			Print(10, curY, buffer, i == selectedFile);
			Print(10 + width + 10, curY, files[i].path, i == selectedFile);
		}
	}
	DrawFrame(0);
}

void screen_rom::setup() {
	loadedFile = -1;
}

void screen_rom::select() {
	// file discovery doesn't work well when rom file is currently active, so we close it
	loadedFile = -1;
	bool wasLoaded = mbc.romFile != 0;
	if (wasLoaded) {
		Bfile_CloseFile_OS(mbc.romFile);
		mbc.romFile = 0;
	}

	discoverFiles();

	DrawBGEmbedded((unsigned char*) bg_menu);
	DrawPausePreview();
	SaveVRAM_1();

	// if our selected file from settings is in the rom list
	selectedFile = 0;
	for (int i = 0; i < numFiles; i++) {
		if (!strcmp(files[i].path, emulator.settings.selectedRom)) {
			selectedFile = i;
			if (wasLoaded) {
				loadedFile = i;
			}
			break;
		}
	}
	
	if (numFiles) {
		checkScroll();
		drawFiles();
	} else {
		int x = 10;
		int y = 10;
		PrintMini(&x, &y, "No .gb files found", 0x42, 0xffffffff, 0, 0, COLOR_RED, COLOR_BLACK, 1, 0);
	}
}

void screen_rom::deselect() {
	if (numFiles) {
		strcpy(emulator.settings.selectedRom, files[selectedFile].path);
		if (selectedFile == loadedFile) {
			char romFile[64];
			unsigned short pFile[256];
			strcpy(romFile, "\\\\fls0\\");
			strcat(romFile, files[selectedFile].path);
			Bfile_StrToName_ncpy(pFile, (const char*) romFile, strlen(romFile) + 2);
			mbc.romFile = Bfile_OpenFile_OS(pFile, READ, 0);
		}
	}

	delete[] files;
	files = NULL;

	emulator.saveSettings();
}

bool screen_rom::checkScroll() {
	int selectY = selectedFile * 18 - curScroll;

	if (selectY < 0) {
		curScroll = 18 * selectedFile - 4;
		return true;
	}
	if (selectY > 170) {
		curScroll = 18 * selectedFile - 166;
		return true;
	}
	return false;
}

void screen_rom::handleUp() {
	if (numFiles) {
		selectedFile = (selectedFile + numFiles - 1) % numFiles;

		checkScroll();
		LoadVRAM_1();
		drawFiles();
	}
}

void screen_rom::handleDown() {
	if (numFiles) {
		selectedFile = (selectedFile + 1) % numFiles;

		checkScroll();
		LoadVRAM_1();
		drawFiles();
	}
}