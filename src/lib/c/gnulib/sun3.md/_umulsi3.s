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
	.globl	___umulsi3
___umulsi3:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	movl	a6@(8),d0
	mulul	a6@(12),d0
	unlk	a6
	rts
