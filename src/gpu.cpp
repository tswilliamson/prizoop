#include "platform.h"
#include "debug.h"

#include "display.h"
#include "memory.h"
#include "cpu.h"
#include "interrupts.h"

#include "gpu.h"

struct gpu_type gpu;

tilestype* tiles = NULL;

unsigned char backgroundPalette[4];
unsigned char spritePalette[2][4];

void gpuStep(void) {
	
	static long lastTicks = 0;
	
	gpu.tick += cpu.clocks - lastTicks;
	lastTicks = cpu.clocks;

	// we can force a step to avoid just spinning wheels when halted:
	if (cpu.halted && cpu.IME && ((cpu.memory.IE_intenable & (INTERRUPTS_JOYPAD | INTERRUPTS_TIMER)) == 0)) {
		unsigned int needTicks = 0;
		switch (GET_LCDC_MODE()) {
			case GPU_MODE_HBLANK:
				needTicks = 204;
				break;
			case GPU_MODE_VBLANK:
				needTicks = 456;
				break;
			case GPU_MODE_OAM:
				needTicks = 80;
				break;
			case GPU_MODE_VRAM:
				needTicks = 172;
				break;
		}
		if (gpu.tick < needTicks) {
			cpu.clocks += needTicks - gpu.tick;
			gpu.tick = needTicks;
		}
	}
	
	switch(GET_LCDC_MODE()) {
		case GPU_MODE_HBLANK:
			if(gpu.tick >= 204) {
				hblank();
				
				if(cpu.memory.LY_lcdline == 144) {
					if (drawFramebuffer) {
						drawFramebuffer();
					}
					cpu.memory.IF_intflag |= INTERRUPTS_VBLANK;
					
					SET_LCDC_MODE(GPU_MODE_VBLANK);

					if (cpu.memory.STAT_lcdstatus & STAT_VBLANKCHECK) {
						cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
					}
				}
				
				else {
					SET_LCDC_MODE(GPU_MODE_OAM);

					if (cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) {
						cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
					}
				}
				
				gpu.tick -= 204;
			}
			
			break;
		
		case GPU_MODE_VBLANK:
			if(gpu.tick >= 456) {
				hblank();
				
				if(cpu.memory.LY_lcdline > 153) {
					cpu.memory.LY_lcdline = 0;
					SET_LCDC_MODE(GPU_MODE_OAM);

					if (cpu.memory.STAT_lcdstatus & STAT_OAMCHECK) {
						cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
					}
				}
				
				gpu.tick -= 456;
			}
			
			break;
		
		case GPU_MODE_OAM:
			if(gpu.tick >= 80) {
				SET_LCDC_MODE(GPU_MODE_VRAM);
				
				gpu.tick -= 80;
			}
			
			break;
		
		case GPU_MODE_VRAM:
			if(gpu.tick >= 172) {
				SET_LCDC_MODE(GPU_MODE_HBLANK);

				if (cpu.memory.STAT_lcdstatus & STAT_HBLANKCHECK) {
					cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
				}
				
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

	if (cpu.memory.LY_lcdline == cpu.memory.LYC_lcdcompare) {
		cpu.memory.STAT_lcdstatus |= STAT_LYCSIGNAL;
		if (cpu.memory.STAT_lcdstatus & STAT_LYCCHECK) {
			cpu.memory.IF_intflag |= INTERRUPTS_LCDSTAT;
		}
	}
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
}
