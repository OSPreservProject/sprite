
	.text
	.globl	___cmpdf2
___cmpdf2:
	link	a6,#0
	movl	a6@(8),d0
	movl	a6@(12),d1
	lea	a6@(16),a0
	jsr	Fcmpd
	jle	L14
	moveq	#1,d0
	jra	LE12
L14:
	movl	a6@(8),d0
	movl	a6@(12),d1
	lea	a6@(16),a0
	jsr	Fcmpd
	jge	L15
	moveq	#-1,d0
	jra	LE12
L15:
	moveq	#0,d0
LE12:
	unlk	a6
	rts
