
# void resolveTileRow(int* scanline, unsigned int tileRow);
# r4 = scanline
# r5 = tileRow

    .global _resolveTileRow
    .text; .align 4
_resolveTileRow:
	mov r5,r2 // r2 = row1 (tileRow >> 8)
	shlr8 r2
	mov r5,r3 // r3 = row2

	# line 1
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l	r1, @(28, r4)

	# line 2
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l r1, @(24, r4)
	
	# line 3
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l r1, @(20, r4)
	
	# line 4
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l r1, @(16, r4)
	
	# line 5
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l r1, @(12, r4)
	
	# line 6
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l r1, @(8, r4)	
	
	# line 7
	shlr r3
	movt r1
	shlr r2
	rotcl r1
	mov.l r1, @(4, r4)
	
	# line 8
	shlr r3
	movt r1
	shlr r2
	rotcl r1
     rts
	mov.l r1, @r4	
    .END    