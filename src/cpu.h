#pragma once

#include "platform.h"
#include "registers.h"

// Representation of the CPU and on chip memory
#pragma pack(push,1)
struct cpu_type {
	// first the cpu on-chip memory (and IO regs) for alignment purposes
	struct {
		union {
			unsigned char all[0x100];
			struct {
				union {
					unsigned char io[0x80];
					struct {
						unsigned char P1_joypad;
						unsigned char SB_serial_data;		// serial data byte
						unsigned char SC_serial_ctl;		// serial control. bit0=CLK, bit7=TRX enable
						unsigned char _unused03;
						unsigned char DIV_clk;				// runs at 16 KHz, any write sets to 0
						unsigned char TIMA_timerctr;		// timer counter value
						unsigned char TMA_timermodulo;		// amt loaded to counter on overflow
						unsigned char TAC_timerctl;			// timer control: bit2=enable, bit1-0=freq div
						unsigned char _unused080E[7];
						unsigned char IF_intflag;			// flag bit 4-0 set when interrupt triggered
						unsigned char _sound103F[48];		// sound registers (Prizoop doesn't currently support sound)
						unsigned char LCDC_ctl;				// lcd control, bits: 7=enable,6=wdwmap,5=wdwon,4=bgtiledata,3=bgmap,2=sprsize,1=spron,0=spr/bgon
						unsigned char STAT_lcdstatus;		// lcd status bits 6-3:interrupt select, bit2=lyc flag, bit1-0=lcd state (hblank,vblank,oam,trx)
						unsigned char SCY_bgscrolly;		// y value scroll for background
						unsigned char SCX_bgscrollx;		// x value scroll for background
						unsigned char LY_lcdline;			// effective scanline, extends past 0x90 in vblank
						unsigned char LYC_lcdcompare;		// scanline compare (usually for stat interrupt)
						unsigned char DMA_dmawrite;			// DMA to the OAM with the written high byte
						unsigned char BGP_bgpalette;		// "color" palette used for bg and window tiles bit7-6=11,bit5-4=10,3-2=01,1-0=00
						unsigned char OBP0_spritepal0;		// sprite palette 0, same as above except bit 0 is always transparent
						unsigned char OBP1_spritepal1;		// sprite palette 1, same as above except bit 0 is always transparent
						unsigned char WY_windowy;			// window y position, visibility requires 0 <= WY <= 143
						unsigned char WX_windowx;			// window x position, offset by 7, visibility req 0 <= WX <= 166
						unsigned char _unused4C7F[0x34];
					};
				};
				unsigned char hram[0x7F];
				unsigned char IE_intenable;					// interrupt enable bits 4-0
			};
		};
	} memory;

	// main cpu cpu.registers
	registers_type registers;
	unsigned char IME;							// master interrupt enable (di, ei)

	// misc cpu info
	unsigned long clocks;		// according to clock frequency (4 MHz)
	unsigned char halted;
	unsigned char stopped;

	// cpu div
	unsigned long div;
	unsigned long divBase;

	// cpu timer
	unsigned long timer;
	unsigned long timerBase;
	unsigned long timerInterrupt;

	// gpu
	unsigned long gpuTick;		// next relevant tick for gpu
};
#pragma pack(pop)

extern cpu_type cpu ALIGN(256);

#define FLAGS_Z (1 << 7)
#define FLAGS_Z_BIT 7
#define FLAGS_N (1 << 6)
#define FLAGS_N_BIT 6
#define FLAGS_HC (1 << 5)
#define FLAGS_HC_BIT 5
#define FLAGS_C (1 << 4)
#define FLAGS_C_BIT 4

#define FLAGS_ISZERO (cpu.registers.f & FLAGS_Z)
#define FLAGS_ISNEGATIVE (cpu.registers.f & FLAGS_N)
#define FLAGS_ISCARRY (cpu.registers.f & FLAGS_C)
#define FLAGS_ISHALFCARRY (cpu.registers.f & FLAGS_HC)

#define FLAGS_ISSET(x) (cpu.registers.f & (x))
#define FLAGS_SET(x) (cpu.registers.f |= (x))
#define FLAGS_CLEAR(x) (cpu.registers.f &= ~(x))

void reset(void);
void cpuStep(void);
void updateDiv();

void updateTimer();

// special timer based write functionality based on timer state
void writeTIMA(unsigned char value);
void writeTAC(unsigned char value);