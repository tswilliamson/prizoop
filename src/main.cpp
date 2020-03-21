#include "platform.h"

#include "emulator.h"
#include "rom.h"
#include "ptune2_simple\Ptune2_direct.h"
#include "debug.h"

#include "calctype/fonts/arial_small/arial_small.c"		// For Menus
#include "calctype/fonts/consolas_intl/consolas_intl.c"	// for FAQS

bool shouldExit = false;

void shutdown() {
	emulator.shutDown();
	unloadROM();
}

struct colorconfig {
	const char* name;
	unsigned short col[8];
};

#if TARGET_WINSIM
int simmain(void) {
#else
int main(void) {
#endif
	// prepare for full color mode
	Bdisp_EnableColor(1);
	EnableStatusArea(3);

	// allocate cached mbc banks on the stack
	ALLOCATE_CACHED_BANKS();

	for (int b = 0; b < 256; b++) {
		unsigned char bytes[4] = {
			(unsigned char) (((b & 0x80) ? 4 : 0) | ((b & 0x08) ? 16 : 0)),
			(unsigned char) (((b & 0x40) ? 4 : 0) | ((b & 0x04) ? 16 : 0)),
			(unsigned char) (((b & 0x20) ? 4 : 0) | ((b & 0x02) ? 16 : 0)),
			(unsigned char) (((b & 0x10) ? 4 : 0) | ((b & 0x01) ? 16 : 0))
		};

		if (b % 8 == 0) {
			OutputLog("\t");
		}
		OutputLog("0x%02x, 0x%02x, 0x%02x, 0x%02x, ", bytes[0], bytes[1], bytes[2], bytes[3]);
		if (b % 8 == 7) {
			OutputLog("\n");
		}
	}
	OutputLog("\n\n");
	for (int b = 0; b < 256; b++) {
		unsigned char bytes[4] = {
			(unsigned char) (((b & 0x01) ? 4 : 0) | ((b & 0x10) ? 16 : 0)),
			(unsigned char) (((b & 0x02) ? 4 : 0) | ((b & 0x20) ? 16 : 0)),
			(unsigned char) (((b & 0x04) ? 4 : 0) | ((b & 0x40) ? 16 : 0)),
			(unsigned char) (((b & 0x08) ? 4 : 0) | ((b & 0x80) ? 16 : 0))
		};

		if (b % 8 == 0) {
			OutputLog("\t");
		}
		OutputLog("0x%02x, 0x%02x, 0x%02x, 0x%02x, ", bytes[0], bytes[1], bytes[2], bytes[3]);
		if (b % 8 == 7) {
			OutputLog("\n");
		}
	}

	reset_printf();
	memset(GetVRAMAddress(), 0, LCD_HEIGHT_PX * LCD_WIDTH_PX * 2);
	printf("Prizoop Initializing...");
	DrawFrame(0x0000);
	Bdisp_PutDisp_DD();

	SetQuitHandler(shutdown);

	emulator.startUp();
	emulator.run();

	return 0;
}

#if !TARGET_WINSIM
void* operator new(unsigned int size){
	return malloc(size);
}
void operator delete(void* addr) {
	free(addr);
}
void* operator new[](unsigned int size) {
	return malloc(size);
}
void operator delete[](void* addr) {
	free(addr);
}
DeviceType getDeviceType() {
	return (size_t) GetVRAMAddress() == 0xAC000000 ? DT_CG50 : DT_CG20;
}

#else
DeviceType getDeviceType() {
	return DT_Winsim;
}
#endif