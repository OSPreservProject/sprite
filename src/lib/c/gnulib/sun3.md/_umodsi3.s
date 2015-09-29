	.data
	.text
LL0:
|#PROC# 04

	LF12	=	0
	LS12	=	0
	LFF12	=	0
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___umodsi3
___umodsi3:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	movl	a6@(8),d0
	divull	a6@(12),d1:d0
	movl	d1,d0
	unlk	a6
	rts
