
extern unsigned char BitResolveTable[256 * 4];
extern unsigned char BitResolveTableRev[256 * 4];

#if TARGET_WINSIM

FORCE_INLINE void BitsToScanline(unsigned char* scanline, unsigned int bits) {
	DebugAssert((size_t(scanline) & 3) == 0);

	unsigned int mask = 0x0C0C0C0C;
	((unsigned int*)scanline)[0] = bits & mask;
	bits >>= 2;
	((unsigned int*)scanline)[1] = bits & mask;
}

FORCE_INLINE void BitsToScanline_Palette(unsigned char* scanline, unsigned int bits, unsigned int palette) {
	DebugAssert((size_t(scanline) & 3) == 0);

	palette |= palette << 8;
	palette |= palette << 16;
	unsigned int mask = 0x0C0C0C0C;
	((unsigned int*)scanline)[0] = (bits & mask) | palette;
	bits >>= 2;
	((unsigned int*)scanline)[1] = (bits & mask) | palette;
}

// unsafe alignment (slower!)
FORCE_INLINE void BitsToScanline_Unsafe(unsigned char* scanline, unsigned int bits, unsigned int palette) {
	palette |= palette << 8;
	palette |= palette << 16;
	unsigned char* bitsChar = (unsigned char*) &bits;
	scanline[0] = (bitsChar[0] & 0x0C) | palette;
	scanline[1] = (bitsChar[1] & 0x0C) | palette;
	scanline[2] = (bitsChar[2] & 0x0C) | palette;
	scanline[3] = (bitsChar[3] & 0x0C) | palette;
	scanline[4] = ((bitsChar[0] >> 2) & 0x0C) | palette;
	scanline[5] = ((bitsChar[1] >> 2) & 0x0C) | palette;
	scanline[6] = ((bitsChar[2] >> 2) & 0x0C) | palette;
	scanline[7] = ((bitsChar[3] >> 2) & 0x0C) | palette;
}

#else

extern "C" {
	void BitsToScanline(unsigned char* scanline, unsigned int bits);
	void BitsToScanline_Palette(unsigned char* scanline, unsigned int bits, int palette);
	void BitsToScanline_Unsafe(unsigned char* scanline, unsigned int bits, int palette);
};

#endif

FORCE_INLINE unsigned int GetTableEntry(unsigned int entry) {
	return *((unsigned int*) &BitResolveTable[entry * 4]);
}

FORCE_INLINE unsigned int GetTableEntryRev(unsigned int entry) {
	return *((unsigned int*) &BitResolveTableRev[entry * 4]);
}

template<bool isUnsafe>
FORCE_INLINE void resolveTileRow(unsigned char* scanline, unsigned int tileRow) {
	unsigned int bits = GetTableEntry(tileRow >> 8) | (GetTableEntry(tileRow & 0xFF) << 1);
	isUnsafe ? BitsToScanline_Unsafe(scanline, bits, 0) : BitsToScanline(scanline, bits);
}

template<bool isUnsafe>
FORCE_INLINE void resolveTileRowReverse(unsigned char* scanline, unsigned int tileRow) {
	unsigned int bits = GetTableEntryRev(tileRow >> 8) | (GetTableEntryRev(tileRow & 0xFF) << 1);
	isUnsafe ? BitsToScanline_Unsafe(scanline, bits, 0) : BitsToScanline(scanline, bits);
}

template<bool isUnsafe>
FORCE_INLINE void resolveTileRowPal(unsigned int palette, unsigned char* scanline, unsigned int tileRow) {
	unsigned int bits = GetTableEntry(tileRow >> 8) | (GetTableEntry(tileRow & 0xFF) << 1);
	isUnsafe ? BitsToScanline_Unsafe(scanline, bits, palette) : BitsToScanline_Palette(scanline, bits, palette);
}

template<bool isUnsafe>
FORCE_INLINE void resolveTileRowReversePal(unsigned int palette, unsigned char* scanline, unsigned int tileRow) {
	unsigned int bits = GetTableEntryRev(tileRow >> 8) | (GetTableEntryRev(tileRow & 0xFF) << 1);
	isUnsafe ? BitsToScanline_Unsafe(scanline, bits, palette) : BitsToScanline_Palette(scanline, bits, palette);
}
