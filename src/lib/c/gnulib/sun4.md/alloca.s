!
!       alloca.s
!

        .text
	.globl  ___builtin_alloca
___builtin_alloca:
	mov	%sp, %o3
	sub	%sp, %o0, %sp
	ba	2f
	subcc	%o2, %o1, %o5
1:
	ld	[%o3 + %o1], %o4
	st	%o4, [%sp + %o1]
	add	%o1, 0x4, %o1
2:
	bg	1b
	subcc	%o5, 0x4, %o5
	jmp	%o7 + 0x8
	add	%sp, %o2, %o0

