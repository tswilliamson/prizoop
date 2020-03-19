
extern unsigned short MortonTable[256];
extern unsigned short ReverseMortonTable[256];

#if TARGET_WINSIM

FORCE_INLINE void resolveTileRow(int* scanline, unsigned int tileRow) {
	unsigned int bits = (ReverseMortonTable[tileRow >> 8] << 2) | (ReverseMortonTable[tileRow & 0xFF] << 3);

	scanline[0] = bits & 12; bits >>= 2;
	scanline[1] = bits & 12; bits >>= 2;
	scanline[2] = bits & 12; bits >>= 2;
	scanline[3] = bits & 12; bits >>= 2;
	scanline[4] = bits & 12; bits >>= 2;
	scanline[5] = bits & 12; bits >>= 2;
	scanline[6] = bits & 12; bits >>= 2;
	scanline[7] = bits & 12; 
}

FORCE_INLINE void resolveTileRowReverse(int* scanline, unsigned int tileRow) {
	unsigned int bits = (MortonTable[tileRow >> 8] << 2) | (MortonTable[tileRow & 0xFF] << 3);

	scanline[0] = bits & 12; bits >>= 2;
	scanline[1] = bits & 12; bits >>= 2;
	scanline[2] = bits & 12; bits >>= 2;
	scanline[3] = bits & 12; bits >>= 2;
	scanline[4] = bits & 12; bits >>= 2;
	scanline[5] = bits & 12; bits >>= 2;
	scanline[6] = bits & 12; bits >>= 2;
	scanline[7] = bits & 12;
}

FORCE_INLINE void resolveTileRowPal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int bits = (ReverseMortonTable[tileRow >> 8] << 2) | (ReverseMortonTable[tileRow & 0xFF] << 3);

	scanline[0] = palette | (bits & 12); bits >>= 2;
	scanline[1] = palette | (bits & 12); bits >>= 2;
	scanline[2] = palette | (bits & 12); bits >>= 2;
	scanline[3] = palette | (bits & 12); bits >>= 2;
	scanline[4] = palette | (bits & 12); bits >>= 2;
	scanline[5] = palette | (bits & 12); bits >>= 2;
	scanline[6] = palette | (bits & 12); bits >>= 2;
	scanline[7] = palette | (bits & 12);
}

FORCE_INLINE void resolveTileRowReversePal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int bits = (MortonTable[tileRow >> 8] << 2) | (MortonTable[tileRow & 0xFF] << 3);

	scanline[0] = palette | (bits & 12); bits >>= 2;
	scanline[1] = palette | (bits & 12); bits >>= 2;
	scanline[2] = palette | (bits & 12); bits >>= 2;
	scanline[3] = palette | (bits & 12); bits >>= 2;
	scanline[4] = palette | (bits & 12); bits >>= 2;
	scanline[5] = palette | (bits & 12); bits >>= 2;
	scanline[6] = palette | (bits & 12); bits >>= 2;
	scanline[7] = palette | (bits & 12);
}

#else

extern "C" {
	void BitsToScanline(unsigned int bits, int* scanline);
	void BitsToScanline_Palette(unsigned int bits, int* scanline, int palette);
};


FORCE_INLINE void resolveTileRow(int* scanline, unsigned int tileRow) {
	unsigned int bits = (ReverseMortonTable[tileRow >> 8] << 2) | (ReverseMortonTable[tileRow & 0xFF] << 3);
	BitsToScanline(bits, scanline);
}

FORCE_INLINE void resolveTileRowReverse(int* scanline, unsigned int tileRow) {
	unsigned int bits = (MortonTable[tileRow >> 8] << 2) | (MortonTable[tileRow & 0xFF] << 3);
	BitsToScanline(bits, scanline);
}

FORCE_INLINE void resolveTileRowPal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int bits = (ReverseMortonTable[tileRow >> 8] << 2) | (ReverseMortonTable[tileRow & 0xFF] << 3);
	BitsToScanline_Palette(bits, scanline, palette);
}

FORCE_INLINE void resolveTileRowReversePal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int bits = (MortonTable[tileRow >> 8] << 2) | (MortonTable[tileRow & 0xFF] << 3);
	BitsToScanline_Palette(bits, scanline, palette);
}

#endif