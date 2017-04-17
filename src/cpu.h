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
						unsigned char NR10_snd1sweep;		// sound channel 1 sweep, bits 6-4: sweep time, bit 3: pos/neg, bit 2-0: sweep amt
						unsigned char NR11_snd1len;			// sound channel 1 length, bit 7-6: wave duty, bit 5-0 : length
						unsigned char NR12_snd1env;			// sound channel 1 enveloper, bits 7-4: vol, bit 3: pos/neg, bit 2-0: delta amt
						unsigned char NR13_snd1frqlo;		// sound channel 1 frequency (low 8 bits)
						unsigned char NR14_snd1ctl;			// sound channel 1 ctrl, bits 7: restart, bit 6: use len, bits 2-0: high 3 freq bits
						unsigned char _unused15;
						unsigned char NR21_snd2len;			// sound channel 2 length (same bits as 1)
						unsigned char NR22_snd2env;			// sound channel 2 enveloper (same bits as 1)
						unsigned char NR23_snd2frqlo;		// sound channel 2 freq (lower 8 bits)
						unsigned char NR24_snd2ctl;			// sound channel 2 ctrl (same bits as 1)
						unsigned char NR30_snd3enable;		// sound channel 3 enable (bit 7)
						unsigned char NR31_snd3len;			// sound channel 3 length (all bits)
						unsigned char NR32_snd3vol;			// sound channel 3 volume (bits 6-5)
						unsigned char NR33_snd3frqlo;		// sound channel 3 freq (lower 8 bits)
						unsigned char NR34_snd3ctl;			// sound channel 3 ctrl, bit 7: restart, bit 6: use len, bits 2-0 : high 3 freq bits
						unsigned char _unused1F;
						unsigned char NR41_snd4len;			// sound channel 4 (noise) length
						unsigned char NR42_snd4env;			// sound channel 4 enveloper (same bits as 1)
						unsigned char NR43_snd4cnt;			// sound channel 4 polynomial noise thingy (bits 7-4: bitshift, bit3: 16/8 counter, bit2-0: div ratio
						unsigned char NR44_snd4ctl;			// sound channel 4 ctrl, bit 7: restart, bit 6: use len
						unsigned char NR50_spkvol;			// speaker volume, bit 6-4 : left vol, bit 2-0: right vol
						unsigned char NR51_chselect;		// sound to speaker channel select (bit = 4*speaker+channel active)
						unsigned char NR52_soundmast;		// sound master enable (bit 7) and sound on flag (bits 3-0)
						unsigned char _unused272F[9];	
						unsigned char WAVE_ptr[16];			// sound channel 3's wave pattern RAM
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
						unsigned char _unused4C;
						unsigned char KEY1_cgbspeed;		// speed switch for GBC
						unsigned char _unused4E;
						unsigned char VBK_cgbvram;			// vram select for GBC
						unsigned char _unused50;
						unsigned char HDMA1_cgbsrchigh;		// Source hugh byte for CGB DMA, write only
						unsigned char HDMA2_cgbsrclow;		// Source low byte for CGB DMA, write only
						unsigned char HDMA3_cgbsrchigh;		// Dest hugh byte for CGB DMA, write only
						unsigned char HDMA4_cgbsrclow;		// Dest low byte for CGB DMA, write only
						unsigned char HDMA5_cgbstat;		// Status/operation byte for CGB DMA, R/W, bit 7 = mode, bit 0-6 = length / remaining
						unsigned char _unused5667[0x12];
						unsigned char BGPI_bgpalindex;		// BG palette index / increment for CGB, bits 0-5 : palette address, bit 7 : auto increment
						unsigned char BGPD_bgpaldata;		// data byte to bg palette index, writes may increment index based on auto increment
						unsigned char OBPI_objpalindex;		// OBJ palette index / increment for CGB, bits 0-5 : palette address, bit 7: auto increment
						unsigned char OBPD_objpaldata;		// data byte to obj palette index, writes may increment index based on auto increment
						unsigned char _unused6C6F[0x04];
						unsigned char SVBK_cgbram;			// ram select for GBC
						unsigned char _unused717F[0x0F];
					};
				};
				unsigned char hram[0x7F];
				unsigned char IE_intenable;					// interrupt enable bits 4-0
			};
		};
	} memory;

	// main cpu cpu.registers
	registers_type registers;
	unsigned int IME;							// master interrupt enable (di, ei)

	// misc cpu info
	unsigned int clocks;						// according to clock frequency (4 MHz)
	unsigned int halted;
	unsigned int stopped;

	// cpu div
	unsigned int div;
	unsigned int divBase;

	// cpu timer
	unsigned int timer;
	unsigned int timerBase;
	unsigned int timerInterrupt;

	// gpu
	unsigned int gpuTick;		// next relevant tick for gpu
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

// resets CPU to default GB settings
void cpuReset(void);

void cpuStep(void);
void updateDiv();

void updateTimer();

// special timer based write functionality based on timer state
void writeTIMA(unsigned char value);
void writeTAC(unsigned char value);