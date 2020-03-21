
# void BitsToScanline(unsigned char* scanline, unsigned int bits)
# r4 = scanline
# r5 = bits

    .global _BitsToScanline
    .text; .align 4
_BitsToScanline:
	mov.l .BTS_Mask, r0
	mov r5,r1
	and r0,r1
	mov.l r1,@r4
	shlr2 r5
	and r5,r0
	rts
	mov.l r0,@(4,r4)
    .text; .align 4
.BTS_Mask:
	.long	0x0C0C0C0C

    .END    