
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

static int freqSpeed = 4;
// initializes the platform sound system, called when emulation begins. Returns false on error
bool sndInit() {
	unsigned char settings[5] = { 0,9,0,0,0 };//115200,1xstop bits
	if (Serial_Open(&settings[0]))
	{
		return false;
	}

	SCIF2.SCSMR.BIT.CKS = 1;	//1/4
	SCIF2.SCSMR.BIT.CA = 1;		//Synchronous mode is 16 times faster
	SCIF2.SCBRR = 4;			//!override speed to 1836000 bps (for 9860)

	bytesLeft = 0;

	freqSpeed = 8;
	switch (Ptune2_GetSetting()) {
		case PT2_DEFAULT:
		case PT2_NOMEMWAIT:
			freqSpeed = 4;
			break;
		case PT2_HALFINC:
			freqSpeed = 6;
			break;
		case PT2_DOUBLE:
			freqSpeed = 8;
			break;
	}

	return true;
}

static void feed() {
	TIME_SCOPE();

	unsigned char last = curSoundBuffer[BUFF_SIZE - 1];
	sndFrame(curSoundBuffer, BUFF_SIZE);

	// smooth result to avoid jerkies/clicks
	for (int i = BUFF_SIZE - 1; i > 0; i--) {
		curSoundBuffer[i] = (curSoundBuffer[i - 1] + curSoundBuffer[i]) / 2;
	}
	curSoundBuffer[0] = (curSoundBuffer[0] + last) / 2;

	bytesLeft = BUFF_SIZE;
}

// platform update from emulator for sound system, called 8 times per frame (should be enough!)
void sndUpdate() {
	TIME_SCOPE();

	{
		static int curVoltage = 0;
		static int curSample = 0;
		static int freqSample = 0;
		int timeConstantShift = 8;	// 1/128th bits per time constant for good volume
		int i = BUFF_SIZE - bytesLeft;
		int toAdd = Serial_PollTX();
		if (toAdd == 256) curVoltage = 0;	// reset voltage because we missed too many samples
		while (toAdd > 64)
		{
			unsigned char writeBuffer[64];
			for (int iter = 0; iter < 64; iter++) {
				if (freqSample % freqSpeed == 0) {
					if (!bytesLeft) {
						feed();
						i = 0;
					}
					curSample = ((int)curSoundBuffer[i]) * 16 + 2048;
					i++;
					bytesLeft--;
				}

				int byte = 0;
				for (int b = 0; b < 8; b++) {
					if (curVoltage < curSample) {
						curVoltage += (8192 >> timeConstantShift);
						byte |= (1 << b);
					}
					else {
						curVoltage -= (curVoltage >> timeConstantShift);
					}
				}

				writeBuffer[iter] = byte;

				freqSample++;
			}

			Serial_Write(writeBuffer, 64);
			toAdd -= 64;
		}
	}
}

// cleans up the platform sound system, called when emulation ends
void sndCleanup() {
	Serial_Close(1);
}

#endif