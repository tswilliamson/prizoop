/*
===============================================================================

  Ptune2 is SH7305 CPG&BSC tuning utility for PRIZM fx-CG10/20  v1.10

 copyright(c)2014,2015,2016 by sentaro21
 e-mail sentaro21@pm.matrix.jp

===============================================================================
*/

#include "SH7305_CPG_BSC.h"
#include "Ptune2_direct.h"

//---------------------------------------------------------------------------------------------
#define FLF_default  900
#define FLFMAX      2048-1

#define PLL_64x 0b111111 // 
#define PLL_36x 0b100011 //
#define PLL_32x 0b011111 // 
#define PLL_26x 0b011001 // 
#define PLL_16x 0b001111 // default
#define PLLMAX  64-1

#define DIV_2 0b0000	// 1/2
#define DIV_4 0b0001	// 1/4
#define DIV_8 0b0010	// 1/8
#define DIV16 0b0011	// 1/16
#define DIV32 0b0100	// 1/32
#define DIV64 0b0101	// 1/64

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

#define FLLFRQ_default	0x00004384
#define FRQCRA_default	0x0F102203

#define CS0BCR_default	0x24920400
#define CS2BCR_default	0x24923400
#define CS3BCR_default  0x24924400
#define CS4BCR_default  0x36DB0400
#define CS5aBCR_default 0x15140400
#define CS5bBCR_default 0x15140400
#define CS6aBCR_default 0x34D30200
#define CS6bBCR_default 0x34D30200

#define CS0WCR_default	0x000001C0
#define CS2WCR_default	0x00000140
#define CS3WCR_default  0x000024D0
#define CS4WCR_default  0x00000540
#define CS5aWCR_default 0x00010240
#define CS5bWCR_default 0x00010240
#define CS6aWCR_default 0x000302C0
#define CS6bWCR_default 0x000302C0

//---------------------------------------------------------------------------------------------
unsigned int	SaveDataF0_FLLFRQ=FLLFRQ_default;		// FLL:900		default
unsigned int	SaveDataF0_FRQCR =FRQCRA_default;		// PLL:x16 IFC:1/4 SFC:1/8 BFC:1/8 PFC:1/16
unsigned int	SaveDataF0_CS0BCR=CS0BCR_default;		// IWW:2 IWRRS:2
unsigned int	SaveDataF0_CS2BCR=CS2BCR_default;		// IWW:2 IWRRS:2
unsigned int	SaveDataF0_CS0WCR=CS0WCR_default;		// wait:3
unsigned int	SaveDataF0_CS2WCR=CS2WCR_default;		// wati:2
unsigned int	SaveDataF0_CS5aBCR=CS5aBCR_default;		//
unsigned int	SaveDataF0_CS5aWCR=CS5aWCR_default;		//

unsigned int	SaveDataF1_FLLFRQ=FLLFRQ_default;		// FLL:900		default
unsigned int	SaveDataF1_FRQCR =FRQCRA_default;		// PLL:x16 IFC:1/4 SFC:1/8 BFC:1/8 PFC:1/16
unsigned int	SaveDataF1_CS0BCR=CS0BCR_default;		// IWW:2 IWRRS:2
unsigned int	SaveDataF1_CS2BCR=CS2BCR_default;		// IWW:2 IWRRS:2
unsigned int	SaveDataF1_CS0WCR=CS0WCR_default;		// wait:3
unsigned int	SaveDataF1_CS2WCR=CS2WCR_default;		// wati:2
unsigned int	SaveDataF1_CS5aBCR=CS5aBCR_default;		//
unsigned int	SaveDataF1_CS5aWCR=CS5aWCR_default;		//

unsigned int	SaveDataF2_FLLFRQ=0x00004000+900;		// FLL:900		 59MHz
unsigned int	SaveDataF2_FRQCR =(PLL_32x<<24)+(DIV_8<<20)+(DIV16<<12)+(DIV16<<8)+DIV32;
unsigned int	SaveDataF2_CS0BCR=0x04900400;			// IWW:0 IWRRS:0
unsigned int	SaveDataF2_CS2BCR=0x04903400;			// IWW:0 IWRRS:0
unsigned int	SaveDataF2_CS0WCR=0x00000140;			// wait:2
unsigned int	SaveDataF2_CS2WCR=0x000100C0;			// wait:1 WW:0
unsigned int	SaveDataF2_CS5aBCR=CS5aBCR_default;		//
unsigned int	SaveDataF2_CS5aWCR=CS5aWCR_default;		//

unsigned int	SaveDataF3_FLLFRQ=0x00004000+900;		// FLL:900		118MHz
unsigned int	SaveDataF3_FRQCR =(PLL_32x<<24)+(DIV_4<<20)+(DIV_8<<12)+(DIV_8<<8)+DIV32;
unsigned int	SaveDataF3_CS0BCR=0x14900400;			// IWW:1 IWRRS:0
unsigned int	SaveDataF3_CS2BCR=0x04903400;			// IWW:0 IWRRS:0
unsigned int	SaveDataF3_CS0WCR=0x000002C0;			// wait:5
unsigned int	SaveDataF3_CS2WCR=0x000201C0;			// wait:3 WW:1
unsigned int	SaveDataF3_CS5aBCR=CS5aBCR_default;		//
unsigned int	SaveDataF3_CS5aWCR=CS5aWCR_default;		//

unsigned int	SaveDataF4_FLLFRQ=0x00004000+900;		// FLL:900		118MHz
unsigned int	SaveDataF4_FRQCR =(PLL_32x<<24)+(DIV_4<<20)+(DIV_4<<12)+(DIV_4<<8)+DIV32;
unsigned int	SaveDataF4_CS0BCR=0x24900400;			// IWW:2 IWRRS:0
unsigned int	SaveDataF4_CS2BCR=0x04903400;			// IWW:0 IWRRS:0
unsigned int	SaveDataF4_CS0WCR=0x00000440;			// wait:10
unsigned int	SaveDataF4_CS2WCR=0x00040340;			// wait:6 WW:3
unsigned int	SaveDataF4_CS5aBCR=CS5aBCR_default;		//
unsigned int	SaveDataF4_CS5aWCR=CS5aWCR_default;		//

unsigned int	SaveDataF5_FLLFRQ=0x00004000+900;		// FLL:900		191MHz
unsigned int	SaveDataF5_FRQCR =(PLL_26x<<24)+(DIV_2<<20)+(DIV_4<<12)+(DIV_4<<8)+DIV16;
unsigned int	SaveDataF5_CS0BCR=0x24900400;			// IWW:2 IWRRS:0
unsigned int	SaveDataF5_CS2BCR=0x04903400;			// IWW:0 IWRRS:0
unsigned int	SaveDataF5_CS0WCR=0x000003C0;			// wait:8
unsigned int	SaveDataF5_CS2WCR=0x000402C0;			// wait:5 WW:3
unsigned int	SaveDataF5_CS5aBCR=CS5aBCR_default;		//
unsigned int	SaveDataF5_CS5aWCR=CS5aWCR_default;		//

//---------------------------------------------------------------------------------------------
void FRQCR_kick(void) { 
    CPG.FRQCRA.BIT.KICK = 1 ;
    while((CPG.LSTATS & 1)!=0);
}
void change_FRQCRs( unsigned int value) { 
    CPG.FRQCRA.LONG = value ;
    FRQCR_kick();
}
//---------------------------------------------------------------------------------------------

void LoadDataF0(){				// default 
	BSC.CS0WCR.BIT.WR = WAIT18 ;
	BSC.CS2WCR.BIT.WR = WAIT18 ;
	CPG.FLLFRQ.LONG   = FLLFRQ_default ;
	change_FRQCRs(      FRQCRA_default ) ;
	BSC.CS0BCR.LONG   = CS0BCR_default ;
	BSC.CS2BCR.LONG   = CS2BCR_default ;
	BSC.CS0WCR.LONG   = CS0WCR_default ;
	BSC.CS2WCR.LONG   = CS2WCR_default ;
	BSC.CS5ABCR.LONG  = CS5aBCR_default ;
	BSC.CS5AWCR.LONG  = CS5aWCR_default ;
}
void LoadDataF1(){
	BSC.CS0WCR.BIT.WR = WAIT18 ;
	BSC.CS2WCR.BIT.WR = WAIT18 ;
	CPG.FLLFRQ.LONG   = SaveDataF1_FLLFRQ ;
	change_FRQCRs(      SaveDataF1_FRQCR ) ;
	BSC.CS0BCR.LONG   = SaveDataF1_CS0BCR ;
	BSC.CS2BCR.LONG   = SaveDataF1_CS2BCR ;
	BSC.CS0WCR.LONG   = SaveDataF1_CS0WCR ;
	BSC.CS2WCR.LONG   = SaveDataF1_CS2WCR ;
	BSC.CS5ABCR.LONG  = SaveDataF1_CS5aBCR ;
	BSC.CS5AWCR.LONG  = SaveDataF1_CS5aWCR ;
}
void LoadDataF2(){
	BSC.CS0WCR.BIT.WR = WAIT18 ;
	BSC.CS2WCR.BIT.WR = WAIT18 ;
	CPG.FLLFRQ.LONG   = SaveDataF2_FLLFRQ ;
	change_FRQCRs(      SaveDataF2_FRQCR ) ;
	BSC.CS0BCR.LONG   = SaveDataF2_CS0BCR ;
	BSC.CS2BCR.LONG   = SaveDataF2_CS2BCR ;
	BSC.CS0WCR.LONG   = SaveDataF2_CS0WCR ;
	BSC.CS2WCR.LONG   = SaveDataF2_CS2WCR ;
	BSC.CS5ABCR.LONG  = SaveDataF2_CS5aBCR ;
	BSC.CS5AWCR.LONG  = SaveDataF2_CS5aWCR ;
}
void LoadDataF3(){
	BSC.CS0WCR.BIT.WR = WAIT18 ;
	BSC.CS2WCR.BIT.WR = WAIT18 ;
	CPG.FLLFRQ.LONG   = SaveDataF3_FLLFRQ ;
	change_FRQCRs(      SaveDataF3_FRQCR ) ;
	BSC.CS0BCR.LONG   = SaveDataF3_CS0BCR ;
	BSC.CS2BCR.LONG   = SaveDataF3_CS2BCR ;
	BSC.CS0WCR.LONG   = SaveDataF3_CS0WCR ;
	BSC.CS2WCR.LONG   = SaveDataF3_CS2WCR ;
	BSC.CS5ABCR.LONG  = SaveDataF3_CS5aBCR ;
	BSC.CS5AWCR.LONG  = SaveDataF3_CS5aWCR ;
}
void LoadDataF4(){
	BSC.CS0WCR.BIT.WR = WAIT18 ;
	BSC.CS2WCR.BIT.WR = WAIT18 ;
	CPG.FLLFRQ.LONG   = SaveDataF4_FLLFRQ ;
	change_FRQCRs(      SaveDataF4_FRQCR ) ;
	BSC.CS0BCR.LONG   = SaveDataF4_CS0BCR ;
	BSC.CS2BCR.LONG   = SaveDataF4_CS2BCR ;
	BSC.CS0WCR.LONG   = SaveDataF4_CS0WCR ;
	BSC.CS2WCR.LONG   = SaveDataF4_CS2WCR ;
	BSC.CS5ABCR.LONG  = SaveDataF4_CS5aBCR ;
	BSC.CS5AWCR.LONG  = SaveDataF4_CS5aWCR ;
}
void LoadDataF5(){
	BSC.CS0WCR.BIT.WR = WAIT18 ;
	BSC.CS2WCR.BIT.WR = WAIT18 ;
	CPG.FLLFRQ.LONG   = SaveDataF5_FLLFRQ ;
	change_FRQCRs(      SaveDataF5_FRQCR ) ;
	BSC.CS0BCR.LONG   = SaveDataF5_CS0BCR ;
	BSC.CS2BCR.LONG   = SaveDataF5_CS2BCR ;
	BSC.CS0WCR.LONG   = SaveDataF5_CS0WCR ;
	BSC.CS2WCR.LONG   = SaveDataF5_CS2WCR ;
	BSC.CS5ABCR.LONG  = SaveDataF5_CS5aBCR ;
	BSC.CS5AWCR.LONG  = SaveDataF5_CS5aWCR ;
}


void SaveDataF1(){
	SaveDataF1_FLLFRQ =CPG.FLLFRQ.LONG ;
	SaveDataF1_FRQCR  =CPG.FRQCRA.LONG ;
	SaveDataF1_CS0BCR =BSC.CS0BCR.LONG ;
	SaveDataF1_CS2BCR =BSC.CS2BCR.LONG ;
	SaveDataF1_CS0WCR =BSC.CS0WCR.LONG ;
	SaveDataF1_CS2WCR =BSC.CS2WCR.LONG ;
	SaveDataF1_CS5aBCR=BSC.CS5ABCR.LONG  ;
	SaveDataF1_CS5aWCR=BSC.CS5AWCR.LONG;
}
void SaveDataF2(){
	SaveDataF2_FLLFRQ =CPG.FLLFRQ.LONG ;
	SaveDataF2_FRQCR  =CPG.FRQCRA.LONG ;
	SaveDataF2_CS0BCR =BSC.CS0BCR.LONG ;
	SaveDataF2_CS2BCR =BSC.CS2BCR.LONG ;
	SaveDataF2_CS0WCR =BSC.CS0WCR.LONG ;
	SaveDataF2_CS2WCR =BSC.CS2WCR.LONG ;
	SaveDataF2_CS5aBCR=BSC.CS5ABCR.LONG  ;
	SaveDataF2_CS5aWCR=BSC.CS5AWCR.LONG;
}
void SaveDataF3(){
	SaveDataF3_FLLFRQ =CPG.FLLFRQ.LONG ;
	SaveDataF3_FRQCR  =CPG.FRQCRA.LONG ;
	SaveDataF3_CS0BCR =BSC.CS0BCR.LONG ;
	SaveDataF3_CS2BCR =BSC.CS2BCR.LONG ;
	SaveDataF3_CS0WCR =BSC.CS0WCR.LONG ;
	SaveDataF3_CS2WCR =BSC.CS2WCR.LONG ;
	SaveDataF3_CS5aBCR=BSC.CS5ABCR.LONG ;
	SaveDataF3_CS5aWCR=BSC.CS5AWCR.LONG ;
}
void SaveDataF4(){
	SaveDataF4_FLLFRQ =CPG.FLLFRQ.LONG ;
	SaveDataF4_FRQCR  =CPG.FRQCRA.LONG ;
	SaveDataF4_CS0BCR =BSC.CS0BCR.LONG ;
	SaveDataF4_CS2BCR =BSC.CS2BCR.LONG ;
	SaveDataF4_CS0WCR =BSC.CS0WCR.LONG ;
	SaveDataF4_CS2WCR =BSC.CS2WCR.LONG ;
	SaveDataF4_CS5aBCR=BSC.CS5ABCR.LONG ;
	SaveDataF4_CS5aWCR=BSC.CS5AWCR.LONG ;
}
void SaveDataF5(){
	SaveDataF5_FLLFRQ =CPG.FLLFRQ.LONG ;
	SaveDataF5_FRQCR  =CPG.FRQCRA.LONG ;
	SaveDataF5_CS0BCR =BSC.CS0BCR.LONG ;
	SaveDataF5_CS2BCR =BSC.CS2BCR.LONG ;
	SaveDataF5_CS0WCR =BSC.CS0WCR.LONG ;
	SaveDataF5_CS2WCR =BSC.CS2WCR.LONG ;
	SaveDataF5_CS5aBCR=BSC.CS5ABCR.LONG ;
	SaveDataF5_CS5aWCR=BSC.CS5AWCR.LONG ;
}

//---------------------------------------------------------------------------------------------

