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
.asciz "$Header$ SPRITE (Berkeley)"
.align	8
.seg	"text"

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachReadPTE --
 *
 *     	Map the given hardware pmeg into the kernel's address space and 
 *	return the pte at the corresponding address.  There is a reserved
 *	address in the kernel that is used to map this hardware pmeg.
 *
 *	VmMachPTE VmMachReadPTE(pmegNum, addr)
 *	    int		pmegNum;	The pmeg to read the PTE for.
 *	    Address	addr;		The virtual address to read the PTE for.
 *
 * Results:
 *     The value of the PTE.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachReadPTE
_VmMachReadPTE:
    /* 
     * Set the segment map entry.
     */
    set		_vmMachPTESegAddr, %OUT_TEMP1	/* Get access address */
    ld		[%OUT_TEMP1], %OUT_TEMP1
#ifdef sun4c
    stba	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE /* Write seg map entry */
#else
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE /* Write seg map entry */
#endif

    /*
     * Get the page map entry.
     */
    lda		[%o1] VMMACH_PAGE_MAP_SPACE, %RETURN_VAL_REG	/* Return it */

    retl	/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachWritePTE --
 *
 *     	Map the given hardware pmeg into the kernel's address space and 
 *	write the pte at the corresponding address.  There is a reserved
 *	address in the kernel that is used to map this hardware pmeg.
 *
 *	void VmMachWritePTE(pmegNum, addr, pte)
 *	    int	   	pmegNum;	The pmeg to write the PTE for.
 *	    Address	addr;		The address to write the PTE for.
 *	    VmMachPTE	pte;		The page table entry to write.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The hardware page table entry is set.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachWritePTE
_VmMachWritePTE:
    /* 
     * Set the segment map entry.
     */
    set		_vmMachPTESegAddr, %OUT_TEMP1	/* Get access address */
    ld		[%OUT_TEMP1], %OUT_TEMP1
#ifdef sun4c
    stba	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE /* Write seg map entry */
#else
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE /* Write seg map entry */
#endif

    /*
     * Set the page map entry.
     */
    /* place to write to */
    set		VMMACH_PAGE_MAP_MASK, %OUT_TEMP2
    and		%o1, %OUT_TEMP2, %o1	/* Mask out low bits */
    sta		%o2, [%o1] VMMACH_PAGE_MAP_SPACE

    retl	/* Return from leaf routine */
    nop


/*
 * ----------------------------------------------------------------------
 *
 * bcopy --
 *
 *	Copy numBytes from *sourcePtr in to *destPtr.
 *	This routine is optimized to do transfers when sourcePtr and 
 *	destPtr are both double-word aligned.
 *
 *	void
 *	bcopy(sourcePtr, destPtr, numBytes)
 *	    Address sourcePtr;          Where to copy from.
 *	    Address destPtr;            Where to copy to.
 *	    int numBytes;      The number of bytes to copy
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The area that destPtr points to is modified.
 *
 * ----------------------------------------------------------------------
 */
.globl	_bcopy
_bcopy:
						/* sourcePtr in o0 */
						/* destPtr in o1 */
						/* numBytes in o2 */
/*
 * If the source or dest are not double-word aligned then everything must be
 * done as word or byte copies.
 */
    or		%o0, %o1, %OUT_TEMP1
    andcc	%OUT_TEMP1, 7, %g0
    be		BDoubleWordCopy
    nop
    andcc	%OUT_TEMP1, 3, %g0
    be		BWordCopy
    nop
    ba		BByteCopyIt
    nop

    /*
     * Do as many 64-byte copies as possible.
     */

BDoubleWordCopy:
    cmp    	%o2, 64
    bl     	BFinishWord
    nop
    ldd		[%o0], %OUT_TEMP1	/* uses out_temp1 and out_temp2 */
    std		%OUT_TEMP1, [%o1]
    ldd		[%o0 + 8], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 8]
    ldd		[%o0 + 16], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 16]
    ldd		[%o0 + 24], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 24]
    ldd		[%o0 + 32], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 32]
    ldd		[%o0 + 40], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 40]
    ldd		[%o0 + 48], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 48]
    ldd		[%o0 + 56], %OUT_TEMP1
    std		%OUT_TEMP1, [%o1 + 56]
    
    sub   	%o2, 64, %o2
    add		%o0, 64, %o0
    add		%o1, 64, %o1
    ba     	BDoubleWordCopy
    nop
BWordCopy:
    cmp		%o2, 64
    bl		BFinishWord
    nop
    ld		[%o0], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1]
    ld		[%o0 + 4], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 4]
    ld		[%o0 + 8], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 8]
    ld		[%o0 + 12], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 12]
    ld		[%o0 + 16], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 16]
    ld		[%o0 + 20], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 20]
    ld		[%o0 + 24], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 24]
    ld		[%o0 + 28], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 28]
    ld		[%o0 + 32], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 32]
    ld		[%o0 + 36], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 36]
    ld		[%o0 + 40], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 40]
    ld		[%o0 + 44], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 44]
    ld		[%o0 + 48], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 48]
    ld		[%o0 + 52], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 52]
    ld		[%o0 + 56], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 56]
    ld		[%o0 + 60], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1 + 60]
    
    sub   	%o2, 64, %o2
    add		%o0, 64, %o0
    add		%o1, 64, %o1
    ba     	BWordCopy
    nop

    /*
     * Copy up to 64 bytes of remainder, in 4-byte chunks.  I SHOULD do this
     * quickly by dispatching into the middle of a sequence of move
     * instructions, but I don't yet.
     */

BFinishWord:
    cmp		%o2, 4
    bl		BByteCopyIt
    nop
    ld		[%o0], %OUT_TEMP1
    st		%OUT_TEMP1, [%o1]
    sub		%o2, 4, %o2
    add		%o0, 4, %o0
    add		%o1, 4, %o1
    ba		BFinishWord
    nop
    
    /*
     * Do one byte copies until done.
     */
BByteCopyIt:
    tst    	%o2
    ble     	BDoneCopying
    nop
    ldub	[%o0], %OUT_TEMP1
    stb		%OUT_TEMP1, [%o1]
    sub		%o2, 1, %o2
    add		%o0, 1, %o0
    add		%o1, 1, %o1
    ba     	BByteCopyIt
    nop

    /* 
     * Return.
     */

BDoneCopying: 
    retl		/* return from leaf routine */
    nop

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
 * VmMachPMEGZero --
 *
 *     	Set all of the page table entries in the pmeg to 0.  There is a special
 *	address in the kernel's address space (vmMachPMEGSegAddr) that is used
 *	to map the pmeg in so that it can be zeroed.
 *
 *	void VmMachPMEGZero(pmeg)
 *	    int pmeg;
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The given pmeg is zeroed.
 *
 * ----------------------------------------------------------------------------
 */

.globl	_VmMachPMEGZero
_VmMachPMEGZero:
    /* Write segment map entry */
    set		_vmMachPMEGSegAddr, %OUT_TEMP1
    ld		[%OUT_TEMP1], %OUT_TEMP1
#ifdef sun4c
    stba	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE
#else
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE
#endif

    /*
     * Now zero out all page table entries.  %OUT_TEMP1 is starting address
     * and %OUT_TEMP2 is ending address.
     */
    
    set		VMMACH_SEG_SIZE, %OUT_TEMP2
    add		%OUT_TEMP1, %OUT_TEMP2, %OUT_TEMP2

KeepZeroing:
    sta		%g0, [%OUT_TEMP1] VMMACH_PAGE_MAP_SPACE
    set		VMMACH_PAGE_SIZE_INT, %g1
    add		%OUT_TEMP1, %g1, %OUT_TEMP1
    cmp		%OUT_TEMP1, %OUT_TEMP2
#ifdef NOTDEF
    bl		KeepZeroing
#else
    bcs		KeepZeroing
#endif NOTDEF
    nop

    retl	/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachReadAndZeroPMEG --
 *
 *	Read out all page table entries in the given pmeg and then set each to
 *	zero. There is a special address in the kernel's address space 
 *	(vmMachPMEGSegAddr) that is used to access the PMEG.
 *
 *	void VmMachPMEGZero(pmeg, pteArray)
 *	    int 	pmeg;
 *	    VmMachPTE	pteArray[VMMACH_NUM_PAGES_PER_SEG];
 *
 * Results:
 *      None.
 *
 * Side effects:
 *     The given pmeg is zeroed and *pteArray is filled in with the contents
 *	of the PMEG before it is zeroed.
 *
 * ----------------------------------------------------------------------------
 */

.globl	_VmMachReadAndZeroPMEG
_VmMachReadAndZeroPMEG:
    /*
     * %OUT_TEMP1 is address.  %OUT_TEMP2 is a counter.
     */
    set		_vmMachPMEGSegAddr, %OUT_TEMP1
    ld		[%OUT_TEMP1], %OUT_TEMP1
#ifdef sun4c
    stba	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE	/* Write PMEG */
#else
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE	/* Write PMEG */
#endif

    set		VMMACH_NUM_PAGES_PER_SEG_INT, %OUT_TEMP2
KeepZeroing2:
    lda		[%OUT_TEMP1] VMMACH_PAGE_MAP_SPACE, %o0	/* Read out the pte */
    st		%o0, [%o1]				/* pte into array */
    add		%o1, 4, %o1			/* increment array */
    sta		%g0, [%OUT_TEMP1] VMMACH_PAGE_MAP_SPACE	/* Clear out the pte. */
    set		VMMACH_PAGE_SIZE_INT, %g1
    add		%OUT_TEMP1, %g1, %OUT_TEMP1	/* next addr */
    subcc	%OUT_TEMP2, 1, %OUT_TEMP2
    bg		KeepZeroing2
    nop

    retl	/* Return from leaf routine */			
    nop

#ifdef NOTDEF
/*
 * ----------------------------------------------------------------------------
 *
 * VmMachTracePMEG --
 *
 *	Read out all page table entries in the given pmeg, generate trace
 *	records for each with ref or mod bit set and then clear the ref
 *	and mod bits.
 *
 *	void VmMachTracePMEG(pmeg)
 *
 * Results:
 *      None.
 *
 * Side effects:
 *	The reference and modified bits are cleared for all pages in
 *	this PMEG.
 *
 * ----------------------------------------------------------------------------
 */

#define	PTE_MASK (VMMACH_RESIDENT_BIT + VMMACH_REFERENCED_BIT + VMMACH_MODIFIED_BIT)

.globl	_VmMachTracePMEG
_VmMachTracePMEG:
    /* Start prologue */
    set		(-MACH_SAVED_WINDOW_SIZE), %g1
    save	%sp, %g1, %sp
    /* end prologue */

    /* OUT_TEMP1 is addr */
    set		_vmMachPMEGSegAddr, %OUT_TEMP1
    ld		[%OUT_TEMP1], %OUT_TEMP1
#ifdef sun4c
    stba	%i0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE	/* Write pmeg */
#else
    stha	%i0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE	/* Write pmeg */
#endif

    /* o1 is counter = second param */
    set		VMMACH_NUM_PAGES_PER_SEG_INT, %o1
TryTrace:
    /* VOL_TEMP1 is pte read out = d2 */
    lda		[%OUT_TEMP1] VMMACH_PAGE_MAP_SPACE, %VOL_TEMP1	/* Read pte */

    /*
     * Trace this page if it is resident and the reference and modify bits
     * are set.
     */
    /* OUT_TEMP2 is temp = d0 */
    andcc	%VOL_TEMP1, PTE_MASK, %OUT_TEMP2
    be		SkipPage
    nop
    cmp		%OUT_TEMP2, VMMACH_RESIDENT_BIT		/* BUG!!!!!! */
    be		SkipPage
    nop
    mov		%VOL_TEMP1, %o0		/* pte in o0, pageNum in o1 */
					/* Clear the ref and mod bits. */
    and		%VOL_TEMP1, ~(VMMACH_REFERENCED_BIT + VMMACH_MODIFIED_BIT), %VOL_TEMP1
    sta		%VOL_TEMP1, [%OUT_TEMP1] VMMACH_PAGE_MAP_SPACE
    call	_VmMachTracePage, 2		/* VmMachTrace(pte, pageNum) */
    nop

SkipPage:
    /* 
     * Go to the next page.
     */
    add		%OUT_TEMP1, VMMACH_PAGE_SIZE_INT, %OUT_TEMP1	/* next addr */
    subcc	%o1, 1, %o1
    bg		TryTrace
    nop

    ret			
    restore
#endif NOTDEF

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

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachCopyUserSegMap --
 *
 *     	Copy the software segment map entries into the hardware segment entries.
 *	All segment table entries for user address space up to the bottom of
 *	the hole in the virtual address space are copied.
 *	It is assumed that the user context register is 
 *	set to the context for which the segment map entries are to be set.
 *	
 *	void VmMachCopyUserSegMap(tablePtr)
 *	    short *tablePtr;
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Hardware segment map entries for the current user context are set.
 *
 * ----------------------------------------------------------------------------
 */
.globl _VmMachCopyUserSegMap
_VmMachCopyUserSegMap:
    /*
     * Due to the hole in the address space, I must make sure that no
     * segment for an address in the hole gets anything written to it, since
     * this would overwrite the pmeg mapping for a valid address's segment.
     */

    /* Start prologue */
    set		(-MACH_SAVED_WINDOW_SIZE), %OUT_TEMP1
    save	%sp, %OUT_TEMP1, %sp
    /* end prologue */
						/* segTableAddr in %i0 */

    set		VMMACH_BOTTOM_OF_HOLE, %OUT_TEMP2	/* contains end addr */
    srl		%OUT_TEMP2, VMMACH_SEG_SHIFT, %OUT_TEMP1	/* num segs */

    /* panic if not divisable by 8, since we do 8 at a time in loop */
    andcc	%OUT_TEMP1, 7, %g0
    be		StartCopySetup
    nop
    /*	Call panic - what args? */
    clr		%o0
    call	_panic, 1
    nop

StartCopySetup:
    /* preload offsets - each another seg size away */
    set		VMMACH_SEG_SIZE, %i1
    add		%i1, %i1, %i2
    add		%i1, %i2, %i3
    add		%i1, %i3, %i4
    add		%i1, %i4, %i5
    add		%i1, %i5, %o1
    add		%i1, %o1, %o2
    clr		%o0
CopyLoop:
#ifdef sun4c
    lduh	[%i0], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 2], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %i1] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 4], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %i2] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 6], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %i3] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 8], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %i4] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 10], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %i5] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 12], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %o1] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 14], %OUT_TEMP1
    stba	%OUT_TEMP1, [%o0 + %o2] VMMACH_SEG_MAP_SPACE
#else
    lduh	[%i0], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 2], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %i1] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 4], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %i2] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 6], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %i3] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 8], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %i4] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 10], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %i5] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 12], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %o1] VMMACH_SEG_MAP_SPACE
    lduh	[%i0 + 14], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o0 + %o2] VMMACH_SEG_MAP_SPACE
#endif /* sun4c */

    set		(8 * VMMACH_SEG_SIZE), %OUT_TEMP1
    add		%o0, %OUT_TEMP1, %o0
    cmp		%o0, %OUT_TEMP2		/* compare against end addr */
    blu		CopyLoop
    add		%i0, 16, %i0		/* delay slot */

    ret
    restore

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachGetContextReg --
 *
 *     	Return the value of the context register.
 *
 *	int VmMachGetContextReg()
 *
 * Results:
 *     The value of context register.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

.globl	_VmMachGetContextReg
_VmMachGetContextReg:
					/* Move context reg into result reg  */
    set		VMMACH_CONTEXT_OFF, %RETURN_VAL_REG
    lduba	[%RETURN_VAL_REG] VMMACH_CONTROL_SPACE, %RETURN_VAL_REG

    retl		/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSetContextReg --
 *
 *     	Set the user and kernel context registers to the given value.
 *
 *	void VmMachSetContext(value)
 *	    int value;		Value to set register to
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachSetContextReg
_VmMachSetContextReg:

    set		VMMACH_CONTEXT_OFF, %OUT_TEMP1
    stba	%o0, [%OUT_TEMP1] VMMACH_CONTROL_SPACE

    retl		/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachGetUserContext --
 *
 *     	Return the value of the user context register.
 *
 *	int VmMachGetUserContext()
 *
 * Results:
 *     The value of user context register.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachGetUserContext
_VmMachGetUserContext:
    /* There is no separate user context register on the sun4. */
    set		VMMACH_CONTEXT_OFF, %RETURN_VAL_REG
    lduba	[%RETURN_VAL_REG] VMMACH_CONTROL_SPACE, %RETURN_VAL_REG
    
    retl			/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachGetKernelContext --
 *
 *     	Return the value of the kernel context register.
 *
 *	int VmMachGetKernelContext()
 *
 * Results:
 *     The value of kernel context register.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachGetKernelContext
_VmMachGetKernelContext:
    /* There is no separate kernel context register on the sun4. */
    set		VMMACH_CONTEXT_OFF, %RETURN_VAL_REG
    lduba	[%RETURN_VAL_REG] VMMACH_CONTROL_SPACE, %RETURN_VAL_REG
    retl
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSetUserContext --
 *
 *     	Set the user context register to the given value.
 *
 *	void VmMachSetUserContext(value)
 *	    int value;		 Value to set register to
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

.globl	_VmMachSetUserContext
_VmMachSetUserContext:
    /* There is no separate user context register on the sun4. */
    set		VMMACH_CONTEXT_OFF, %OUT_TEMP1
    stba	%o0, [%OUT_TEMP1] VMMACH_CONTROL_SPACE
    retl			/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSetKernelContext --
 *
 *     	Set the kernel context register to the given value.
 *
 *	void VmMachSetKernelContext(value)
 *	    int value;		Value to set register to
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The supervisor context is set.
 *
 * ----------------------------------------------------------------------------
 */

.globl	_VmMachSetKernelContext
_VmMachSetKernelContext:
    /* There is no separate kernel context register on the sun4. */
    set		VMMACH_CONTEXT_OFF, %OUT_TEMP1
    stba	%o0, [%OUT_TEMP1] VMMACH_CONTROL_SPACE
    retl			/* Return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachInitSystemEnableReg --
 *
 *     	Set the system enable register to turn on caching, etc.
 *
 *	void VmMachInitSystemEnableReg()
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Caching will be turned on, if it wasn't already.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachInitSystemEnableReg
_VmMachInitSystemEnableReg:
    set		VMMACH_SYSTEM_ENABLE_REG, %OUT_TEMP1
    lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %o0
#ifdef sun4c
    or		%o0, VMMACH_ENABLE_CACHE_BIT | VMMACH_ENABLE_DVMA_BIT, %o0
#else
    or          %o0, VMMACH_ENABLE_CACHE_BIT, %o0
#endif
    stba	%o0, [%OUT_TEMP1] VMMACH_CONTROL_SPACE
    retl			/* Return from leaf routine */
    nop

#ifndef sun4c
/* Not used in sun4c */
/*
 * ----------------------------------------------------------------------------
 *
 * VmMachInitAddrErrorControlReg --
 *
 *     	Set the addr error control register to enable asynchronous memory
 *	error reporting.
 *
 *	void VmMachInitAddrErrorControlReg()
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Asynchronous memory errors may now be reported.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachInitAddrErrorControlReg
_VmMachInitAddrErrorControlReg:
    set		VMMACH_ADDR_CONTROL_REG, %OUT_TEMP1
    ld		[%OUT_TEMP1], %OUT_TEMP2
    or		%OUT_TEMP2, VMMACH_ENABLE_MEM_ERROR_BIT, %OUT_TEMP2
    st		%OUT_TEMP2, [%OUT_TEMP1]
    retl
    nop
#endif



/*
 * ----------------------------------------------------------------------------
 *
 * VmMachClearCacheTags --
 *
 *     	Clear all tags in the cache.
 *
 *	void VmMachClearCacheTags()
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachClearCacheTags
_VmMachClearCacheTags:
    set		VMMACH_CACHE_TAGS_ADDR, %OUT_TEMP1
    set		VMMACH_NUM_CACHE_TAGS, %OUT_TEMP2
ClearTags:
    sta		%g0, [%OUT_TEMP1] VMMACH_CONTROL_SPACE		/* clear tag */
    subcc	%OUT_TEMP2, 1, %OUT_TEMP2			/* dec cntr */
    bne		ClearTags
    add		%OUT_TEMP1, VMMACH_CACHE_TAG_INCR, %OUT_TEMP1	/* delay slot */

    retl
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachFlushCurrentContext --
 *
 *     	Flush the current context from the cache.
 *
 *	void VmMachFlushCurrentContext()
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All data cached from the current context is flushed from the cache.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachFlushCurrentContext
_VmMachFlushCurrentContext:
    /* Start prologue */
    set		(-MACH_SAVED_WINDOW_SIZE), %OUT_TEMP1
    save	%sp, %OUT_TEMP1, %sp
    /* end prologue */

    /*
     * Spread the stores evenly through 16 chunks of the cache.  This helps
     * to avoid back-to-back writebacks.
     */
    set		(VMMACH_CACHE_SIZE / 16), %l0

    /* Start with last line in each of the 16 chunks.  We work backwards. */
    sub		%l0, VMMACH_CACHE_LINE_SIZE, %i0

    /* Preload a bunch of offsets so we can straight-line a lot of this. */
    add		%l0, %l0, %l1
    add		%l1, %l0, %l2
    add		%l2, %l0, %l3
    add		%l3, %l0, %l4
    add		%l4, %l0, %l5
    add		%l5, %l0, %l6
    add		%l6, %l0, %l7

    add		%l7, %l0, %o0
    add		%o0, %l0, %o1
    add		%o1, %l0, %o2
    add		%o2, %l0, %o3
    add		%o3, %l0, %o4
    add		%o4, %l0, %o5
    add		%o5, %l0, %i4

    sta		%g0, [%i0] VMMACH_FLUSH_CONTEXT_SPACE
FlushingContext:
    sta		%g0, [%i0 + %l0] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l1] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l2] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l3] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l4] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l5] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l6] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %l7] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %o0] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %o1] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %o2] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %o3] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %o4] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %o5] VMMACH_FLUSH_CONTEXT_SPACE
    sta		%g0, [%i0 + %i4] VMMACH_FLUSH_CONTEXT_SPACE

    subcc	%i0, VMMACH_CACHE_LINE_SIZE, %i0	/* decrement loop */
    bge,a	FlushingContext
							/* delay slot */
    sta		%g0, [%i0] VMMACH_FLUSH_CONTEXT_SPACE

    ret
    restore


/*
 * ----------------------------------------------------------------------------
 *
 * VmMachFlushSegment --
 *
 *     	Flush a segment from the cache.
 *
 *	void VmMachFlushSegment(segVirtAddr)
 *	Address	segVirtAddr;	(Address of segment)
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All data cached from the segment is flushed from the cache.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachFlushSegment
_VmMachFlushSegment:
    /* Start prologue */
    set		(-MACH_SAVED_WINDOW_SIZE), %OUT_TEMP1
    save	%sp, %OUT_TEMP1, %sp
    /* end prologue */

    set		VMMACH_SEG_MAP_MASK, %o0
    and		%i0, %o0, %i0				/* beginning of seg */
    
    set		VMMACH_NUM_CACHE_LINES / 16, %i1	/* num loops */

    /*
     * Spread the stores evenly through 16 chunks of the cache.  This helps
     * to avoid back-to-back writebacks.
     */
    set		(VMMACH_CACHE_SIZE / 16), %l0

    /* Start with last line in each of the 16 chunks.  We work backwards. */
    add		%i0, %l0, %i0
    sub		%i0, VMMACH_CACHE_LINE_SIZE, %i0

    /* Preload a bunch of offsets so we can straight-line a lot of this. */
    add		%l0, %l0, %l1
    add		%l1, %l0, %l2
    add		%l2, %l0, %l3
    add		%l3, %l0, %l4
    add		%l4, %l0, %l5
    add		%l5, %l0, %l6
    add		%l6, %l0, %l7

    add		%l7, %l0, %o0
    add		%o0, %l0, %o1
    add		%o1, %l0, %o2
    add		%o2, %l0, %o3
    add		%o3, %l0, %o4
    add		%o4, %l0, %o5
    add		%o5, %l0, %i4

FlushingSegment:
    sta		%g0, [%i0] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l0] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l1] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l2] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l3] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l4] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l5] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l6] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %l7] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %o0] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %o1] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %o2] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %o3] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %o4] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %o5] VMMACH_FLUSH_SEG_SPACE
    sta		%g0, [%i0 + %i4] VMMACH_FLUSH_SEG_SPACE

    subcc	%i1, 1, %i1				/* decrement loop */
    bne		FlushingSegment
							/* delay slot */
    sub		%i0, VMMACH_CACHE_LINE_SIZE, %i0

    ret
    restore

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachFlushPage --
 *
 *     	Flush a page from the cache.
 *
 *	void VmMachFlushPage(pageVirtAddr)
 *	Address	pageVirtAddr;	(Address of page)
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All data cached from the page is flushed from the cache.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachFlushPage
_VmMachFlushPage:
    /* Start prologue */
    set		(-MACH_SAVED_WINDOW_SIZE), %OUT_TEMP1
    save	%sp, %OUT_TEMP1, %sp
    /* end prologue */
    
    set		~VMMACH_OFFSET_MASK, %o0
    and		%i0, %o0, %i0			/* beginning of page */

						/* number of loops */
    set		(VMMACH_PAGE_SIZE_INT / VMMACH_CACHE_LINE_SIZE / 16), %i1

    /*
     * Spread the stores evenly through 16 chunks of the page flush area in the
     * cache.  This helps to avoid back-to-back writebacks.
     */
    set		(VMMACH_PAGE_SIZE_INT / 16), %l0

    /* Start with last line in each of the 16 chunks.  We work backwards. */
    add		%i0, %l0, %i0
    sub		%i0, VMMACH_CACHE_LINE_SIZE, %i0

    /* Preload a bunch of offsets so we can straight-line a lot of this. */
    add		%l0, (VMMACH_PAGE_SIZE_INT / 16), %l1
    add		%l1, (VMMACH_PAGE_SIZE_INT / 16), %l2
    add		%l2, (VMMACH_PAGE_SIZE_INT / 16), %l3
    add		%l3, (VMMACH_PAGE_SIZE_INT / 16), %l4
    add		%l4, (VMMACH_PAGE_SIZE_INT / 16), %l5
    add		%l5, (VMMACH_PAGE_SIZE_INT / 16), %l6
    add		%l6, (VMMACH_PAGE_SIZE_INT / 16), %l7

    add		%l7, (VMMACH_PAGE_SIZE_INT / 16), %o0
    add		%o0, (VMMACH_PAGE_SIZE_INT / 16), %o1
    add		%o1, (VMMACH_PAGE_SIZE_INT / 16), %o2
    add		%o2, (VMMACH_PAGE_SIZE_INT / 16), %o3
    add		%o3, (VMMACH_PAGE_SIZE_INT / 16), %o4
    add		%o4, (VMMACH_PAGE_SIZE_INT / 16), %o5
    add		%o5, (VMMACH_PAGE_SIZE_INT / 16), %i4

FlushingPage:
    sta		%g0, [%i0] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l0] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l1] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l2] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l3] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l4] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l5] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l6] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %l7] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %o0] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %o1] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %o2] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %o3] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %o4] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %o5] VMMACH_FLUSH_PAGE_SPACE
    sta		%g0, [%i0 + %i4] VMMACH_FLUSH_PAGE_SPACE

    subcc	%i1, 1, %i1				/* decrement loop */
    bne		FlushingPage
    sub		%i0, VMMACH_CACHE_LINE_SIZE, %i0	/* delay slot */
    ret
    restore

#ifdef NOTDEF
/*
 * ----------------------------------------------------------------------------
 *
 *  VmMachContextForCachedAddr --
 *
 *     	Read the context for which a virtual address is cached from out of
 *	the cache tags.
 *
 *	unsigned int VmMachContextForCachedAddr(virtualAddr)
 *	Address	virtualAddr;
 *
 * Results:
 *     The context.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_VmMachContextForCachedAddr
_VmMachContextForCachedAddr:
    set		VMMACH_CACHE_TAGS_ADDR, %OUT_TEMP1
    ldua	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %RETURN_VAL_REG
    /* context is in bits 22 through 29 */
/* How do I find correct cache block? */
    srl		%RETURN_VAL_REG, 22, %RETURN_VAL_REG
    retl
    nop
#endif NOTDEF



/*
 * ----------------------------------------------------------------------
 *
 * Vm_Copy{In,Out}
 *
 *	Copy numBytes from *sourcePtr in to *destPtr.
 *	This routine is optimized to do transfers when sourcePtr and 
 *	destPtr are both double-word aligned.
 *
 *	void
 *	Vm_Copy{In,Out}(numBytes, sourcePtr, destPtr)
 *	    register int numBytes;      The number of bytes to copy
 *	    Address sourcePtr;          Where to copy from.
 *	    Address destPtr;            Where to copy to.
 *
 * Results:
 *	Returns SUCCESS if the copy went OK (which is almost always).  If
 *	a bus error (other than a page fault) occurred while reading or
 *	writing user memory, then SYS_ARG_NO_ACCESS is returned (this return
 *	occurs from the trap handler, rather than from this procedure).
 *
 * Side effects:
 *	The area that destPtr points to is modified.
 *
 * ----------------------------------------------------------------------
 */
.globl _VmMachDoCopy
.globl _Vm_CopyIn
_VmMachDoCopy:
_Vm_CopyIn:
						/* numBytes in o0 */
						/* sourcePtr in o1 */
						/* destPtr in o2 */
/*
 * If the source or dest are not double-word aligned then everything must be
 * done as word or byte copies.
 */

GotArgs:
    or		%o1, %o2, %OUT_TEMP1
    andcc	%OUT_TEMP1, 7, %g0
    be		DoubleWordCopy
    nop
    andcc	%OUT_TEMP1, 3, %g0
    be		WordCopy
    nop
    ba		ByteCopyIt
    nop

    /*
     * Do as many 64-byte copies as possible.
     */

DoubleWordCopy:
    cmp    	%o0, 64
    bl     	FinishWord
    nop
    ldd		[%o1], %OUT_TEMP1	/* uses out_temp1 and out_temp2 */
    std		%OUT_TEMP1, [%o2]
    ldd		[%o1 + 8], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 8]
    ldd		[%o1 + 16], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 16]
    ldd		[%o1 + 24], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 24]
    ldd		[%o1 + 32], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 32]
    ldd		[%o1 + 40], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 40]
    ldd		[%o1 + 48], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 48]
    ldd		[%o1 + 56], %OUT_TEMP1
    std		%OUT_TEMP1, [%o2 + 56]
    
    sub   	%o0, 64, %o0
    add		%o1, 64, %o1
    add		%o2, 64, %o2
    ba     	DoubleWordCopy
    nop
WordCopy:
    cmp		%o0, 64
    bl		FinishWord
    nop
    ld		[%o1], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2]
    ld		[%o1 + 4], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 4]
    ld		[%o1 + 8], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 8]
    ld		[%o1 + 12], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 12]
    ld		[%o1 + 16], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 16]
    ld		[%o1 + 20], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 20]
    ld		[%o1 + 24], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 24]
    ld		[%o1 + 28], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 28]
    ld		[%o1 + 32], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 32]
    ld		[%o1 + 36], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 36]
    ld		[%o1 + 40], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 40]
    ld		[%o1 + 44], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 44]
    ld		[%o1 + 48], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 48]
    ld		[%o1 + 52], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 52]
    ld		[%o1 + 56], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 56]
    ld		[%o1 + 60], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2 + 60]
    
    sub   	%o0, 64, %o0
    add		%o1, 64, %o1
    add		%o2, 64, %o2
    ba     	WordCopy
    nop

    /*
     * Copy up to 64 bytes of remainder, in 4-byte chunks.  I SHOULD do this
     * quickly by dispatching into the middle of a sequence of move
     * instructions, but I don't yet.
     */

FinishWord:
    cmp		%o0, 4
    bl		ByteCopyIt
    nop
    ld		[%o1], %OUT_TEMP1
    st		%OUT_TEMP1, [%o2]
    sub		%o0, 4, %o0
    add		%o1, 4, %o1
    add		%o2, 4, %o2
    ba		FinishWord
    nop
    
    /*
     * Do one byte copies until done.
     */
ByteCopyIt:
    tst    	%o0
    ble     	DoneCopying
    nop
    ldub	[%o1], %OUT_TEMP1
    stb		%OUT_TEMP1, [%o2]
    sub		%o0, 1, %o0
    add		%o1, 1, %o1
    add		%o2, 1, %o2
    ba     	ByteCopyIt
    nop

    /* 
     * Return.
     */

DoneCopying: 
    clr		%o0
    retl		/* return from leaf routine */
    nop

/*
 * Vm_CopyOut is just like Vm_CopyIn except that it checks to make sure
 * that the destination is in the user area (otherwise this would be a
 * trap door to write to kernel space).
 */

.globl _Vm_CopyOut, _mach_FirstUserAddr, _mach_LastUserAddr
_Vm_CopyOut:
					    /* numBytes in o0 */
					    /* sourcePtr in o1 */
					    /* destPtr in o2 */
    set		_mach_FirstUserAddr, %OUT_TEMP1
    ld		[%OUT_TEMP1], %OUT_TEMP1	/* get 1st user addr */
    cmp		%o2, %OUT_TEMP1
    blu		BadAddress		/* branch carry set */
    nop
    sub		%o2, 1, %OUT_TEMP2
    addcc	%OUT_TEMP2, %o0, %OUT_TEMP2
    blu		BadAddress
    nop

    set		_mach_LastUserAddr, %OUT_TEMP1
    ld		[%OUT_TEMP1], %OUT_TEMP1
    cmp		%OUT_TEMP2, %OUT_TEMP1
    bleu	GotArgs
    nop

    /*
     * User address out of range.  Check for a zero byte count before
     * returning an error, though;  there appear to be kernel routines
     * that call Vm_CopyOut with a zero count but bogus other arguments.
     */

BadAddress:
    tst		%o0
    bne		BadAddressTruly
    clr		%RETURN_VAL_REG
    retl
    nop
BadAddressTruly:
				/* Mike, what is this magic number?! */
    set		0x20000, %RETURN_VAL_REG
    retl
    nop

/*
 * ----------------------------------------------------------------------
 *
 * Vm_StringNCopy
 *
 *	Copy the NULL terminated string from *sourcePtr to *destPtr up
 *	numBytes worth of bytes.
 *
 *	void
 *	Vm_StringNCopy(numBytes, sourcePtr, destPtr, bytesCopiedPtr)
 *	    register int numBytes;      The number of bytes to copy
 *	    Address sourcePtr;          Where to copy from.
 *	    Address destPtr;            Where to copy to.
 *	    int	*bytesCopiedPtr;	Number of bytes copied.
 *
 *	NOTE: The trap handler assumes that this routine does not push anything
 *	      onto the stack.  It uses this fact to allow it to return to the
 *	      caller of this routine upon an address fault.  If you must push
 *	      something onto the stack then you had better go and modify 
 *	      "CallTrapHandler" in asmDefs.h appropriately.
 *
 * Results:
 *	Normally returns SUCCESS.  If a non-recoverable bus error occurs,
 *	then the trap handler fakes up a SYS_ARG_NO_ACCESS return from
 *	this procedure.
 *
 * Side effects:
 *	The area that destPtr points to is modified and *bytesCopiedPtr 
 *	contains the number of bytes copied.  NOTE: this always copies
 *	at least one char, even if the numBytes param is zero!!!
 *
 * ----------------------------------------------------------------------
 */
.globl  _Vm_StringNCopy
_Vm_StringNCopy:
						/* numBytes in o0 */
						/* sourcePtr in o1 */
						/* destPtr in o2 */
						/* bytesCopiedPtr in o3 */

    mov		%o0, %OUT_TEMP2			/* save numBytes */
StartCopyingChars: 
    ldub	[%o1], %OUT_TEMP1		/* Copy the character */
    stb		%OUT_TEMP1, [%o2]
    add		%o1, 1, %o1			/* increment addresses */
    add		%o2, 1, %o2
    cmp		%OUT_TEMP1, 0			/* See if hit null in string. */
    be		NullChar
    nop

    subcc	%OUT_TEMP2, 1, %OUT_TEMP2	/* Decrement the byte counter */
    bne		StartCopyingChars		/* Copy more chars if haven't */
    nop 					/*     reached the limit. */
NullChar: 
    sub		%o0, %OUT_TEMP2, %OUT_TEMP2	/* Compute # of bytes copied */
    st		%OUT_TEMP2, [%o3] 		/* and store the result. */
    clr 	%RETURN_VAL_REG			/* Return SUCCESS. */
    retl
    nop


/*
 * ----------------------------------------------------------------------
 *
 * Vm_TouchPages --
 *
 *	Touch the range of pages.
 *
 *	void
 *	Vm_TouchPages(firstPage, numPages)
 *	    int	firstPage;	First page to touch.
 *	    int	numPages;	Number of pages to touch.
 *
 *	NOTE: The trap handler assumes that this routine does not push anything
 *	      onto the stack.  It uses this fact to allow it to return to the
 *	      caller of this routine upon an address fault.  If you must push
 *	      something onto the stack then you had better go and modify 
 *	      "CallTrapHandler" in asmDefs.h appropriately.
 *
 * Results:
 *	Returns SUCCESS if we're able to touch the page (which is almost
 *	always).  If a bus error (other than a page fault) occurred while 
 *	reading user memory, then SYS_ARG_NO_ACCESS is returned (this return
 *	occurs from the trap handler, rather than from this procedure).
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl _Vm_TouchPages
_Vm_TouchPages:
				/* o0 = first page = starting addr */
				/* o1 = numPages */
    set		VMMACH_PAGE_SHIFT, %OUT_TEMP1
    sll		%o0, %OUT_TEMP1, %o0	/* o0 = o0 << VMMACH_PAGE_SHIFT */
    /* Mike had arithmetic shift here, why??? */
StartTouchingPages:
    tst		%o1		/* Quit when %o1 == 0 */
    be		DoneTouchingPages
    nop
    ld		[%o0], %OUT_TEMP1	/* Touch page at addr in %o0 */
    sub		%o1, 1, %o1	/* Go back around to touch the next page. */
    set		VMMACH_PAGE_SIZE, %OUT_TEMP2
    add		%o0, %OUT_TEMP2, %o0
    ba		StartTouchingPages
    nop

DoneTouchingPages:
    clr		%RETURN_VAL_REG		/* return success */
    retl
    nop

/*
 * The address marker below is there so that the trap handler knows the
 * end of code that may take a page fault while copying into/out of
 * user space.
 */

.globl	_VmMachCopyEnd
_VmMachCopyEnd:
