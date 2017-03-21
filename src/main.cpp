#include "platform.h"

#include "emulator.h"

#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "debug.h"
#include "keys.h"
#include "memory.h"
#include "rom.h"

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

#if TARGET_WINSIM
int simmain(void) {
#else
int main(void) {
#endif
	int key;

	// prepare for full color mode
	Bdisp_EnableColor(1);
	EnableStatusArea(3);
	Bdisp_AllClr_VRAM();
	Bdisp_PutDisp_DD();

	// allocate cached rom banks on the stack
	ALLOCATE_ROM_BANKS();

	// show emulator options screen


	// figure out options
	bool overclock = false;
	bool scale = true;
	char frameskip = 0;
	int colorScheme = 0;
	const colorconfig colorSchemes[] = {
		{ "Cyan",		{ COLOR_LIGHTCYAN, COLOR_CYAN, COLOR_DARKCYAN, COLOR_BLACK, COLOR_WHITE, COLOR_CYAN, COLOR_MEDIUMBLUE, COLOR_BLACK } },
		{ "B&W",		{ COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_DARKGRAY, COLOR_BLACK, COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_SLATEGRAY, COLOR_BLACK } },
		{ "Classic",	{ COLOR_LIGHTGREEN, COLOR_MEDIUMSEAGREEN, COLOR_DARKGREEN, COLOR_BLACK, COLOR_WHITE, COLOR_MEDIUMSEAGREEN, COLOR_DARKGREEN, COLOR_BLACK } },
		{ "Red",		{ COLOR_WHITE, COLOR_YELLOW, COLOR_SANDYBROWN, COLOR_BROWN, COLOR_WHITE, COLOR_ORANGE, COLOR_RED, COLOR_BLACK } },
		{ "Lollipop",	{ COLOR_LIGHTCYAN, COLOR_CYAN, COLOR_DARKCYAN, COLOR_BLACK, COLOR_WHITE, COLOR_YELLOW, COLOR_ORANGE, COLOR_DARKRED } },
	};
	do {
		reset_printf();
		printf("Prizoop!\n");
		printf("F1 - Overclock: %s\n", overclock ? "Yes" : "No");
		printf("F2 - Fit to screen: %s\n", scale ? "Yes" : "No");
		if (frameskip == -1) {
			printf("F3 - Frameskip: Auto");
		} else {
			printf("F3 - Frameskip: %d", frameskip);
		}
		printf("F4 - Color Scheme: %s", colorSchemes[colorScheme].name);
		printf("F6 - Start");

		GetKey(&key);

		switch (key) {
			case  KEY_CTRL_F1:
				overclock = !overclock;
				break;
			case  KEY_CTRL_F2:
				scale = !scale;
				break;
			case  KEY_CTRL_F3:
				frameskip = frameskip + 1;
				if (frameskip == 5)
					frameskip = -1;
				break;
			case  KEY_CTRL_F4:
				colorScheme = (colorScheme + 1) % (sizeof(colorSchemes) / sizeof(colorSchemes[0]));
				break;
			case KEY_CTRL_F6:
				shouldExit = true;
		}
	} while (!shouldExit);
	shouldExit = false;

	// one more clear
	Bdisp_AllClr_VRAM();
	Bdisp_PutDisp_DD();

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

	const char* filename = "\\\\fls0\\DKLand.gb";
	printf("Loading file \"%s\"...\n", filename);

	resetMemoryMaps();
	reset();

	if (!loadROM(filename)) {
		printf("Failed!\n");
		GetKey(&key);
		return 1;
	}

	// a somewhat forcibly unrolled loop
	while (shouldExit == false) {
		{
			TIME_SCOPE_NAMED("Main");
			cpuStep();
			gpuStep();
			interruptStep();
			cpuStep();
			gpuStep();
			interruptStep();
			cpuStep();
			gpuStep();
			interruptStep();
			cpuStep();
			gpuStep();
			interruptStep();
			cpuStep();
			gpuStep();
			interruptStep();
			cpuStep();
			gpuStep();
			interruptStep();
			cpuStep();
			gpuStep();
			interruptStep();
		}
	}

	return 0;
}

