
#if !TARGET_WINSIM

#include "platform.h"
#include "snd.h"
#include "ptune2_simple\Ptune2_direct.h"

#define BUFF_SIZE (SOUND_RATE / 256)
static unsigned char curSoundBuffer[BUFF_SIZE];
static int bytesLeft = 0;

struct st_scif0 {                                      /* struct SCIF0 */
       union {                                         /* SCSMR        */
             unsigned short WORD;                      /*  Word Access */
             struct {                                  /*  Bit Access  */
                    unsigned short :8;                 /*              */
                    unsigned short CA:1;               /*   CA         */
                    unsigned short CHR:1;              /*   CHR        */
                    unsigned short PE:1;               /*   PE         */
                    unsigned short OE:1;               /*   OE         */
                    unsigned short STOP:1;             /*   STOP       */
                    unsigned short :1;                 /*              */
                    unsigned short CKS:2;              /*   CKS        */
                    } BIT;                             /*              */
             } SCSMR;                                  /*              */
       unsigned char wk0[2];                           /*              */
       unsigned char SCBRR;                            /* SCBRR        */
       unsigned char wk1[3];                           /*              */
       union {                                         /* SCSCR        */
             unsigned short WORD;                      /*  Word Access */
             struct {                                  /*  Bit Access  */
                    unsigned short :8;                 /*              */
                    unsigned short TIE:1;              /*   TIE        */
                    unsigned short RIE:1;              /*   RIE        */
                    unsigned short TE:1;               /*   TE         */
                    unsigned short RE:1;               /*   RE         */
                    unsigned short REIE:1;             /*   REIE       */
                    unsigned short :1;                 /*              */
                    unsigned short CKE:2;              /*   CKE        */
                    } BIT;                             /*              */
             } SCSCR;                                  /*              */
       unsigned char wk2[2];                           /*              */
       unsigned char SCFTDR;                           /* SCFTDR       */
       unsigned char wk3[3];                           /*              */
       union {                                         /* SCFSR        */
             unsigned short WORD;                      /*  Word Access */
             struct {                                  /*  Bit Access  */
                    unsigned short PERC:4;             /*   PERC       */
                    unsigned short FERC:4;             /*   FERC       */
                    unsigned short ER:1;               /*   ER         */
                    unsigned short TEND:1;             /*   TEND       */
                    unsigned short TDFE:1;             /*   TDFE       */
                    unsigned short BRK:1;              /*   BRK        */
                    unsigned short FER:1;              /*   FER        */
                    unsigned short PER:1;              /*   PER        */
                    unsigned short RDF:1;              /*   RDF        */
                    unsigned short DR:1;               /*   DR         */
                    } BIT;                             /*              */
             } SCFSR;                                  /*              */
       unsigned char wk4[2];                           /*              */
       unsigned char SCFRDR;                           /* SCFRDR       */
       unsigned char wk5[3];                           /*              */
       union {                                         /* SCFCR        */
             unsigned short WORD;                      /*  Word Access */
             struct {                                  /*  Bit Access  */
                    unsigned short :5;                 /*              */
                    unsigned short RSTRG:3;            /*   RSTRG      */
                    unsigned short RTRG:2;             /*   RTRG       */
                    unsigned short TTRG:2;             /*   TTRG       */
                    unsigned short MCE:1;              /*   MCE        */
                    unsigned short TFRST:1;            /*   TFRST      */
                    unsigned short RFRST:1;            /*   RFRST      */
                    unsigned short LOOP:1;             /*   LOOP       */
                    } BIT;                             /*              */
             } SCFCR;                                  /*              */
       unsigned char wk6[2];                           /*              */
       union {                                         /* SCFDR        */
             unsigned short WORD;                      /*  Word Access */
             struct {                                  /*  Bit Access  */
                    unsigned short :3;                 /*              */
                    unsigned short TFDC:5;             /*   TFDC       */
                    unsigned short :3;                 /*              */
                    unsigned short RFDC:5;             /*   RFDC       */
                    } BIT;                             /*              */
             } SCFDR;                                  /*              */
       unsigned char wk7[6];                           /*              */
       union {                                         /* SCLSR        */
             unsigned short WORD;                      /*  Word Access */
             struct {                                  /*  Bit Access  */
                    unsigned short :15;                /*              */
                    unsigned short ORER:1;             /*   ORER       */
                    } BIT;                             /*              */
             } SCLSR;                                  /*              */
};           

#define SCIF2 (*(volatile struct st_scif0 *)0xA4410000)/* SCIF0 Address */

#define OLD_WAY 0

static int freqSpeed = 2;
// initializes the platform sound system, called when emulation begins. Returns false on error
bool sndInit() {
	unsigned char settings[5] = { 0,9,0,0,0 };//115200,1xstop bits
	if (Serial_Open(&settings[0]))
	{
		return false;
	}

	SCIF2.SCSMR.BIT.CKS = 2;	//1/16
	SCIF2.SCSMR.BIT.CA = 1;		//Synchronous mode is 16 times faster
	SCIF2.SCBRR = 0;			//!override speed to 1836000 bps (for 9860)

	bytesLeft = 0;

	switch (Ptune2_GetSetting()) {
		case PT2_DEFAULT:
		case PT2_NOMEMWAIT:
			freqSpeed = 2;
			break;
		case PT2_HALFINC:
			freqSpeed = 3;
			break;
		case PT2_DOUBLE:
			freqSpeed = 4;
			break;
	}

	return true;
}

//Conversion tables
#define X 0x10
#define _ 0x00

static const unsigned char LowTable[4 * 4] =
{
	_,_,_,_,//0
	X,_,_,_,//1
	X,_,X,_,//2
	_,X,X,X,//3
};

										   //0    1    2    3    4    5    6    7
static const unsigned char HighTable[8] = { 0x00,0x01,0x44,0x45,0x65,0x67,0xE7,0xEF };

static int giTableIndex = 0;
static int giSubSample = 0;
static int giFromHighTab = 0;

static const int ditherTable[8] = {
	0,
	-4,
	3,
	-1,
	2,
	-3,
	1,
	2,
};

// platform update from emulator for sound system, called 8 times per frame (should be enough!)
void sndUpdate() {
	TIME_SCOPE();

	if (!bytesLeft) {
		unsigned char last = curSoundBuffer[BUFF_SIZE - 1];
		sndFrame(curSoundBuffer, BUFF_SIZE);

		// smooth result to avoid jerkies/clicks
		for (int i = BUFF_SIZE-1; i > 0; i--) {
			curSoundBuffer[i] = (curSoundBuffer[i - 1] + curSoundBuffer[i]) / 2;
		}
		curSoundBuffer[0] = (curSoundBuffer[0] + last) / 2;

		bytesLeft = BUFF_SIZE;
	}

#if OLD_WAY
	// MPoupe with added dither (to reduce quantization noise) and middle range usage (more predictable voltage curve)
	{
		int i = BUFF_SIZE - bytesLeft;
		int toAdd = 16 - (SCIF2.SCFDR.WORD >> 8);
		while (toAdd && bytesLeft)
		{
			int iTmp;
			if (giTableIndex >= freqSpeed)
			{
				// audio dither
				giSubSample = (((int) curSoundBuffer[i] + curSoundBuffer[i] * ditherTable[i%8] / 64) + 128) >> 4;

				i++;
				bytesLeft--;
				giFromHighTab = HighTable[giSubSample >> 2];
				giSubSample = (giSubSample & 3) << 2;
				giTableIndex = 0;
			}

			
			iTmp = LowTable[giSubSample | giTableIndex] | giFromHighTab;
			SCIF2.SCFTDR = (unsigned char)iTmp;

			giTableIndex++;
			toAdd--;
		};
	}
#else
	{
		static int curVoltage = 0;
		static int curSample = 0;
		static int freqSample = 0;
		const int timeConstantShift = 7;	// 1/128th bits per time constant for good volume
		int i = BUFF_SIZE - bytesLeft;
		int toAdd = 16 - (SCIF2.SCFDR.WORD >> 8);
		if (toAdd == 16) curVoltage = 0;	// reset voltage because we missed too many samples
		while (toAdd && bytesLeft)
		{
			if (freqSample % freqSpeed == 0) {
				curSample = ((int)curSoundBuffer[i]) + 128;
				i++;
				bytesLeft--;
			}

			int byte = 0;
			int ditheredSample = curSample + ditherTable[freqSample % 8] * 16;
			for (int b = 0; b < 8; b++) {
				if (curVoltage < curSample) {
					curVoltage += (512 >> timeConstantShift);
					byte |= (1 << b);
				} else {
					curVoltage -= (curVoltage >> timeConstantShift);
				}
			}

			SCIF2.SCFTDR = (unsigned char)byte;

			toAdd--;
			freqSample++;
		}
	}
#endif
}

// cleans up the platform sound system, called when emulation ends
void sndCleanup() {
	Serial_Close(1);
}

#endif