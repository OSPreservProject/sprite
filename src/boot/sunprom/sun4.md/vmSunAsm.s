/*
 * vmSun.s -
 *
 *	Subroutines to access Sun virtual memory mapping hardware.
 *	All of the routines in here assume that source and destination
 *	function codes are set to MMU space.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#include "vmSunConst.h"
#include "machAsmDefs.h"

.seg	"data"
.asciz "$Header: /sprite/src/kernel/vm/sun4.md/RCS/vmSunAsm.s,v 9.3 89/10/11 17:37:01 mgbaker Exp Locker: mgbaker $ SPRITE (Berkeley)"
.align	8
.seg	"text"

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachGetPageMap --
 *
 *     	Return the page map entry for the given virtual address.
 *	It is assumed that the user context register is set to the context
 *	for which the page map entry is to retrieved.
 *
 *	int Vm_GetPageMap(virtualAddress)
 *	    Address virtualAddress;
 *
 * Results:
 *     The contents of the hardware page map entry.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachGetPageMap
_VmMachGetPageMap:
    set		VMMACH_PAGE_MAP_MASK, %OUT_TEMP1
    and		%o0, %OUT_TEMP1, %o0	/* relevant bits from addr */
    lda		[%o0] VMMACH_PAGE_MAP_SPACE, %RETURN_VAL_REG	/* read it */

    retl					/* Return */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachGetSegMap --
 *
 *     	Return the segment map entry for the given virtual address.
 *	It is assumed that the user context register is set to the context
 *	for which the segment map entry is to retrieved.
 *
 *	int VmMachGetSegMap(virtualAddress)
 *	    Address virtualAddress;
 *
 * Results:
 *     The contents of the segment map entry.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachGetSegMap
_VmMachGetSegMap:
    set		VMMACH_SEG_MAP_MASK, %OUT_TEMP1
    and		%o0, %OUT_TEMP1, %o0	/* Get relevant bits. */
/* Is this necessary? */
#ifdef sun4c
    lduba	[%o0] VMMACH_SEG_MAP_SPACE, %RETURN_VAL_REG	/* read it */
#else
    lduha	[%o0] VMMACH_SEG_MAP_SPACE, %RETURN_VAL_REG	/* read it */
#endif

    retl		/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSetPageMap --
 *
 *     	Set the page map entry for the given virtual address to the pte valud 
 *      given in pte.  It is assumed that the user context register is 
 *	set to the context for which the page map entry is to be set.
 *
 *	void VmMachSetPageMap(virtualAddress, pte)
 *	    Address 	virtualAddress;
 *	    VmMachPTE	pte;
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The hardware page map entry is set.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachSetPageMap
_VmMachSetPageMap:
    set		VMMACH_PAGE_MAP_MASK, %OUT_TEMP1
    and		%o0, %OUT_TEMP1, %o0	/* Mask out low bits */
    sta		%o1, [%o0] VMMACH_PAGE_MAP_SPACE	/* write map entry */

    retl		/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSetSegMap --
 *
 *     	Set the segment map entry for the given virtual address to the given 
 *	value.  It is assumed that the user context register is set to the 
 *	context for which the segment map entry is to be set.
 *
 *	void VmMachSetSegMap(virtualAddress, value)
 *	    Address	virtualAddress;
 *	    int		value;
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Hardware segment map entry for the current user context is set.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachSetSegMap
_VmMachSetSegMap:
#ifdef sun4c
    stba	%o1, [%o0] VMMACH_SEG_MAP_SPACE		/* write value to map */
#else
    stha	%o1, [%o0] VMMACH_SEG_MAP_SPACE		/* write value to map */
#endif

    retl	/* return from leaf routine */
    nop

