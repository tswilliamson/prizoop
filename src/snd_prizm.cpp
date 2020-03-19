
#if !TARGET_WINSIM

#include "platform.h"
#include "snd.h"
#include "ptune2_simple\Ptune2_direct.h"
#include "keys.h"

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

#define BUFF_SIZE (SOUND_RATE / 64)

static int curSoundBuffer[BUFF_SIZE];
static int sampleNum = 0;

static int sndSettings[5] = {
	3,			// volDivisor (2-5)
	4,			// sampleScale (out of 16)
	14,			// latency (out of 16)
	4,			// voltageDrop (out of 64)
	2,			// blendMode (out of 4)
};

int minVoltage = 0;
int maxVoltage = 0;

const int numSoundSettings = 5;

unsigned int lastSoundCounter;

struct BTCEntry {
	unsigned char bits;
	short voltageOffset;
};

static BTCEntry* btcTable = NULL;

inline const int ApplyLatency(const int val) {
	return val * sndSettings[2] / 16;
}

inline const int ApplyDrop(const int val) {
	return val * sndSettings[3] / 64;
}

void computeBTCTable() {
	const int volDivisor = 1 << sndSettings[0];
	const int baseRise = 6400;
	const int baseDrop = ApplyDrop(baseRise);
	minVoltage = min(baseDrop * 8 / volDivisor, 4096);
	maxVoltage = max(16384 - baseRise * 8 / volDivisor, 12288);

	for (int targetVoltage = 8; targetVoltage < 32768 + 8; targetVoltage += 16) {
		for (int rollingBitState = 0; rollingBitState < 2; rollingBitState++) {
			unsigned bitState = rollingBitState;
			int voltage = 16384;
			unsigned word = 0;

			for (int b = 0; b < 8; b++) {
				int bit1Voltage, bit0Voltage;
				switch (bitState) {
					case 0:
						bit1Voltage = voltage + ApplyLatency(baseRise) / volDivisor;
						bit0Voltage = voltage - baseDrop / volDivisor;
						break;
					case 1:
						bit1Voltage = voltage + baseRise / volDivisor;
						bit0Voltage = voltage - ApplyLatency(baseDrop) / volDivisor;
						break;
				}
						
				if (abs(bit0Voltage - targetVoltage) > abs(bit1Voltage - targetVoltage)) {
					// bit1 is closer
					word |= (1 << b);
					bitState = 1;
					voltage = bit1Voltage;
				} else {
					bitState = 0;
					voltage = bit0Voltage;
				}
			}

			int entry = (targetVoltage / 16) * 2 + rollingBitState;
			btcTable[entry].bits = word;
			btcTable[entry].voltageOffset = voltage - 16384;
		}

	}
}

inline unsigned char LookupBTC(int& curVoltage, int destVoltage, unsigned short lastBits) {
	int voltageOffset = (destVoltage + 16384 - curVoltage);
	const BTCEntry& entry = btcTable[(voltageOffset / 16) * 2 + (lastBits & 1)];
	curVoltage += entry.voltageOffset;
	if (curVoltage < minVoltage) curVoltage = minVoltage;
	else if (curVoltage > maxVoltage) curVoltage = maxVoltage;
	return entry.bits;
}

// initializes the platform sound system, called when emulation begins. Returns false on error
bool sndInit() {
	unsigned char settings[5] = { 0,9,0,0,0 };//115200,1xstop bits
	if (Serial_Open(&settings[0]))
	{
		return false;
	}

	SCIF2.SCSMR.BIT.CKS = 0;	//1/1
	SCIF2.SCSMR.BIT.CA = 1;		//Synchronous mode is 16 times faster

	sampleNum = 0;
	lastSoundCounter = 0x7FFFFFFF;

	// tune bitrate based on clock speed
	if (getDeviceType() == DT_CG50) {
		SCIF2.SCBRR = 32;
	} else {
		switch (Ptune2_GetSetting()) {
			case PT2_DEFAULT:
			case PT2_NOMEMWAIT:
				SCIF2.SCBRR = 16;
				break;
			case PT2_HALFINC:
				SCIF2.SCBRR = 24;
				break;
			case PT2_DOUBLE:
				SCIF2.SCBRR = 32;
				break;
		}
	}

	btcTable = (BTCEntry*) malloc(sizeof(BTCEntry) * 4096);
	computeBTCTable();

	return true;
}

static void feed() {
	sndFrame(curSoundBuffer, BUFF_SIZE);
	sampleNum = 0;
}

// platform update from emulator for sound system, called 8 times per frame (should be enough!)
void sndUpdate() {
	{
		int toAdd = Serial_PollTX() & 0x1FE;

		// perform this update every 1/360th of a second or so
		lastSoundCounter = REG_TMU_TCNT_1 - 40;

		if (toAdd > 0)
		{
			TIME_SCOPE();

			static int curVoltage = 0;
			static int curSample = 0;

			if (toAdd == 256) {
				// reset voltage because we missed too many samples
				curVoltage = curSample;
			}

			unsigned char writeBuffer[256];
			int target1 = 0, target2 = 0;
			for (int iter = 0; iter < toAdd; iter += 2) {
				if (sampleNum == BUFF_SIZE) {
					feed();
					sampleNum = 0;
				}
				int lastSample = curSample;
				const int sampleScale = sndSettings[1];
				int baseVoltage = 8192 - sampleScale * 512;
				curSample = curSoundBuffer[sampleNum] * sampleScale / 16 + baseVoltage;
				sampleNum++;
				
				switch (sndSettings[4]) {
					case 0:
					{
						// no blending
						target1 = curSample;
						target2 = curSample;
						break;
					}
					case 1:
					{
						// single average
						target1 = (curSample + lastSample) >> 1;
						target2 = curSample;
						break;
					}
					case 2:
					{
						// walking average
						target1 = (curSample + lastSample * 3) >> 2;
						target2 = (curSample * 3 + lastSample) >> 2;
						break;
					}
					case 3:
					{
						// embedded average
						target1 = (curSample + lastSample * 2 + curVoltage) >> 2;
						target2 = (curSample * 2 + lastSample + curVoltage) >> 2;
						break;
					}
				}

				static unsigned char bits = 0;
				bits = LookupBTC(curVoltage, target1, bits);
				writeBuffer[iter] = bits;
				bits = LookupBTC(curVoltage, target2, bits);
				writeBuffer[iter + 1] = bits;
			}

			Serial_Write(writeBuffer, toAdd);
		}
	}
}

// cleans up the platform sound system, called when emulation ends
void sndCleanup() {
	Serial_Close(1);

	if (btcTable) {
		free((void*)btcTable);
		btcTable = NULL;
	}
}

const int perVolumeSettings[4][5] = {
	{ 2,6,15,4,2 },
	{ 3,4,14,4,2 },
	{ 4,3,14,8,2 },
	{ 5,6,12,33,1 },
};

void updateSettings() {
	const int* settings = perVolumeSettings[sndSettings[0] - 2];
	for (int i = 0; i < numSoundSettings; i++) {
		sndSettings[i] = settings[i];
	}

	computeBTCTable();
}

void sndVolumeUp() {
	if (sndSettings[0] < 5) {
		sndSettings[0]++;

		updateSettings();
	}
}


void sndVolumeDown() {
	if (sndSettings[0] > 2) {
		sndSettings[0]--;

		updateSettings();
	}
}

#endif