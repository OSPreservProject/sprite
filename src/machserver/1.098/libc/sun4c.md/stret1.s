!
!       stret1.s
!

        .globl .stret1

.stret1:
	ld	[%i7 + 0x8], %o3
	and	%o1, 0xfff, %o4
	sethi	%hi(0x0), %o5
	or	%o4, %o5, %o5
	cmp	%o5, %o3
	bne	2f
	ld	[%fp + 0x40], %i0
1:
	subcc	%o1, 0x1, %o1
	ldub	[%o0 + %o1], %o4
	bg	1b
	stb	%o4, [%i0 + %o1]
	add	%i7, 0x4, %i7
2:
	ret
	restore

