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
	.globl	___fixdfdi
___fixdfdi:
|#PROLOGUE# 0

	link	a6,#-8
|#PROLOGUE# 1

	movl	a6@(8),d0
	movl	a6@(12),d1
	jsr	Fintd
	movl	d0,a6@(-8)
	movl	a6@(8),d0
	movl	a6@(12),d1
	jsr	Fintd
	tstl	d0
	jge	L2000000
	moveq	#-1,d0
	jra	L2000001
L2000000:
	moveq	#0,d0
L2000001:
	movl	d0,a6@(-4)
	movl	a6@(-8),d0
	movl	a6@(-4),d1
	unlk	a6
	rts
