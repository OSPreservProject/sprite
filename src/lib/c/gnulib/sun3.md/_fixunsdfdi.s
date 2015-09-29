	.data
	.text
LL0:
|#PROC# 07

	LF12	=	8
	LS12	=	0
	LFF12	=	8
	LSS12	=	0
	LP12	=	8
	.data
	.text
	.globl	___fixunsdfdi
___fixunsdfdi:
|#PROLOGUE# 0

	link	a6,#-8
|#PROLOGUE# 1

	jsr	Fund
	movl	d0,a6@(-8)
	clrl	a6@(-4)
	movl	a6@(-4),d1
	unlk	a6
	rts
