#include "platform.h"

#include "emulator.h"
#include "rom.h"
#include "ptune2_simple\Ptune2_direct.h"

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