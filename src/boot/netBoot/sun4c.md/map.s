!
!	.seg	"data"
!	.asciz	"@(#)map.s 1.5 88/02/08"
!	Copyright (c) 1986 by Sun Microsystems, Inc.
!

!
! Memory Mapping for Sun 4
!
! The following subroutines accept any address in the mappable range
! (256 megs).  They access the map for the current context register.  They
! assume that currently we are running in supervisor state.
! 
#define ASI_SM 3			
#define ASI_PM 4
#define SEGMENTADDRBITS 0xFFFC0000
	.seg	"text"

!
! Read the page map entry for the given address v
! and return it in a form suitable for software use.
!
! long
! getpgmap(v)
! caddr_t v;
!
	.global _getpgmap
_getpgmap:
	andn	%o0,0x3,%o1		! align to word boundary
	lda	[%o1]ASI_PM,%o0		! read page map entry
	retl
	nop				! nop hack needed for proto 1 XXXX

! 
! Set the pme for address v using the software pte given.
! 
! setpgmap(v, pte)
! caddr_t v;
! long pte;
! 
	.global _setpgmap
_setpgmap:
	andn	%o0,0x3,%o2		! align to word boundary
	retl
	sta	%o1,[%o2]ASI_PM		! write page map entry

!
! Set the segment map entry for segno to pm.
! 
! setsegmap(v, pm)
! u_int segno;
! u_short pm;
! 
	.global _setsegmap
_setsegmap:
	set	SEGMENTADDRBITS, %o2
	and	%o0, %o2, %o0		! get relevant segment address bits
	retl
	stha	%o1,[%o0]ASI_SM		! write segment entry
/*
 * Get the segment map entry for ther given virtual address
 *
 * getsegmap(v)
 * caddr_t vaddr;
 */
	.global _getsegmap
_getsegmap:
        set     SEGMENTADDRBITS, %o2
        and     %o0, %o2, %o0           ! get relevant segment address bits
        lduha   [%o0]ASI_SM,%o0         ! read segment entry
	retl
	nop
