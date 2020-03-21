
# void BitsToScanline_Palette(unsigned char* scanline, unsigned int bits, unsigned int palette)
# r4 = scanline
# r5 = bits
# r6 = palette

    .global _BitsToScanline_Palette
    .text; .align 4
_BitsToScanline_Palette:
	mov.l .BTS_Mask, r0
	swap.b r6,r2
	or r2,r6
	mov r5,r1
	swap.w r6,r2
	or r2,r6
	and r0,r1
	or r6,r1
	mov.l r1,@r4
	shlr2 r5
	and r5,r0
	or r6,r0
	rts
	mov.l r0,@(4,r4)
    .text; .align 4
.BTS_Mask:
	.long	0x0C0C0C0C

    .END    