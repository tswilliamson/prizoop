
#include "platform.h"
#include "debug.h"
#include "cpu.h"
#include "snd.h"

struct sound_status {
	int ch1EnvCounter;
	int ch1SweepCounter;
	int ch1Volume;

	int ch2EnvCounter;
	int ch2Volume;

	int ch4EnvCounter;
	int ch4Volume;
	int ch4LFSR;
};

sound_status snd;

// wave pattern iterator
static int sndIter = 0;

// sound properties iterator
static int propIter = 0;

const int LENGTH_STEP = (SOUND_RATE / 256);
const int SWEEP_STEP = LENGTH_STEP * 2;
const int ENV_STEP = SWEEP_STEP * 2;
const int FREQ_FACTOR = 131072 * 8 / SOUND_RATE;

const int waveduty[4][8] = {
	{ 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 1, 1, 0, 0,},
};

void sndStartup() {
	memset(&snd, 0, sizeof(snd));
}

// called from the platform sound system to fill a buffer based on current sound values
void sndFrame(unsigned char* buffer, int buffSize) {
	propIter = (propIter + 1) % 4;

	memset(buffer, 0, buffSize);

	if ((cpu.memory.NR52_soundmast & 0x80) == 0) {
		cpu.memory.NR52_soundmast = 0;	// make sure channel on flags are reset
		return;
	}

	// master volume 0-14
	const int masterVol = (cpu.memory.NR50_spkvol & 0x07) + ((cpu.memory.NR50_spkvol & 0x70) >> 4);

	// skip if nothing will output
	if (masterVol == 0 || cpu.memory.NR51_chselect == 0)
		return;

	// check for channel inits
	if (cpu.memory.NR14_snd1ctl & 0x80) {
		// init channel 1
		cpu.memory.NR52_soundmast |= 0x01;							// set not out of length
		cpu.memory.NR14_snd1ctl &= 0x7F;							// disable init flag
		snd.ch1EnvCounter = 0;										// reset envelope counter
		snd.ch1SweepCounter = 0;									// reset sweep counter
		snd.ch1Volume = ((cpu.memory.NR12_snd1env & 0xF0) >> 4);	// use initial volume
	}
	if (cpu.memory.NR24_snd2ctl & 0x80) {
		// init channel 2
		cpu.memory.NR52_soundmast |= 0x02;							// set not out of length
		cpu.memory.NR24_snd2ctl &= 0x7F;							// disable init flag
		snd.ch2EnvCounter = 0;										// reset envelope counter
		snd.ch2Volume = ((cpu.memory.NR22_snd2env & 0xF0) >> 4);	// use initial volume
	}
	if (cpu.memory.NR34_snd3ctl & 0x80) {
		// init channel 3
		cpu.memory.NR52_soundmast |= 0x04;							// set not out of length
		cpu.memory.NR34_snd3ctl &= 0x7F;							// disable init flag
	}
	if (cpu.memory.NR44_snd4ctl & 0x80) {
		// init channel 4
		cpu.memory.NR52_soundmast |= 0x08;							// set not out of length
		cpu.memory.NR44_snd4ctl &= 0x7F;							// disable init flag
		snd.ch4EnvCounter = 0;										// reset envelope counter
		snd.ch4Volume = ((cpu.memory.NR42_snd4env & 0xF0) >> 4);	// use initial volume
		snd.ch4LFSR = cpu.clocks >> 4;
	}

	// sound channel 1
	int ch1UseLength = cpu.memory.NR14_snd1ctl & 0x40;
	if (!ch1UseLength || (cpu.memory.NR52_soundmast & 0x01)) {	// not using or not out of length yet
		// selected duty cycle
		const int* duty = &waveduty[(cpu.memory.NR11_snd1len & 0xC0) >> 6][0];
		
		// determine rate to frequency conversion
		int freq = (cpu.memory.NR13_snd1frqlo | ((cpu.memory.NR14_snd1ctl & 0x07) << 8));
		int invFreqFactor = 1024 * FREQ_FACTOR / (2048 - freq);

		// volume is multiple of channel and master volume (divided by 4 to allow 4 channels)
		int vol = (snd.ch1Volume * masterVol) >> 2;

		// kill volume if no speakers are used
		vol = ((cpu.memory.NR51_chselect & 0x01) || (cpu.memory.NR51_chselect & 0x10)) ? vol : 0;

		// fill our sound buffer
		int j = sndIter;
		for (int i = 0; i < buffSize; j++, i++) {
			buffer[i] = vol * duty[((j * invFreqFactor) >> 10) % 8];
		}

		// if length is in use, "decrement" it until sound is done
		if (ch1UseLength) {
			int length = cpu.memory.NR11_snd1len & 0x3F;
			length++;
			if (length == 64) {
				// length finished!
				length = 0;
				cpu.memory.NR52_soundmast &= ~0x01;
			}
			cpu.memory.NR11_snd1len = (cpu.memory.NR11_snd1len & 0xC0) | length;
		}
	}

	// sound channel 2
	int ch2UseLength = cpu.memory.NR24_snd2ctl & 0x40;
	if (!ch2UseLength || (cpu.memory.NR52_soundmast & 0x02)) {	// not using or not out of length yet
		// selected duty cycle
		const int* duty = &waveduty[(cpu.memory.NR21_snd2len & 0xC0) >> 6][0];

		// determine rate to frequency conversion
		int freq = 2048 - (cpu.memory.NR23_snd2frqlo | ((cpu.memory.NR24_snd2ctl & 0x07) << 8));
		int invFreqFactor = 1024 * FREQ_FACTOR / freq;

		// volume is mult of current channel volume and master volume
		int vol = ((snd.ch2Volume & 0xF) * masterVol) >> 2;

		// kill volume if no speakers are used
		vol = ((cpu.memory.NR51_chselect & 0x02) || (cpu.memory.NR51_chselect & 0x20)) ? vol : 0;

		// fill our sound buffer
		int j = sndIter;
		for (int i = 0; i < buffSize; j++, i++) {
			buffer[i] += vol * duty[((j * invFreqFactor) >> 10) % 8];
		}

		// if length is in use, "decrement" it until sound is done
		if (ch2UseLength) {
			int length = cpu.memory.NR21_snd2len & 0x3F;
			length++;
			if (length == 64) {
				// length finished!
				length = 0;
				cpu.memory.NR52_soundmast &= ~0x02;
			}
			cpu.memory.NR21_snd2len = (cpu.memory.NR21_snd2len & 0xC0) | length;
		}
	}

	// sound channel 3 (wave RAM)
	int ch3UseLength = cpu.memory.NR34_snd3ctl & 0x40;
	if ((!ch3UseLength || (cpu.memory.NR52_soundmast & 0x04)) && (cpu.memory.NR30_snd3enable & 0x80)) {	// not using or not out of length yet, AND enabled
		// determine rate to frequency conversion
		int freq = 2048 - (cpu.memory.NR33_snd3frqlo | ((cpu.memory.NR34_snd3ctl & 0x07) << 8));
		int invFreqFactor = 1024 * FREQ_FACTOR / freq;

		// volume is master volume since we use a bitshift w/ pattern RAM
		int vol = masterVol;
		int volBit = (cpu.memory.NR32_snd3vol & 0x60) >> 5;

		// kill volume if no speakers are used or 0 bit shift is selected
		vol = ((cpu.memory.NR51_chselect & 0x04) || (cpu.memory.NR51_chselect & 0x40)) && volBit ? vol : 0;

		// fill our sound buffer
		int j = sndIter;
		volBit += 1;			// 1/4th per channel
		for (int i = 0; i < buffSize; j += 2, i += 2) {
			int samp = ((j * invFreqFactor) >> 9) % 32;
			buffer[i] += (vol * ((samp & 1) ? (cpu.memory.WAVE_ptr[samp/2] & 0x0F) : ((cpu.memory.WAVE_ptr[samp/2] & 0xF0) >> 4))) >> volBit;
		}

		// if length is in use, "decrement" it until sound is done
		if (ch2UseLength) {
			int length = cpu.memory.NR31_snd3len;
			length++;
			if (length == 256) {
				// length finished!
				length = 0;
				cpu.memory.NR52_soundmast &= ~0x04;
			}
			cpu.memory.NR31_snd3len = length;
		}
	}

	// sound channel 4
	int ch4UseLength = cpu.memory.NR44_snd4ctl & 0x40;
	if (!ch4UseLength || (cpu.memory.NR52_soundmast & 0x08)) {	// not using or not out of length yet
		// determine rate to frequency conversion
		const int divTable[8] = { 8, 16, 32, 48, 64, 80, 96, 112 };
		int freq = divTable[cpu.memory.NR43_snd4cnt & 7] << ((cpu.memory.NR43_snd4cnt & 0xF0) >> 4);
		int invFreqFactor = 512 * FREQ_FACTOR / freq;

		// volume is mult of current channel volume and master volume
		int vol = ((snd.ch4Volume & 0xF) * masterVol) >> 2;

		// kill volume if no speakers are used
		vol = ((cpu.memory.NR51_chselect & 0x08) || (cpu.memory.NR51_chselect & 0x80)) ? vol : 0;

		// fill our sound buffer
		int j = sndIter;
		int last = ((j * invFreqFactor) >> 7);
		if (cpu.memory.NR43_snd4cnt & 0x80) {
			// 7 bit shift
			for (int i = 0; i < buffSize; j++, i++) {
				int cur = ((j * invFreqFactor) >> 7);
				if (last != cur) {
					int xorBit = ((snd.ch4LFSR & 0x02) >> 1) ^ (snd.ch4LFSR & 0x01);
					snd.ch4LFSR = ((snd.ch4LFSR >> 1) && 0x7F7F) | (xorBit << 15) | (xorBit << 7);
					last = cur;
				}
				buffer[i] += ((snd.ch4LFSR & 0xF) * vol) >> 4;
			}
		} else {
			// 15 bit shift
			for (int i = 0; i < buffSize; j++, i++) {
				int cur = ((j * invFreqFactor) >> 7);
				if (last != cur) {
					int xorBit = ((snd.ch4LFSR & 0x02) >> 1) ^ (snd.ch4LFSR & 0x01);
					snd.ch4LFSR = (snd.ch4LFSR >> 1) | (xorBit << 15);
					last = cur;
				}
				buffer[i] += ((snd.ch4LFSR & 0xF) * vol) >> 4;
			}
		}

		// if length is in use, "decrement" it until sound is done
		if (ch4UseLength) {
			int length = cpu.memory.NR41_snd4len;
			length++;
			if (length == 256) {
				// length finished!
				length = 0;
				cpu.memory.NR52_soundmast &= ~0x08;
			}
			cpu.memory.NR41_snd4len = length;
		}
	}

	// sweep step every 2 frames
	if ((propIter & 1) == 0) {
		int ch1Sweep = (cpu.memory.NR10_snd1sweep & 0x70) >> 4;
		if (ch1Sweep) {
			if (++snd.ch1SweepCounter >= ch1Sweep) {
				// change channel 1 frequency
				int freq = (cpu.memory.NR13_snd1frqlo | ((cpu.memory.NR14_snd1ctl & 0x07) << 8));

				snd.ch1SweepCounter = 0;
				int bits = cpu.memory.NR10_snd1sweep & 0x07;
				if (cpu.memory.NR10_snd1sweep & 0x08) {
					// decrease
					freq -= (freq >> bits);
				}
				else {
					freq += (freq >> bits);
				}

				// check for freq overflow
				if (freq <= 0) {
					freq = 0;
					cpu.memory.NR52_soundmast &= ~0x01;
				}
				else if (freq >= 2048) {
					freq = 2047;
					cpu.memory.NR52_soundmast &= ~0x01;
				}

				cpu.memory.NR13_snd1frqlo = freq & 0xFF;
				cpu.memory.NR14_snd1ctl = (cpu.memory.NR14_snd1ctl & 0xF8) | (freq >> 8);
			}
		}
	}

	// envelope step every 4 frames
	if (propIter == 0) {
		int ch1Envelope = cpu.memory.NR12_snd1env & 0x07;
		if (ch1Envelope) {
			if (++snd.ch1EnvCounter >= ch1Envelope) {
				snd.ch1EnvCounter = 0;
				if (cpu.memory.NR12_snd1env & 0x08) {
					if (snd.ch1Volume < 15) snd.ch1Volume++;
				}
				else {
					// decrease
					if (snd.ch1Volume) snd.ch1Volume--;
				}
			}
		}

		int ch2Envelope = cpu.memory.NR22_snd2env & 0x07;
		if (ch2Envelope) {
			if (++snd.ch2EnvCounter >= ch2Envelope) {
				snd.ch2EnvCounter = 0;
				if (cpu.memory.NR22_snd2env & 0x08) {
					if (snd.ch2Volume < 15) snd.ch2Volume++;
				} else {
					// decrease
					if (snd.ch2Volume) snd.ch2Volume--;
				}
			}
		}

		int ch4Envelope = cpu.memory.NR42_snd4env & 0x07;
		if (ch4Envelope) {
			if (++snd.ch4EnvCounter >= ch4Envelope) {
				snd.ch4EnvCounter = 0;
				if (cpu.memory.NR42_snd4env & 0x08) {
					if (snd.ch4Volume < 15) snd.ch4Volume++;
				} else {
					// decrease
					if (snd.ch4Volume) snd.ch4Volume--;
				}
			}
		}
	}

	// wave pattern iterator for correct offset in next frame
	sndIter = (sndIter + buffSize) & 0xFFFF;	
}