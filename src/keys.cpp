#include "keys.h"
#include "emulator.h"
#include "debug.h"
#include "gpu.h"
#include "snd.h"

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
		keys.k1.a = getGBAKey(emu_button::A);
		keys.k1.b = getGBAKey(emu_button::B);
		keys.k1.select = getGBAKey(emu_button::SELECT);
		keys.k1.start = getGBAKey(emu_button::START);
		keys.k2.right = getGBAKey(emu_button::RIGHT);
		keys.k2.left = getGBAKey(emu_button::LEFT);
		keys.k2.up = getGBAKey(emu_button::UP);
		keys.k2.down = getGBAKey(emu_button::DOWN);
	}

	if (keyDown_fast(emulator.settings.keyMap[emu_button::STATE_SAVE])) {
		emulator.saveState();
		while (keyDown_fast(emulator.settings.keyMap[emu_button::STATE_SAVE])) {}
	}

	if (keyDown_fast(emulator.settings.keyMap[emu_button::STATE_LOAD])) {
		emulator.loadState();
		while (keyDown_fast(emulator.settings.keyMap[emu_button::STATE_LOAD])) {}
	}

	if (keyDown_fast(48)) {
		// this will set keys.exit once a full frame renders
		enablePausePreview();
	}

	// sound volume controls
	if (emulator.settings.sound && framecounter % 8 == 0) {
		if (keyDown_fast(42)) sndVolumeUp();
		if (keyDown_fast(32)) sndVolumeDown();
	}

#if DEBUG
	if (keyDown_fast(79)) {
		ScopeTimer::DisplayTimes();
	}
#endif
}