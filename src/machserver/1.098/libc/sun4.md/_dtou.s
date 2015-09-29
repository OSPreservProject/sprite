
        .text
	.globl  __dtou
__dtou:
	save	%sp, -0x50, %sp
	st	%i0, [%fp + 0x44]
	ld	[%fp + 0x44], %f30
	st	%i1, [%fp + 0x48]
	ld	[%fp + 0x48], %f31
	sethi	%hi(3f), %o0
	ldd	[%o0 + %lo(3f)], %f0
	fcmped	%f30, %f0
	nop
	fbul,a	1f
	fdtoi	%f30, %f6
	sethi	%hi(3f), %o1
	ldd	[%o1 + %lo(3f)], %f2
	fsubd	%f30, %f2, %f4
	sethi	%hi(0x80000000), %o3
	fdtoi	%f4, %f5
	st	%f5, [%sp + 0x48]
	ld	[%sp + 0x48], %i5
	ba	2f
	or	%i5, %o3, %i5
1:
	st	%f6, [%sp + 0x48]
	ld	[%sp + 0x48], %i5
2:
	ret
	restore	%g0, %i5, %o0

3:
    .long       0x41e00000
    .long       0x00000000

