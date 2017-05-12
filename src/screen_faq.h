#pragma once


#include "emulator.h"

struct screen_faq : public emulator_screen {
	int faqHandle;		// file handle to faq file (0 if invalid)		

	int faqSize;
	int readOffset;		// buffer offset for current line (up/down)
	int x;				// x amount on current faq line (left/right)

	// current loaded text buffer and its offset
	int textOffset;
	int textSize;
	char* textBuffer;	

	virtual void setup() override;
	virtual void select() override;
	virtual void deselect() override;

	// key press handles
	virtual void handleUp() override;
	virtual void handleDown() override;
	virtual void handleLeft() override;
	virtual void handleRight() override;

private:
	void draw();
	void loadFAQ();
	void readText(int bufferPos);
};