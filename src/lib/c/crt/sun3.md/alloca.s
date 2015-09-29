|
|   alloca.s
|

        .globl _alloca
_alloca:	
        movl	sp@,a0
	movl	sp@(4),d0
	addqw	#3,d0
	andl	#-4,d0
	subl	d0,sp
	movl	sp,d0
	addqw	#8,d0
	jmp	a0@

