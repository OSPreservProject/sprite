	.data
	.text
LL0:
|#PROC# 04

	LF12	=	12
	LS12	=	0
	LFF12	=	12
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___mulsf3
___mulsf3:
|#PROLOGUE# 0

	link	a6,#-12
|#PROLOGUE# 1

	movl	a6@(12),d0
	jsr	Fstod
	movl	d0,a6@(-12)
	movl	d1,a6@(-8)
	movl	a6@(8),d0
	jsr	Fstod
	lea	a6@(-12),a0
	jsr	Fmuld
	jsr	Fdtos
	movl	d0,a6@(-4)
	unlk	a6
	rts
