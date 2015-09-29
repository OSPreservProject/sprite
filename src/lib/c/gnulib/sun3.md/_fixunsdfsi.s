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
	.globl	___fixunsdfsi
___fixunsdfsi:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	jsr	Fund
	unlk	a6
	rts
