
#if TARGET_WINSIM

FORCE_INLINE void resolveTileRow(int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = (tileRow << 1) & 0x1FE;

	scanline[7] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[6] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[5] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[4] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[3] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[2] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[1] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[0] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
}

FORCE_INLINE void resolveTileRowReverse(int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = (tileRow << 1) & 0x1FE;

	scanline[0] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[1] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[2] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[3] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[4] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[5] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[6] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[7] = (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
}

FORCE_INLINE void resolveTileRowPal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = (tileRow << 1) & 0x1FE;

	scanline[7] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[6] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[5] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[4] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[3] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[2] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[1] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[0] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
}

FORCE_INLINE void resolveTileRowReversePal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = (tileRow << 1) & 0x1FE;

	scanline[0] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[1] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[2] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[3] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[4] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[5] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[6] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
	scanline[7] = palette | (row1 & 1) | (row2 & 2);
	row1 >>= 1; row2 >>= 1;
}

#else

FORCE_INLINE void resolveTileRow(int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = tileRow;
	
	asm(
		// line 1
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(28, %0)					\n\t"
		// line 2
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(24, %0)					\n\t"
		// line 3
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(20, %0)					\n\t"
		// line 4
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(16, %0)					\n\t"
		// line 5
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(12, %0)					\n\t"
		// line 6
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(8, %0)					\n\t"
		// line 7
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(4, %0)					\n\t"
		// line 8
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @%0							\n\t"
		// outputs
		: 
		// inputs
		: "r" (scanline), "r" (row2), "r" (row1)
		// clobbers
		: "r1"
	);
}

FORCE_INLINE void resolveTileRowReverse(int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = tileRow;
	
	asm(
		// line 1
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @%0							\n\t"
		// line 2
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(4, %0)					\n\t"
		// line 3
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(8, %0)					\n\t"
		// line 4
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(12, %0)					\n\t"
		// line 5
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(16, %0)					\n\t"
		// line 6
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(20, %0)					\n\t"
		// line 7
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(24, %0)					\n\t"
		// line 8
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"mov.l	r1, @(28, %0)					\n\t"
		// outputs
		: 
		// inputs
		: "r" (scanline), "r" (row2), "r" (row1)
		// clobbers
		: "r1"
	);
}

FORCE_INLINE void resolveTileRowPal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = tileRow;
	
	asm(
		// line 1
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(28, %0)					\n\t"
		// line 2
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(24, %0)					\n\t"
		// line 3
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(20, %0)					\n\t"
		// line 4
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(16, %0)					\n\t"
		// line 5
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(12, %0)					\n\t"
		// line 6
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(8, %0)					\n\t"
		// line 7
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(4, %0)					\n\t"
		// line 8
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @%0							\n\t"
		// outputs
		: 
		// inputs
		: "r" (scanline), "r" (row2), "r" (row1), "r" (palette)
		// clobbers
		: "r1"
	);
}

FORCE_INLINE void resolveTileRowReversePal(int palette, int* scanline, unsigned int tileRow) {
	unsigned int row1 = tileRow >> 8;
	unsigned int row2 = tileRow;
	
	asm(
		// line 1
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @%0							\n\t"
		// line 2
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(4, %0)					\n\t"
		// line 3
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(8, %0)					\n\t"
		// line 4
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(12, %0)					\n\t"
		// line 5
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(16, %0)					\n\t"
		// line 6
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(20, %0)					\n\t"
		// line 7
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(24, %0)					\n\t"
		// line 8
		"shlr %1								\n\t"
		"movt r1								\n\t"
		"shlr %2								\n\t"
		"rotcl r1								\n\t"
		"or %3, r1								\n\t"
		"mov.l	r1, @(28, %0)					\n\t"
		// outputs
		: 
		// inputs
		: "r" (scanline), "r" (row2), "r" (row1), "r" (palette)
		// clobbers
		: "r1"
	);
}


#endif