!
!	.seg	"data"
!	.asciz	"@(#)getidprom.s 1.3 88/02/08"
!	Copyright (c) 1986 by Sun Microsystems, Inc.
!

#define IDPROMBASE 0x00000000	
#define IDPROMSIZE 0x20		
#define ASI_CTL 2	
	.align 4
	.seg	"text"
!
! getidprom(addr, size)
!
! Read the ID prom.
! This is mapped from IDPROMBASE for IDPROMSIZE bytes in the
! ASI_CTL address space for byte access only.
! 
	.global _getidprom
_getidprom:
	set     IDPROMBASE, %g1
	clr     %g2
1:
	lduba   [%g1 + %g2]ASI_CTL, %g7 ! get id prom byte
	add     %g2, 1, %g2		! interlock
	stb     %g7, [%o0]		! put it out
	cmp     %g2, %o1		! done yet?
	bne,a   1b
	add     %o0, 1, %o0		! delay slot
	retl				! leaf routine return
	nop				! delay slot
