|
|   _setjmp.s
|

        .globl __setjmp
__setjmp:
	movl	sp@(4),a0
	movl	sp@(0),a0@
	clrl	a0@(4)
	clrl	a0@(8)
	moveml	d2/d3/d4/d5/d6/d7/a2/a3/a4/a5/a6/a7,a0@(0xc)
	clrl	d0
	rts

	.globl __longjmp
__longjmp:		
        movl	sp@(4),a0
	movl	sp@(8),d0
	bne	1f
	moveq	#1,d0
1:
	moveml	a0@(0xc),d2/d3/d4/d5/d6/d7/a2/a3/a4/a5/a6/a7
	movl	a0@,sp@
	rts

