
#include "platform.h"
#include "keys.h"
#include "rom.h"
#include "memory.h"
#include "cgb_bootstrap.h"

#include "screen_play.h"

#if !TARGET_WINSIM
#include "Prizm_SafeOverClock.h"
#else
#define SetSafeClockSpeed(...) {}
#endif

bool overclocked = false;

void screen_play::setup() {
	loadedROM[0] = 0;
}

void screen_play::select() {
	// was ROM already loaded? we may need to switch
	if (loadedROM[0]) {
		if (strcmp(loadedROM, emulator.settings.selectedRom)) {
			unloadROM();
		}
		else {
			// still playing the same game
			play();
			return;
		}
	}

	// if we got here we need to reinit the ROM
	initRom();
}

void screen_play::deselect() {
	// shouldn't happen, but just in case:
	// undo overclock
	if (overclocked) {
		SetSafeClockSpeed(SCS_Normal);
		overclocked = false;
	}
}

void screen_play::handleSelect() {
	if (mbc.romFile) {
		play();
	}
}

void screen_play::initRom() {
	LoadVRAM_1();

	reset_printf();
	printf("Loading file \"%s\"...\n", emulator.settings.selectedRom);

	resetMemoryMaps();
	reset();

	if (!loadROM(emulator.settings.selectedRom)) {
		printf("Failed!\n");
	}

	strcpy(loadedROM, emulator.settings.selectedRom);
}

void screen_play::play() {
	if (emulator.settings.scaleToScreen) {
		DrawBG("\\\\fls0\\Prizoop\\fit.bmp");
	}
	else {
		DrawBG("\\\\fls0\\Prizoop\\1x1.bmp");
	}
	Bdisp_PutDisp_DD();

	// Apply settings
	if (emulator.settings.overclock) {
		// clock up and apply the quit handler
		if (emulator.settings.scaleToScreen) {
			SetSafeClockSpeed(SCS_Double);
		}
		else {
			SetSafeClockSpeed(SCS_Fast);
		}

		overclocked = true;
	} else {
		// just in case
		SetSafeClockSpeed(SCS_Normal);
	}

	// look for gameboy color palette override
	unsigned short palette[12];
	if (!emulator.settings.useCGBColors || !getCGBTableEntry(&memoryMap[0][ROM_OFFSET_NAME], palette)) {
		colorpalette_type pal;
		emulator.getPalette(emulator.settings.bgColorPalette, pal);
		memcpy(&palette[0], pal.colors, sizeof(pal.colors));

		emulator.getPalette(emulator.settings.obj1ColorPalette, pal);
		memcpy(&palette[4], pal.colors, sizeof(pal.colors));

		emulator.getPalette(emulator.settings.obj2ColorPalette, pal);
		memcpy(&palette[8], pal.colors, sizeof(pal.colors));
	}

	SetupDisplayPalette(palette);
	SetupDisplayDriver(emulator.settings.scaleToScreen, emulator.settings.frameSkip);

#if DEBUG
	// init debug timing system
	ScopeTimer::InitSystem();
#endif

	// wait for menu key to exit
	while (keyDown_fast(48) == false) {
		cpuStep();
	}
	while (keyDown_fast(48) == true);

	// save sram
	saveRAM();

	// undo overclock
	if (overclocked) {
		SetSafeClockSpeed(SCS_Normal);
		overclocked = false;
	}

#if DEBUG
	ScopeTimer::Shutdown();
#endif

	// switch to options screen
	emulator.tryScreenChange(2);
}