#include "platform.h"

#include "rom.h"
#include "cpu.h"
#include "gpu.h"
#include "interrupts.h"
#include "debug.h"
#include "keys.h"
#include "memory.h"

#include "Prizm_SafeOverClock.h"

bool shouldExit = false;
bool overclocked = false;

void clockdown() {
	if (overclocked) {
		SetSafeClockSpeed(SCS_Normal);
		overclocked = false;
	}

	ScopeTimer::Shutdown();
}

// returns 0 if the key is down, 1 if up (to match gameboy conventions)
unsigned char keyDown_fast(int keyCode) {
	static const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;

	int row, col, word, bit;
	row = keyCode % 10;
	col = keyCode / 10 - 1;
	word = row >> 1;
	bit = col + 8 * (row & 1);
	return (0 != (keyboard_register[word] & 1 << bit)) ? 0 : 1;
}

struct colorconfig {
	const char* name;
	unsigned short col[4];
};

int main(void) {
	int key;

	// prepare for full color mode
	Bdisp_EnableColor(1);
	EnableStatusArea(3);
	Bdisp_AllClr_VRAM();
	Bdisp_PutDisp_DD();

	// figure out options
	bool overclock = false;
	bool scale = true;
	char frameskip = 2;
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
}


void refresh() {
	{
		keys.k1.a = keyDown_fast(78);	// SHIFT
		keys.k1.b = keyDown_fast(77);	// ALPHA
		keys.k1.select = keyDown_fast(68); // OPTN
		keys.k1.start = keyDown_fast(58); // VARS
		keys.k2.right = keyDown_fast(27);
		keys.k2.left = keyDown_fast(38);
		keys.k2.up = keyDown_fast(28);
		keys.k2.down = keyDown_fast(37);
	}

	if (keyDown_fast(79) == 0) {
		ScopeTimer::DisplayTimes();
	}

	if (keyDown_fast(48) == 0) {
		// MENU key initiate a GetKey to let the app escape
		int key;
		GetKey(&key);
	}
}
