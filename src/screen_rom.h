#pragma once

#include "emulator.h"

struct screen_rom : public emulator_screen {

	virtual void setup() override;
	virtual void select() override;
	virtual void deselect() override;

	// key press handles
	virtual void handleUp() override;
	virtual void handleDown() override;
	virtual void handleLeft() override;
	virtual void handleRight() override;
	virtual void handleSelect() override;
};