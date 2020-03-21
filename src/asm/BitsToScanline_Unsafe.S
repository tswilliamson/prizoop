
# void BitsToScanline_Unsafe(unsigned char* scanline, unsigned int bits, unsigned int palette)
# r4 = scanline
# r5 = bits
# r6 = palette

    .global _BitsToScanline_Unsafe
    .text; .align 4
_BitsToScanline_Unsafe:
	mov.l .BTS_Mask, r1
	swap.b r6,r2
	or r2,r6
	mov r5,r0
	swap.w r6,r2
	or r2,r6
	and r1,r0
	or r6,r0
	mov.b r0,@(3,r4)
	shlr8 r0
	mov.b r0,@(2,r4)
	shlr8 r0
	mov.b r0,@(1,r4)
	shlr8 r0
	mov.b r0,@(0,r4)
	shlr2 r5
	and r5,r1
	or r6,r1
	mov r1,r0
	mov.b r0,@(7,r4)
	shlr8 r0
	mov.b r0,@(6,r4)
	shlr8 r0
	mov.b r0,@(5,r4)
	shlr8 r0
	rts
	mov.b r0,@(4,r4)
    .text; .align 4
.BTS_Mask:
	.long	0x0C0C0C0C

    .END    