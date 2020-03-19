
# void BlendTripleScanline24(unsigned short* scanline, int*src0, int* src1, unsigned int* ppuPalette) {
# r4 = scanline
# r5 = src0
# r6 = src1
# r7 = ppuPalette

    .global _BlendTripleScanline24
    .text; .align 4
_BlendTripleScanline24:
	// r8-r13 to the stack
	mov.l	r8,@-r15
	mov.l	r9,@-r15
	mov.l	r10,@-r15
	mov.l	r11,@-r15
	mov.l	r12,@-r15
	mov.l	r13,@-r15
	
	mov r4, r11					// r11 = scanline1
	mov #120, r8
	shll2 r8
	add r8, r4
	mov r4, r12					// r12 = scanline2
	add r8, r4
	mov r4, r13					// r13 = scanline3
	
	mov.w .MixConstant565, r4		// r4 = mix565 constant
	mov.l .MixConstant565_32, r9	// r9 = mix565_32 constant

	mov #40, r8
_BTS24_Inner_Loop:
	mov.l @r5+, r0	
	mov.l @(r0,r7), r1
	mov.w r1, @r11		// scanline1[0] = color00
	mov.l @r5+, r0	
	mov.l @(r0,r7), r2
	mov r2, r0
	mov.w r0, @(4, r11)	// scanline1[2] = color01
	xor r1, r2  // r2 = r1 ^ r0
	add r1, r0	// r0 = r1 + r0
	and r4, r2	// r2 = r2 & mix
	sub r2, r0  // r0 = r0 - r2
	shlr r0
	mov.w r0, @(2, r11)	// scanline1[1] = color00 mixed w/ color01
	
	mov.l @r5+, r0	
	mov.l @(r0,r7), r1
	mov r1, r0
	mov.w r0, @(6, r11)	// scanline1[3] = color02
	mov.l @r5+, r0	
	mov.l @(r0,r7), r2
	mov r2, r0
	mov.w r0, @(10, r11)	// scanline1[5] = color03
	xor r1, r2  // r2 = r1 ^ r0
	add r1, r0	// r0 = r1 + r0
	and r4, r2	// r2 = r2 & mix
	sub r2, r0  // r0 = r0 - r2
	shlr r0
	mov.w r0, @(8, r11)	// scanline1[4] = color02 mixed w/ color03

	mov.l @r6+, r0	
	mov.l @(r0,r7), r1  
	mov.w r1, @r13		// scanline3[0] = color10
	mov.l @r6+, r0	
	mov.l @(r0,r7), r2
	mov r2, r0
	mov.w r0, @(4, r13)	// scanline3[2] = color11
	xor r1, r2  // r2 = r1 ^ r0
	add r1, r0	// r0 = r1 + r0
	and r4, r2	// r2 = r2 & mix
	sub r2, r0  // r0 = r0 - r2
	shlr r0
	mov.w r0, @(2, r13)	// scanline3[1] = color10 mixed w/ color11
	
	mov.l @r6+, r0	
	mov.l @(r0,r7), r1
	mov r1, r0  
	mov.w r0, @(6, r13)	// scanline3[3] = color12
	mov.l @r6+, r0	
	mov.l @(r0,r7), r2
	mov r2, r0
	mov.w r0, @(10, r13)	// scanline3[5] = color13
	xor r1, r2  // r2 = r1 ^ r0
	add r1, r0	// r0 = r1 + r0
	and r4, r2	// r2 = r2 & mix
	sub r2, r0  // r0 = r0 - r2
	shlr r0
	mov.w r0, @(8, r13)	// scanline3[4] = color10 mixed w/ color11

	// now mix565_32 on scanline2 (r12)
	mov.l @r11+, r2
	mov.l @r13+, r1
	mov r2, r0
	xor r1, r2 
	addc r1, r0
	and r9, r2
	sub r2, r0
	rotcr r0
	clrt
	mov.l r0, @r12
	mov.l @r11+, r2
	mov.l @r13+, r1
	mov r2, r0
	xor r1, r2 
	addc r1, r0
	and r9, r2
	sub r2, r0
	rotcr r0
	clrt
	mov.l r0, @(4,r12)
	mov.l @r11+, r2
	mov.l @r13+, r1
	mov r2, r0
	xor r1, r2 
	addc r1, r0
	and r9, r2
	sub r2, r0
	rotcr r0
	mov.l r0, @(8,r12)

	dt r8
	bf/s _BTS24_Inner_Loop
	add #12, r12

	// r13 - r8 from the stack
	mov.l	@r15+,r13
	mov.l	@r15+,r12
	mov.l	@r15+,r11
	mov.l	@r15+,r10
	mov.l	@r15+,r9
	rts	
	mov.l	@r15+,r8
		
    .text; .align 4
.MixConstant565_32:
	.long	0x08210821
.MixConstant565:
	.word	0x0821
	
	/*
// color mixing helpers
inline unsigned int mix565(unsigned int X, unsigned int Y) {
	// RGB565 color mix
	// thanks to http://www.slack.net/~ant/info/rgb_mixing.html
	return (X + Y - ((X ^ Y) & 0x0821)) >> 1;
}

inline unsigned int mix565_32(unsigned int X, unsigned int Y) {
	// RGB565 color mix with two simultaneous colors
	return ((X + Y - ((X ^ Y) & 0x08210821)) >> 1) | ((X & Y & 0x80000000));
}
*/
    .END    
