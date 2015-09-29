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
	.globl	___truncdfsf2
___truncdfsf2:
|#PROLOGUE# 0

	link	a6,#-4
|#PROLOGUE# 1

	movl	a6@(8),d0
	movl	a6@(12),d1
	jsr	Fdtos
	movl	d0,a6@(-4)
	unlk	a6
	rts
