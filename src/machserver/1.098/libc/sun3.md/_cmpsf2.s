	.data
	.text
LL0:
	.text
	.globl	___cmpsf2
___cmpsf2:
	link	a6,#0
	movl	a6@(12),d1
	movl	a6@(8),d0
	jsr	Fcmps
	jfngt	L14
	moveq	#1,d0
	jra	LE12
L14:
	movl	a6@(12),d1
	movl	a6@(8),d0
	jsr	Fcmps
	jfnlt	L15
	moveq	#-1,d0
	jra	LE12
L15:
	moveq	#0,d0
LE12:
	unlk	a6
	rts
