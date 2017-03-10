
/*
	Prizm SafeOverClock

	Please see detailed notes in the header!
 */

#include "Prizm_SafeOverClock.h"
#include "SH7305_CPG_BSC.h"

// don't ever speed optimize this file!
#pragma GCC optimize ("Os")


///////////////////////////////////////////////////////////////////////////////////////////////////
// Defines and data

// FRQCR_STC values for each enum :
const int FRQCR_STC_vals[4] = {
	0x07,			// half
	0x0F,			// normal
	0x17,			// fast 
	0x1E,			// double 
};

#define WAIT_0 0b0000
#define WAIT_1 0b0001
#define WAIT_2 0b0010	// cs2wcr default 
#define WAIT_3 0b0011	// cs0wcr default 
#define WAIT_4 0b0100
#define WAIT_5 0b0101
#define WAIT_6 0b0110
#define WAIT_8 0b0111
#define WAIT10 0b1000
#define WAIT12 0b1001
#define WAIT14 0b1010
#define WAIT18 0b1011
#define WAIT24 0b1100

#define WAITW0 0b0001
#define WAITW1 0b0010
#define WAITW2 0b0011
#define WAITW3 0b0100
#define WAITW4 0b0101
#define WAITW5 0b0110
#define WAITW6 0b0111

//	precision 10KHz   ex. 14.7456MHz => 1474
//	flash max speed				-margin		//		11x+16			spa0%	spa0%+USB
#define	MaxROMwait_0 	 1600-200		//spanision 100ns			1648	1700
#define	MaxROMwait_1 	 2700-300		//					2756	2747	2835
#define	MaxROMwait_2 	 3800-200		//			*IWW0	3854	3818	3941
#define	MaxROMwait_3 	 4900-250  		//	dafault	4891	4944	4911	5067
#define	MaxROMwait_4 	 6000-300		//					6049	5998	6194
#define	MaxROMwait_5 	 7100-350		//					7141	7100	7333
#define	MaxROMwait_6 	 8200-400		//			*IWW1	8227	8185	8464
#define	MaxROMwait_8 	10300-500		//						   10349   10696
#define	MaxROMwait10 	12500-600		//						   12547   12951
#define	MaxROMwait12 	14600-700		//			*IWW2		   14733   15210
#define	MaxROMwait14 	16600-800		//
#define	MaxROMwait18 	20000-900		//

//	SRAM max speed				-margin				19x+19			spa0%	spa0%USB	mx0%	mx0%USB
#define	MaxRAMwait_0 	 1900-400		// 80ns?					1912	1970		2004	2043
#define	MaxRAMwait_1 	 3800-800		//							3802	3916		4005	4082
#define	MaxRAMwait_2 	 5700-500		//		default				5703	5855		6011	6112
#define	MaxRAMwait_3 	 7600-600		//							7594	7813		7999	8150
#define	MaxRAMwait_4 	 9500-700		//							9488	9780	   10013   10200
#define	MaxRAMwait_5 	11400-800		//						   11407   11712	   12024   12238
#define	MaxRAMwait_6 	13300-900		//						   13278   13677	   14000   14263
#define	MaxRAMwait_8 	17100-2100		//						   17098   17644	   17997   18308
#define	MaxRAMwait10 	18100-2100		//					

#define	MaxRAMwaitW_0 	 3500-500		//							3497				3549
#define	MaxRAMwaitW_1 	 7000-1000		//							6974				7114
#define	MaxRAMwaitW_2 	10500-1500		//						   10467			   10673
#define	MaxRAMwaitW_3 	14000-2000		//						   13964			   14291
#define	MaxRAMwaitW_4 	17500-2500		//						   17491			   17936
#define	MaxRAMwaitW_5 	18000-2500		//						   18251			   20745
#define	MaxRAMwaitW_6 	18500-2500		//						   18289			   20902

#define	Max_PLLdef	80000
#define	Max_IFCdef	27500
#define	Max_SFCdef	18000
#define	Max_BFCdef	13400
#define	Max_PFCdef	 2400

#define DIV_2 0b0000	// 1/2
#define DIV_4 0b0001	// 1/4
#define DIV_8 0b0010	// 1/8
#define DIV16 0b0011	// 1/16
#define DIV32 0b0100	// 1/32
#define DIV64 0b0101	// 1/64

int MaxFreq_RAMwaitW[] = { -1,MaxRAMwaitW_0,MaxRAMwaitW_1,MaxRAMwaitW_2,MaxRAMwaitW_3,MaxRAMwaitW_4,
	MaxRAMwaitW_5,MaxRAMwaitW_6 };

int MaxFreq_ROMwait[] = { MaxROMwait_0,MaxROMwait_1,MaxROMwait_2,MaxROMwait_3,MaxROMwait_4,
	MaxROMwait_5,MaxROMwait_6,MaxROMwait_8,MaxROMwait10,MaxROMwait12,
	MaxROMwait14,MaxROMwait18 };

int MaxFreq_RAMwait[] = { MaxRAMwait_0,MaxRAMwait_1,MaxRAMwait_2,MaxRAMwait_3,MaxRAMwait_4,
	MaxRAMwait_5,MaxRAMwait_6,MaxRAMwait_8,MaxRAMwait10 };

///////////////////////////////////////////////////////////////////////////////////////////////////
// System clock

void FRQCR_kick(void) {
	CPG.FRQCRA.BIT.KICK = 1;
	while ((CPG.LSTATS & 1) != 0);
}

void change_STC(int stc) {
	CPG.FRQCRA.BIT.STC = stc;
	FRQCR_kick();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// BUS Wait times

int freq_sub(int flf, int stc, int fc) {
	int freq_base = flf * 32768 * 10 / 1000 / 2; 	// default 900*32.768KHz / 2 = 14.7456MHz
	int	freq_div = 2;
	if (fc<0) freq_div = 1;
	while (fc>0) { freq_div *= 2; fc -= 1; }
	return	(((freq_base*(stc + 1) / freq_div) + 50) / 100);		// precision 10KHz   ex. 14.7456MHz => 1475
}

int CheckRAMwaitW(int freq) {						 // RAM write
	int i;
	for (i = WAITW6; i >= WAITW0; i--) {
		if (freq >= MaxFreq_RAMwaitW[i]) break;
	}
	return (i + 1);
}

int CheckROMwait(int freq) {						 // ROM
	int i;
	MaxFreq_ROMwait[WAIT14] = (MaxFreq_ROMwait[WAIT12] * 2 - MaxFreq_ROMwait[WAIT10]) * 99 / 100;
	MaxFreq_ROMwait[WAIT18] = (MaxFreq_ROMwait[WAIT14] * 2 - MaxFreq_ROMwait[WAIT10]) * 95 / 100;
	for (i = WAIT18; i >= WAIT_0; i--) {
		if (freq >= MaxFreq_ROMwait[i]) break;
	}
	return (i + 1);
}

int CheckRAMwait(int freq) {						 // RAM
	int i;
	MaxFreq_RAMwait[WAIT10] = (MaxFreq_RAMwait[WAIT_8] * 2 - MaxFreq_RAMwait[WAIT_6]) * 995 / 1000;
	for (i = WAIT10; i >= WAIT_0; i--) {
		if (freq >= MaxFreq_RAMwait[i]) break;
	}
	return (i + 1);
}

void SetROMwait(int freq, int CS0WCR_WR) {
	int	WR;
	int	IWW0;
	IWW0 = BSC.CS0BCR.BIT.IWW;
	if ((freq >= MaxFreq_ROMwait[WAIT_2]) && (IWW0 == 0))	BSC.CS0BCR.BIT.IWW = (1);
	if ((freq >= MaxFreq_ROMwait[WAIT_6]) && (IWW0 == 1))	BSC.CS0BCR.BIT.IWW = (2);
	if ((freq >= MaxFreq_ROMwait[WAIT12]) && (IWW0 == 2))	BSC.CS0BCR.BIT.IWW = (3);

	WR = CheckROMwait(freq);
	if (WR > CS0WCR_WR) BSC.CS0WCR.BIT.WR = (WR);	//	Wait +
}

void SetRAMwait(int freq, int CS2WCR_WR) {
	int	WR;

	WR = CheckRAMwait(freq);
	if (WR > CS2WCR_WR) BSC.CS2WCR.BIT.WR = (WR);	//	wait +

	BSC.CS2WCR.BIT.WW = CheckRAMwaitW(freq);
}

void SetWaitm(int FLF, int FRQCR_STC, int FRQCR_BFC, int CS0WCR_WR, int CS2WCR_WR) {
	int freq;
	int	WR;
	int	IWW0;

	freq = freq_sub(FLF + 1, FRQCR_STC, FRQCR_BFC);	// Bus freq  (FLF+1)

	BSC.CS2WCR.BIT.WW = (CheckRAMwaitW(freq));		// WW wait auto decrement
	WR = CheckROMwait(freq);
	if (WR < CS0WCR_WR) BSC.CS0WCR.BIT.WR = (WR);	// ROM wait decrement
	WR = CheckRAMwait(freq);
	if (WR < CS2WCR_WR) BSC.CS2WCR.BIT.WR = (WR);	// RAM wait decrement

	IWW0 = BSC.CS0BCR.BIT.IWW;
	if ((freq <  MaxFreq_ROMwait[WAIT_2]) && (IWW0 > 0))	BSC.CS0BCR.BIT.IWW = 0;
	if ((freq <  MaxFreq_ROMwait[WAIT_6]) && (IWW0 > 1))	BSC.CS0BCR.BIT.IWW = 1;
	if ((freq <  MaxFreq_ROMwait[WAIT12]) && (IWW0 > 2))	BSC.CS0BCR.BIT.IWW = 2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Peripheral clock

int CheckFC(int FLF, int FRQCR_STC, int FC, int maxfreq) {
	int freq;
	freq = freq_sub(FLF, FRQCR_STC, FC);	// freq
	if (freq >= maxfreq) return 0;
	return freq;
}

void change_PFC(int pfc) {
	CPG.FRQCRA.BIT.PFC = pfc;
	FRQCR_kick();
}

void PFC_auto_up(int FLF, int FRQCR_STC, int FRQCR_BFC, int FRQCR_PFC) {
	if ((CheckFC(FLF, FRQCR_STC, FRQCR_PFC - 1, Max_PFCdef)) == 0) return;	// PFC/2  freq
	if (FRQCR_PFC == FRQCR_BFC) return;
	change_PFC(FRQCR_PFC - 1);
}

void PFC_auto_down(int FLF, int FRQCR_STC, int FRQCR_PFC) {
	if (CheckFC(FLF, FRQCR_STC, FRQCR_PFC, Max_PFCdef) != 0) return;			// PFC freq
	if (FRQCR_PFC >= DIV64) return;
	change_PFC(FRQCR_PFC + 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Library implementation using the above :-)

void SetSafeClockSpeed(SafeClockSpeed withSpeed) {
	int FLF, freq, FRQCR_STC, FRQCR_BFC, FRQCR_PFC, FRQCR_IFC, FRQCR_SFC, CS0WCR_WR, CS2WCR_WR, targetSTC;

	FLF = CPG.FLLFRQ.BIT.FLF;
	FRQCR_STC = CPG.FRQCRA.BIT.STC;
	FRQCR_BFC = CPG.FRQCRA.BIT.BFC;
	FRQCR_PFC = CPG.FRQCRA.BIT.PFC;
	FRQCR_IFC = CPG.FRQCRA.BIT.IFC;
	FRQCR_SFC = CPG.FRQCRA.BIT.SFC;
	CS0WCR_WR = BSC.CS0WCR.BIT.WR;
	CS2WCR_WR = BSC.CS2WCR.BIT.WR;

	targetSTC = FRQCR_STC_vals[withSpeed];

	if (targetSTC <= FRQCR_STC) {
		change_STC(targetSTC);
		SetWaitm(FLF, targetSTC, FRQCR_BFC, CS0WCR_WR, CS2WCR_WR);
		PFC_auto_up(FLF, targetSTC, FRQCR_BFC, FRQCR_PFC);
	} else {
		if (targetSTC - 1 < 1 + FRQCR_BFC) { change_STC(targetSTC); return; }
		if (CheckFC(FLF, targetSTC, -1, Max_PLLdef) == 0) return;					// PLL freq
		if (CheckFC(FLF, targetSTC, FRQCR_IFC, Max_IFCdef) == 0) return;			// IFC freq
		if (CheckFC(FLF, targetSTC, FRQCR_SFC, Max_SFCdef) == 0) return;			// SFC freq
		freq = CheckFC(FLF, targetSTC, FRQCR_BFC, Max_BFCdef);						// BFC freq
		if (freq == 0) return;
		SetROMwait(freq, CS0WCR_WR);
		SetRAMwait(freq, CS2WCR_WR);
		PFC_auto_down(FLF, targetSTC, FRQCR_PFC);
		change_STC(targetSTC);
	}
}
