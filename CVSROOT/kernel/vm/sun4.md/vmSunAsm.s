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
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE /* Write seg map entry */

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
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE /* Write seg map entry */

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
    lduha	[%o0] VMMACH_SEG_MAP_SPACE, %RETURN_VAL_REG	/* read it */

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
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE

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
    stha	%o0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE	/* Write PMEG */

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
    stha	%i0, [%OUT_TEMP1] VMMACH_SEG_MAP_SPACE	/* Write pmeg */

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
#ifdef NOTDEF
    set		VMMACH_SEG_MAP_MASK, %OUT_TEMP1
    and		%o0, %OUT_TEMP1,  %o0	/* mask out low bits of addr */
#endif NOTDEF
    stha	%o1, [%o0] VMMACH_SEG_MAP_SPACE		/* write value to map */

    retl	/* return from leaf routine */
    nop

/*
 * ----------------------------------------------------------------------------
 *
 * VmMachSegMapCopy --
 *
 *     	Copy the software segment map entries into the hardware segment entries.
 *	All segment table entries between address startAddr and address
 *	endAddr are copied.  It is assumed that the user context register is 
 *	set to the context for which the segment map entries are to be set.
 *	
 *	void VmMachSegMapCopy(tablePtr, startAddr, endAddr)
 *	    char *tablePtr;
 *	    int startAddr;
 *	    int endAddr;
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Hardware segment map entries for the current user context are set.
 *
 * ----------------------------------------------------------------------------
 */
.globl _VmMachSegMapCopy
_VmMachSegMapCopy:
						/* segTableAddr in %o0 */
						/* startAddr in %o1 */
						/* endAddr in %o2 */
    set		VMMACH_SEG_MAP_MASK, %OUT_TEMP1
    and		%o1, %OUT_TEMP1, %o1	/* mask out low bits */
copyLoop:
    lduh	[%o0], %OUT_TEMP1
    stha	%OUT_TEMP1, [%o1] VMMACH_SEG_MAP_SPACE
    add		%o0, 4, %o0		/* increment address to copy from */
    set		VMMACH_SEG_SIZE, %OUT_TEMP2
    add		%o1, %OUT_TEMP2, %o1	/* increment addr to copy to */
    cmp		%o2, %o1		/* Hit upper bound? */
    bg		copyLoop
    nop

    retl	/* return from leaf routine */
    nop

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
 *	NOTE: The trap handler assumes that this routine does not push anything
 *	      onto the stack.  It uses this fact to allow it to return to the
 *	      caller of this routine upon an address fault.  If you must push
 *	      something onto the stack then you had better go and modify 
 *	      "CallTrapHandler" in asmDefs.h appropriately.
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
    bcs		BadAddress		/* branch carry set? */
    nop
    sub		%o2, 1, %OUT_TEMP2
    addcc	%OUT_TEMP2, %o0, %OUT_TEMP2
    bcs		BadAddress
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
 *	Returns SUCCESS if were able to touch the page (which is almost
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
    sll		%o0, %OUT_TEMP1, %o0	/* o0 = o0 << VMMAACH_PAGE_SHIFT */
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
    retl
    nop

/*
 * The address marker below is there so that the trap handler knows the
 * end of code that may take a page fault while copying into/out of
 * user space.
 */

.globl	_VmMachCopyEnd
_VmMachCopyEnd:
