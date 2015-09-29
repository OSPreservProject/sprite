	.data
	.text
LL0:
|#PROC# 0

	LF12	=	0
	LS12	=	0
	LFF12	=	0
	LSS12	=	0
	LP12	=	12
	.data
	.text
	.globl	___builtin_delete
___builtin_delete:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	tstl	a6@(8)
	jeq	LE12
	movl	a6@(8),sp@-
	jbsr	_free
	addqw	#4,sp
LE12:
	unlk	a6
	rts
|#PROC# 0

	LF17	=	12
	LS17	=	0
	LFF17	=	12
	LSS17	=	0
	LP17	=	16
	.data
	.text
	.globl	___builtin_vec_delete
___builtin_vec_delete:
|#PROLOGUE# 0

	link	a6,#-12
|#PROLOGUE# 1

	movl	a6@(12),d0
	addql	#1,d0
	movl	d0,a6@(-8)
	movl	a6@(8),a6@(-12)
	mulsl	a6@(16),d0
	addl	d0,a6@(8)
	clrl	a6@(-4)
	jra	LY00000
LY00001:
	movl	a6@(16),d0
	subl	d0,a6@(8)
	movl	a6@(28),sp@-
	movl	a6@(8),sp@-
	movl	a6@(20),a0
	jsr	a0@
	addqw	#8,sp
	addql	#1,a6@(-4)
LY00000:
	movl	a6@(-4),d0
	cmpl	a6@(-8),d0
	jlt	LY00001
	tstl	a6@(24)
	jeq	LE17
	movl	a6@(-12),sp@-
	jbsr	_free
	addqw	#4,sp
LE17:
	unlk	a6
	rts
