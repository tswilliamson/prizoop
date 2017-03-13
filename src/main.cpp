#include "platform.h"

#include "rom.h"
#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "debug.h"
#include "keys.h"
#include "memory.h"

#if !TARGET_WINSIM
#include "Prizm_SafeOverClock.h"
#else
#define SetSafeClockSpeed(...) {}
#endif

bool shouldExit = false;
bool overclocked = false;

void clockdown() {
	if (overclocked) {
		SetSafeClockSpeed(SCS_Normal);
		overclocked = false;
	}

	ScopeTimer::Shutdown();
}

struct colorconfig {
	const char* name;
	unsigned short col[4];
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

	// figure out options
	bool overclock = false;
	bool scale = true;
	char frameskip = 0;
	int colorScheme = 0;
	const colorconfig colorSchemes[] = {
		{ "Cyan",		{ COLOR_WHITE, COLOR_LIGHTCYAN, COLOR_CYAN, COLOR_DARKCYAN } },
		{ "B&W",		{ COLOR_WHITE, COLOR_LIGHTGRAY, COLOR_DARKGRAY, COLOR_BLACK } },
		{ "Classic",	{ COLOR_LIGHTGREEN, COLOR_MEDIUMSEAGREEN, COLOR_DARKGREEN, COLOR_BLACK } },
		{ "Red",		{ COLOR_WHITE, COLOR_YELLOW, COLOR_RED, COLOR_BLACK } },
		{ "Lollipop",	{ COLOR_MINTCREAM, COLOR_LIGHTGREEN, COLOR_BLUE, COLOR_DARKRED } },
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
	SetQuitHandler(clockdown);

	SetupDisplayDriver(scale, frameskip);
	SetupDisplayColors(
		colorSchemes[colorScheme].col[0],
		colorSchemes[colorScheme].col[1],
		colorSchemes[colorScheme].col[2],
		colorSchemes[colorScheme].col[3]
	);

	const char* filename = "\\\\fls0\\tetris.gb";
	printf("Loading file \"%s\"...\n", filename);

	if (!loadROM(filename)) {
		printf("Failed!\n");
		GetKey(&key);
		return 1;
	}

	SetupMemoryMaps();
	reset();

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

