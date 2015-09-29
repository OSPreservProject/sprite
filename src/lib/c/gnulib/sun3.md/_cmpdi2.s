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
	.globl	___cmpdi2
___cmpdi2:
|#PROLOGUE# 0

	link	a6,#0
|#PROLOGUE# 1

	movl	a6@(12),d0
	cmpl	a6@(20),d0
	jlt	LY00001
	cmpl	a6@(20),d0
	jgt	LY00000
	movl	a6@(8),d0
	cmpl	a6@(16),d0
	jcc	L16
LY00001:
	moveq	#0,d0
	jra	LE12
L16:
	movl	a6@(8),d0
	cmpl	a6@(16),d0
	jls	L17
LY00000:
	moveq	#2,d0
	jra	LE12
L17:
	moveq	#1,d0
LE12:
	unlk	a6
	rts
