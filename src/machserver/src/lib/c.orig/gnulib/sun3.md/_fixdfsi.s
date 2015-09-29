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
	.globl	___fixdfsi
___fixdfsi:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	movl	a6@(8),d0
	movl	a6@(12),d1
	jsr	Fintd
	unlk	a6
	rts
