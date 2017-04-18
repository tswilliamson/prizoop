#if TARGET_WINSIM

#include "platform.h"
#include "debug.h"
#include "snd.h"

#include <Windows.h>

HWAVEOUT device;

#define BUFFER_SIZE SOUND_RATE * 4 / 256
#define NUM_BUFFERS 4

static int corr0 = 0;
static short buffer[NUM_BUFFERS][BUFFER_SIZE];
static int corr1 = 0;
static WAVEHDR headers[NUM_BUFFERS];
static int curBuffer = 0;
static bool sndShutdown = false;

static void prepareBuffer(HWAVEOUT hwo, int bufferNum) {
	static int renderBuffer[BUFFER_SIZE];
	headers[bufferNum].dwFlags &= ~WHDR_DONE;

	int last = renderBuffer[BUFFER_SIZE-1];

	// buffer is 4 frames
	const int quartiles[4] = { BUFFER_SIZE / 4, BUFFER_SIZE / 2, BUFFER_SIZE * 3 / 4, BUFFER_SIZE };
	sndFrame(&renderBuffer[0], quartiles[0]);
	sndFrame(&renderBuffer[quartiles[0]], quartiles[1] - quartiles[0]);
	sndFrame(&renderBuffer[quartiles[1]], quartiles[2] - quartiles[1]);
	sndFrame(&renderBuffer[quartiles[2]], quartiles[3] - quartiles[2]);

	for (int i = BUFFER_SIZE - 1; i > 0; i--) {
		buffer[bufferNum][i] = (renderBuffer[i] + renderBuffer[i - 1]) * 16;
	}
	buffer[bufferNum][0] = (renderBuffer[0] + last) * 16;
}

static void CALLBACK sndProc(
	HWAVEOUT  hwo,
	UINT      uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2
) {
}

// initializes the platform sound system, called when emulation begins
bool sndInit() {
	sndShutdown = false;
	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = SOUND_RATE;
	format.nAvgBytesPerSec = SOUND_RATE;
	format.nBlockAlign = 2;
	format.wBitsPerSample = 16;
	format.cbSize = 0;
	
	MMRESULT result = waveOutOpen(
		&device,
		WAVE_MAPPER,
		&format,
		(DWORD_PTR) sndProc,
		NULL,
		CALLBACK_FUNCTION);

	if (result != 0)
		return false;

	memset(&headers[0], 0, sizeof(headers));
	for (int i = 0; i < NUM_BUFFERS; i++) {
		headers[i].lpData = (LPSTR)&buffer[i][0];
		headers[i].dwBufferLength = BUFFER_SIZE * 2;

		MMRESULT result = waveOutPrepareHeader(device, &headers[i], sizeof(headers[i]));
		DebugAssert(result == 0);

		headers[i].dwFlags |= WHDR_DONE;
	}

	// write all 8 
	curBuffer = 0;

	return true;
}

void sndUpdate() {
	if (headers[curBuffer].dwFlags & WHDR_DONE) {
		prepareBuffer(device, curBuffer);
		waveOutWrite(device, &headers[curBuffer], sizeof(headers[0]));
		curBuffer = (curBuffer + 1) % NUM_BUFFERS;
	}
}

// cleans up the platform sound system, called when emulation ends
void sndCleanup() {
	sndShutdown = true;

	waveOutReset(device);

	for (int i = 0; i < NUM_BUFFERS; i++) {
		MMRESULT result = waveOutUnprepareHeader(device, &headers[i], sizeof(headers[0]));
		DebugAssert(result == 0);
	}

	waveOutClose(device);
}

#endif