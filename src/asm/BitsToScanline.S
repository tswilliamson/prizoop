
# void BitsToScanline(unsigned int bits, int* scanline);
# r4 = bits
# r5 = scanline

    .global _BitsToScanline
    .text; .align 4
_BitsToScanline:
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(0, r5)		// scanline[0] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(4, r5)		// scanline[1] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(8, r5)		// scanline[2] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(12, r5)		// scanline[3] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(16, r5)		// scanline[4] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(20, r5)		// scanline[5] = r0
	shlr2 r4				// bits >>= 2
	
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	mov.l r0, @(24, r5)		// scanline[6] = r0
	shlr2 r4				// bits >>= 2
	mov r4,r0				// r0 = bits & 12
	and #12,r0 
	
    rts
	mov.l r0, @(28, r5)		// scanline[7] = bits
    .END    