
#include "platform.h"
#include "keys.h"
#include "rom.h"
#include "memory.h"
#include "cgb_bootstrap.h"
#include "ptune2_simple/Ptune2_direct.h"

#include "screen_play.h"

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

void screen_play::drawPlayBG() {
	if (emulator.settings.scaleToScreen) {
		DrawBGEmbedded((unsigned char*)bg_fit);
	}
	else {
		DrawBGEmbedded((unsigned char*)bg_1x1);
	}
	Bdisp_PutDisp_DD();
}

void screen_play::postStateChange() {
	drawPlayBG();
}

void screen_play::play() {
	mbcFileUpdate();

	drawPlayBG();

	// Apply settings
	bool doOverclock = Ptune2_GetSetting() == PT2_DEFAULT;
	if (doOverclock) {
		if (emulator.settings.overclock) {
			// clock up and apply the quit handler
			if (emulator.settings.scaleToScreen) {
				Ptune2_LoadSetting(PT2_DOUBLE);
			} else {
				Ptune2_LoadSetting(PT2_HALFINC);
			}
		} else {
			Ptune2_LoadSetting(PT2_NOMEMWAIT);
		}
	}

	// look for gameboy color palette override
	unsigned int palette[12];
	if (!emulator.settings.useCGBColors || !getCGBTableEntry(&memoryMap[0][ROM_OFFSET_NAME], palette)) {
		colorpalette_type pal;
		emulator.getPalette(emulator.settings.bgColorPalette, pal);
		for (int i = 0; i < 4; i++) {
			palette[i] = pal.colors[i] | (pal.colors[i] << 16);
		}

		emulator.getPalette(emulator.settings.obj1ColorPalette, pal);
		for (int i = 0; i < 4; i++) {
			palette[i+4] = pal.colors[i] | (pal.colors[i] << 16);
		}

		emulator.getPalette(emulator.settings.obj2ColorPalette, pal);
		for (int i = 0; i < 4; i++) {
			palette[i+8] = pal.colors[i] | (pal.colors[i] << 16);
		}
	}

	SetupDisplayPalette(palette);
	SetupDisplayDriver(emulator.settings.scaleToScreen, emulator.settings.frameSkip);

#if DEBUG
	// init debug timing system
	ScopeTimer::InitSystem();
#endif

	// wait for exit "key" to exit (This actually renders a preview screen first)
	keys.exit = false;
	while (keys.exit == false) {
		cpuStep();
	}

	// make sure menu isn't pressed
	while (keyDown_fast(48) == true);

	// save sram
	saveRAM();

	// undo overclock
	if (doOverclock) {
		Ptune2_LoadSetting(PT2_DEFAULT);
	}

#if DEBUG
	ScopeTimer::Shutdown();
#endif

	// switch to options screen
	emulator.tryScreenChange(2);
}