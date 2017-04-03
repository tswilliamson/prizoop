#include "keys.h"
#include "emulator.h"
#include "debug.h"

struct keys_type keys;

#if !TARGET_WINSIM
// returns true if the key is down, false if up
bool keyDown_fast(unsigned char keyCode) {
	static const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;

	int row, col, word, bit;
	row = keyCode % 10;
	col = keyCode / 10 - 1;
	word = row >> 1;
	bit = col + 8 * (row & 1);
	return (keyboard_register[word] & (1 << bit));
}
#endif

static inline unsigned char getGBAKey(unsigned int keyNum) {
	DebugAssert(keyNum < emu_button::MAX);
	return keyDown_fast(emulator.settings.keyMap[keyNum]) ? 0 : 1;
}

void refresh() {
	{
		keys.k1.a = getGBAKey(0);
		keys.k1.b = getGBAKey(1);
		keys.k1.select = getGBAKey(2);
		keys.k1.start = getGBAKey(3);
		keys.k2.right = getGBAKey(4);
		keys.k2.left = getGBAKey(5);
		keys.k2.up = getGBAKey(6);
		keys.k2.down = getGBAKey(7);
	}

#if DEBUG
	if (keyDown_fast(79)) {
		ScopeTimer::DisplayTimes();
	}
#endif
}