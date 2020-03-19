
# void BitsToScanline_Palette(unsigned int bits, int* scanline, int palette);
# r4 = bits
# r5 = scanline
# r6 = palette

    .global _BitsToScanline_Palette
    .text; .align 4
_BitsToScanline_Palette:
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(0, r5)		// scanline[0] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(4, r5)		// scanline[1] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(8, r5)		// scanline[2] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(12, r5)		// scanline[3] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(16, r5)		// scanline[4] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(20, r5)		// scanline[5] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	or r6,r0
	mov.l r0, @(24, r5)		// scanline[6] = r0
	shlr2 r4				// bits >>= 2

	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	
	or r6,r0				// or the palette directly into r4 now
    rts
	mov.l r0, @(28, r5)		// scanline[7] = bits
    .END    