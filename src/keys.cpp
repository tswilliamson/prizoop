#include "keys.h"

struct keys keys;

#if !TARGET_WINSIM
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

void refresh() {
	{
		keys.k1.a = keyDown_fast(78);	// SHIFT
		keys.k1.b = keyDown_fast(68);	// OPTN
		keys.k1.select = keyDown_fast(39); // F5
		keys.k1.start = keyDown_fast(29); // F6
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
#endif