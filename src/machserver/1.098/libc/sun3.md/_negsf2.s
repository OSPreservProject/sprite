	.data
	.text
LL0:
|#PROC# 04

	LF12	=	4
	LS12	=	0
	LFF12	=	4
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___negsf2
___negsf2:
|#PROLOGUE# 0

	link	a6,#-4
|#PROLOGUE# 1

	movl	a6@(8),d0
	bchg	#31,d0
	movl	d0,a6@(-4)
	unlk	a6
	rts
