
#if !TARGET_WINSIM

// initializes the platform sound system, called when emulation begins. Returns false on error
bool sndInit() {
}

// platform update from emulator for sound system, called 8 times per frame (should be enough!)
void sndUpdate() {
}

// cleans up the platform sound system, called when emulation ends
void sndCleanup() {
}

#endif