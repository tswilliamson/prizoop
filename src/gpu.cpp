#ifndef PS4
	#include <stddef.h>
#endif

#include "platform.h"
#include "display.h"
#include "memory.h"
#include "cpu.h"
#include "interrupts.h"

#include "gpu.h"

struct gpu_type gpu;

tilestype* tiles = NULL;

unsigned char backgroundPalette[4];
unsigned char spritePalette[2][4];

/* References:
http://www.codeslinger.co.uk/pages/projects/gameboy/lcd.html
http://www.codeslinger.co.uk/pages/projects/gameboy/graphics.html
http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-Graphics
*/

void gpuStep(void) {
	enum gpuMode {
		GPU_MODE_HBLANK = 0,
		GPU_MODE_VBLANK = 1,
		GPU_MODE_OAM = 2,
		GPU_MODE_VRAM = 3,
	} static gpuMode = GPU_MODE_HBLANK;
	
	static int lastTicks = 0;
	
	gpu.tick += cpu.ticks - lastTicks;
	
	lastTicks = cpu.ticks;
	
	switch(gpuMode) {
		case GPU_MODE_HBLANK:
			if(gpu.tick >= 204) {
				hblank();
				
				if(cpu.memory.LY_lcdline == 144) {
					if (drawFramebuffer) {
						drawFramebuffer();
					}
					if(interrupt.enable & INTERRUPTS_VBLANK) interrupt.flags |= INTERRUPTS_VBLANK;
					
					gpuMode = GPU_MODE_VBLANK;
				}
				
				else gpuMode = GPU_MODE_OAM;
				
				gpu.tick -= 204;
			}
			
			break;
		
		case GPU_MODE_VBLANK:
			if(gpu.tick >= 456) {
				cpu.memory.LY_lcdline++;
				
				if(cpu.memory.LY_lcdline > 153) {
					cpu.memory.LY_lcdline = 0;
					gpuMode = GPU_MODE_OAM;
				}
				
				gpu.tick -= 456;
			}
			
			break;
		
		case GPU_MODE_OAM:
			if(gpu.tick >= 80) {
				gpuMode = GPU_MODE_VRAM;
				
				gpu.tick -= 80;
			}
			
			break;
		
		case GPU_MODE_VRAM:
			if(gpu.tick >= 172) {
				gpuMode = GPU_MODE_HBLANK;
				
				#ifndef DS
					renderScanline();
				#endif
				
				gpu.tick -= 172;
			}
			
			break;
	}
}

void hblank(void) {
	cpu.memory.LY_lcdline++;
}

void updateTile(unsigned short address, unsigned char value) {
	TIME_SCOPE();

	address &= 0x1ffe;
	
	unsigned short tile = (address >> 4) & 511;
	unsigned short y = (address >> 1) & 7;
	
	unsigned char x, bitIndex;
	for(x = 0; x < 8; x++) {
		bitIndex = 1 << (7 - x);
		
		//((unsigned char (*)[8][8])tiles)[tile][y][x] = ((vram[address] & bitIndex) ? 1 : 0) + ((vram[address + 1] & bitIndex) ? 2 : 0);
		tiles->data[tile][y][x] = ((vram[address] & bitIndex) ? 1 : 0) + ((vram[address + 1] & bitIndex) ? 2 : 0);
	}
	
	#ifdef DS
		dirtyTileset = 1;
	#endif
}
