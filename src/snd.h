#pragma once

// sound baud rate
#define SOUND_RATE 8192

// initializes the platform sound system, called when emulation begins. Returns false on error
bool sndInit();

// platform update from emulator for sound system, called 8 times per frame (should be enough!)
void sndUpdate();

// cleans up the platform sound system, called when emulation ends
void sndCleanup();

// called on rom start up to initialize sound registers
void sndStartup();

// called from the platform sound system to fill a 1/256 second buffer based on current sound values
void sndFrame(unsigned char* buffer, int length);