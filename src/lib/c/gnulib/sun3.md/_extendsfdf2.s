	.data
	.text
LL0:
|#PROC# 07

	LF12	=	4
	LS12	=	0
	LFF12	=	4
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___extendsfdf2
___extendsfdf2:
|#PROLOGUE# 0

	link	a6,#-4
|#PROLOGUE# 1

	movl	a6@(8),d0
	jsr	Fstod
	unlk	a6
	rts
