#pragma once

#include "emulator.h"

struct screen_play : public emulator_screen {
	char loadedROM[32];

	virtual void setup() override;
	virtual void select() override;
	virtual void deselect() override;
	virtual void handleSelect() override;
	virtual void postStateChange() override;

private:
	void initRom();
	void play();
	void drawPlayBG();
};