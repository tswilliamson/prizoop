#include "platform.h"

#include "emulator.h"

#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "debug.h"
#include "keys.h"
#include "memory.h"
#include "rom.h"
#include "bmp.h"

#if !TARGET_WINSIM
#include "Prizm_SafeOverClock.h"
#else
#define SetSafeClockSpeed(...) {}
#endif

bool shouldExit = false;
bool overclocked = false;

void shutdown() {
	if (overclocked) {
		SetSafeClockSpeed(SCS_Normal);
		overclocked = false;
	}

	unloadROM();

	ScopeTimer::Shutdown();
}

struct colorconfig {
	const char* name;
	unsigned short col[8];
};

struct foundFile {
	char path[32];
};

typedef struct
{
	unsigned short id, type;
	unsigned long fsize, dsize;
	unsigned int property;
	unsigned long address;
} file_type_t;

void FindFiles(const char* path, foundFile* toArray, int& numFound) {
	unsigned short filter[0x100], found[0x100];
	int ret, handle;
	file_type_t info; // See Bfile_FindFirst for the definition of this struct

	Bfile_StrToName_ncpy(filter, path, 0x50); // Overkill

	ret = Bfile_FindFirst((const char*) filter, &handle, (char*) found, &info);

	while (ret == 0) {
		Bfile_NameToStr_ncpy(toArray[numFound++].path, found, 32);
		ret = Bfile_FindNext(handle, (char*) found, (char*) &info);
	};

	Bfile_FindClose(handle);
}

#if TARGET_WINSIM
int simmain(void) {
#else
int main(void) {
#endif
	int key;

	// prepare for full color mode
	Bdisp_EnableColor(1);
	EnableStatusArea(3);


	// allocate cached rom banks on the stack
	ALLOCATE_ROM_BANKS();

	// collect ROM files
	foundFile files[16];
	int numFiles = 0;
	FindFiles("\\\\fls0\\*.gb", files, numFiles);
	FindFiles("\\\\fls0\\Games\\*.gb", files, numFiles);
	FindFiles("\\\\fls0\\ROMS\\*.gb", files, numFiles);

	reset_printf();
	memset(GetVRAMAddress(), 0, LCD_HEIGHT_PX * LCD_WIDTH_PX * 2);
	printf("Prizoop Initializing...");
	DrawFrame(0x0000);
	Bdisp_PutDisp_DD();

	if (numFiles == 0) {
		printf("No .gb files found in root!");
		GetKey(&key);
		return 1;
	}

	// show emulator options screen

	// figure out options
	int rom = 0;
	bool overclock = true;
	bool scale = true;
	char frameskip = 1;
#if TARGET_WINSIM
	frameskip = 0;
#endif
	int colorScheme = 2;
	int atX = 0;
	int atY = 0;
	int srcY = 0;
	int destHeight = -1;
	const colorconfig colorSchemes[] = {
		{ "Cyan",		{ COLOR_LIGHTCYAN, COLOR_CYAN, COLOR_DARKCYAN, COLOR_BLACK, COLOR_WHITE, COLOR_CYAN, COLOR_MEDIUMBLUE, COLOR_BLACK } },
		{ "B&W",		{ COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_DARKGRAY, COLOR_BLACK, COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_SLATEGRAY, COLOR_BLACK } },
		{ "Classic",	{ COLOR_LIGHTGREEN, COLOR_MEDIUMSEAGREEN, COLOR_DARKGREEN, COLOR_BLACK, COLOR_WHITE, COLOR_MEDIUMSEAGREEN, COLOR_DARKGREEN, COLOR_BLACK } },
		{ "Red",		{ COLOR_WHITE, COLOR_YELLOW, COLOR_SANDYBROWN, COLOR_BROWN, COLOR_WHITE, COLOR_ORANGE, COLOR_RED, COLOR_BLACK } },
		{ "Lollipop",	{ COLOR_LIGHTCYAN, COLOR_CYAN, COLOR_DARKCYAN, COLOR_BLACK, COLOR_WHITE, COLOR_YELLOW, COLOR_ORANGE, COLOR_DARKRED } },
	};
	do {
		PutBMP("\\\\fls0\\Prizoop\\menu.bmp", atX, atY, srcY, destHeight);
		reset_printf();
		printf("\n");
		printf("F1 - ROM : %s (%d/%d)", files[rom].path, rom+1, numFiles);
		printf("F2 - Overclock: %s\n", overclock ? "Yes" : "No");
		printf("F3 - Fit to screen: %s\n", scale ? "Yes" : "No");
		if (frameskip == -1) {
			printf("F4 - Frameskip: Auto");
		} else {
			printf("F4 - Frameskip: %d", frameskip);
		}
		printf("F5 - Color Scheme: %s", colorSchemes[colorScheme].name);
		printf("F6 - Start");

		DrawFrame(0x0000);
		Bdisp_PutDisp_DD();


		GetKey(&key);

		destHeight = 17;
		switch (key) {
			case KEY_CTRL_F1:
				rom = (rom + 1) % numFiles;
				srcY = 18 * 1;
				break;
			case KEY_CTRL_F2:
				overclock = !overclock;
				srcY = 18 * 2;
				break;
			case KEY_CTRL_F3:
				scale = !scale;
				srcY = 18 * 3;
				break;
			case KEY_CTRL_F4:
				frameskip = frameskip + 1;
				if (frameskip == 5)
					frameskip = -1;
				srcY = 18 * 4;
				break;
			case KEY_CTRL_F5:
				colorScheme = (colorScheme + 1) % (sizeof(colorSchemes) / sizeof(colorSchemes[0]));
				srcY = 18 * 5;
				break;
			case KEY_CTRL_F6:
				shouldExit = true;
		}
		atY = srcY;
	} while (!shouldExit);
	shouldExit = false;

	// init debug timing system
	ScopeTimer::InitSystem();

	// Apply settings
	if (overclock) {
		// clock up and apply the quit handler
		if (scale) {
			SetSafeClockSpeed(SCS_Double);
		} else {
			SetSafeClockSpeed(SCS_Fast);
		}

		overclocked = true;
	}
	else {
		// just in case
		SetSafeClockSpeed(SCS_Normal);
	}

	SetQuitHandler(shutdown);

	SetupDisplayDriver(scale, frameskip);
	SetupDisplayColors(
		colorSchemes[colorScheme].col[0],
		colorSchemes[colorScheme].col[1],
		colorSchemes[colorScheme].col[2],
		colorSchemes[colorScheme].col[3],
		colorSchemes[colorScheme].col[4],
		colorSchemes[colorScheme].col[5],
		colorSchemes[colorScheme].col[6],
		colorSchemes[colorScheme].col[7]
	);

	PutBMP("\\\\fls0\\Prizoop\\debug.bmp");
	reset_printf();
	printf("Loading file \"%s\"...\n", files[rom].path);

	resetMemoryMaps();
	reset();

	if (!loadROM(files[rom].path)) {
		printf("Failed!\n");
		GetKey(&key);
		return 1;
	}

	if (scale) {
		PutBMP("\\\\fls0\\Prizoop\\fit.bmp");
	} else {
		PutBMP("\\\\fls0\\Prizoop\\1x1.bmp");
	}
	Bdisp_PutDisp_DD();

	// a somewhat forcibly unrolled loop
	while (shouldExit == false) {
		cpuStep();
	}

	return 0;
}

