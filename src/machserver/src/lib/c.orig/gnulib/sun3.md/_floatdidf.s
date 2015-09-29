	.data
	.text
LL0:
|#PROC# 07

	LF12	=	16
	LS12	=	0
	LFF12	=	16
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___floatdidf
___floatdidf:
|#PROLOGUE# 0

	link	a6,#-16
|#PROLOGUE# 1

	movl	a6@(12),d0
	jsr	Ffltd
	lea	L2000000,a0
	jsr	Fmuld
	lea	L2000000,a0
	jsr	Fmuld
	movl	d0,a6@(-8)
	movl	d1,a6@(-4)
	movl	a6@(8),d0
	jsr	Ffltd
	tstl	d0
	jpl	LX1000000
	lea	L2000001,a0
	jsr	Faddd
LX1000000:
	movl	d0,a6@(-16)
	movl	d1,a6@(-12)
	movl	a6@(-8),d0
	movl	a6@(-4),d1
	lea	a6@(-16),a0
	jsr	Faddd
	unlk	a6
	rts
L2000000:
	.long	1089470464,0
L2000001:
	.long	1106247680,0
