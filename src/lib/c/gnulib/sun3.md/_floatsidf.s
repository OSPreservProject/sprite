	.data
	.text
LL0:
|#PROC# 07

	LF12	=	0
	LS12	=	0
	LFF12	=	0
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___floatsidf
___floatsidf:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	movl	a6@(8),d0
	jsr	Ffltd
	unlk	a6
	rts
