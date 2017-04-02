
#include "platform.h"
#include "keys.h"

#include "screen_settings.h"

struct option_type {
	const char* name;
	int type;		// 0 = toggle, 1 = keys, 2 = frameskip, 3 = color
	void* addr;
};

static option_type options[] = {
	{ "Overclock", 0, &emulator.settings.overclock },
	{ "Fit To Screen", 0, &emulator.settings.scaleToScreen },
	{ "CGB Colors", 0, &emulator.settings.useCGBColors },
	{ "Clamp Speed", 0, &emulator.settings.clampSpeed },
	{ "Frameskip", 2, &emulator.settings.frameSkip },
	{ "BG Colors", 3, &emulator.settings.bgColorPalette },
	{ "Sprite 1 Colors", 3, &emulator.settings.obj1ColorPalette },
	{ "Sprite 2 Colors", 3, &emulator.settings.obj2ColorPalette },
	{ "Map Keys", 1, NULL },
};

static inline int NumOptions() {
	return sizeof(options) / sizeof(option_type);
}

void screen_settings::setup() {
	curOption = 0;
}

void screen_settings::select() {
	DrawBG("\\\\fls0\\Prizoop\\menu.bmp");
	SaveVRAM_1();

	drawOptions();
}

void screen_settings::deselect() {
	emulator.saveSettings();
}

void screen_settings::drawOptions() {
	for (int i = 0; i < NumOptions(); i++) {
		bool selected = curOption == i;
		int y = i * 18 + 4;

		Print(10, y, options[i].name, selected);

		if (options[i].type == 0) {
			unsigned char isOn = *((unsigned char*)options[i].addr);

			Print(200, y, "On", selected && isOn, isOn ? COLOR_WHITE : COLOR_DARKGRAY);
			Print(230, y, "Off", selected && !isOn, !isOn ? COLOR_WHITE : COLOR_DARKGRAY);
		}
		else if (options[i].type == 1) {
			// key map
		}
		else if (options[i].type == 2) {
			// frame skip
			char skip = *((char*)options[i].addr);

			if (skip == -1) {
				Print(200, y, "Auto", selected);
			}
			else {
				char buffer[5];
				memset(buffer, 0, sizeof(buffer));
				sprintf(buffer, "%d", skip);
				Print(200, y, buffer, selected);
			}
		}
		else if (options[i].type == 3) {
			// color palette
			colorpalette_type pal;
			emulator.getPalette(*((unsigned char*)options[i].addr), pal);

			// draw each color box
			for (int j = 0; j < 4; j++) {
				display_fill area;
				area.x1 = 200 + 18 * j; 
				area.x2 = area.x1 + 16;
				area.y1 = y + 1;
				area.y2 = y + 16;
				area.mode = 1;
				display_fill temp = area;
				Bdisp_AreaClr(&temp, 1, selected ? COLOR_LIGHTGREEN : COLOR_WHITE);
				area.x1++;
				area.x2--;
				area.y1++;
				area.y2--;
				Bdisp_AreaClr(&area, 1, pal.colors[j]);
			}
		}
	}
}

static const char* keyName[] = {
	"A", "B", "Select", "Start", "Right", "Left", "Up", "Down"
};

unsigned char getCurrentKey(bool wait = true) {
	unsigned char keyPressed = 0;
	do {
		for (unsigned int c = 25; c <= 79; c++) {
			// skip invalid keys
			if (c % 10 == 0 || c == 34)
				continue;

			if (keyDown_fast(c)) {
				keyPressed = c;
				// wait for key too lift
				while (keyDown_fast(keyPressed)) {}
				break;
			}
		}
	} while (keyPressed == 0 && wait);

	// wait a moment
	OS_InnerWait_ms(10);

	return keyPressed;
}

void screen_settings::selectKeys() {
	unsigned char newMap[emu_button::MAX];

	// wait for key release
	getCurrentKey(false);

	for (int i = 0; i < emu_button::MAX; i++) {
		display_fill area;
		area.x1 = 65;
		area.x2 = 318;
		area.y1 = 86;
		area.y2 = 132;
		area.mode = 1;
		Bdisp_AreaClr(&area, 0, COLOR_BLACK);

		area.x1 += 2;
		area.x2 -= 2;
		area.y1 += 2;
		area.y2 -= 2;
		Bdisp_AreaClr(&area, 0, COLOR_WHITE);

		const char* pressKey = "Press key for ";
		const char* exitPrompt = "Press MENU to cancel";

		int line1WidthA = PrintWidth(pressKey);
		int line1WidthB = PrintWidth(keyName[i]);
		int x = 192 - (line1WidthA + line1WidthB) / 2;

		Print(x, 90, pressKey, false, COLOR_BLACK);
		Print(x + line1WidthA, 90, keyName[i], true, COLOR_YELLOW);

		int line2Width = PrintWidth(exitPrompt);
		Print(192 - line2Width / 2, 110, exitPrompt, false, COLOR_DARKRED);

		Bdisp_PutDisp_DD();

		newMap[i] = getCurrentKey();

		// MENU = cancel
		if (newMap[i] == 48)
			return;
	}

	// copy to settings
	memcpy(emulator.settings.keyMap, newMap, sizeof(newMap));
}

void screen_settings::handleUp() {
	curOption = (curOption + NumOptions() - 1) % NumOptions();

	LoadVRAM_1();
	drawOptions();
}

void screen_settings::handleDown() {
	curOption = (curOption + 1) % NumOptions();

	LoadVRAM_1();
	drawOptions();
}

void screen_settings::handleSelect() {
	switch (options[curOption].type) {
		case 0:
		{
			unsigned char* isOn = ((unsigned char*)options[curOption].addr);
			*isOn = 1 - *isOn;
			break;
		}
		case 1:
		{
			selectKeys();
			break;
		}
		case 2:
		{
			char* skip = ((char*)options[curOption].addr);
			(*skip)++;
			if (*skip == 5) {
				*skip = -1;
			}
			break;
		}
		case 3:
		{
			unsigned char* palette = ((unsigned char*)options[curOption].addr);
			*palette = ((*palette) + 1) % emulator.numPalettes();
			break;
		}
	}

	LoadVRAM_1();
	drawOptions();
}

