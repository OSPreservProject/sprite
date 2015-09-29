
|* vmSun.s -
|*
|*	Subroutines to access Sun virtual memory mapping hardware.
|*	All of the routines in here assume that source and destination
|*	function codes are set to MMU space.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

#include "vmSunConst.h"
#include "machAsmDefs.h"

    .text
#ifdef NOTDEF

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachReadPTE --
|*
|*     	Map the given hardware pmeg into the kernel's address space and 
|*	return the pte at the corresponding address.  There is a reserved
|*	address in the kernel that is used to map this hardware pmeg.
|*
|*	VmMachPTE VmMachReadPTE(pmegNum, addr)
|*	    int		pmegNum;/* The pmeg to read the PTE for. */
|*	    Address	addr;	/* The virtual address to read the PTE for. */
|*
|* Results:
|*     The value of the PTE.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachReadPTE
_VmMachReadPTE:
|* 
|* Set the segment map entry.
|*
    movl	_vmMachPTESegAddr,a0	| Get access address
    addl	#VMMACH_SEG_MAP_OFF,a0	| Bump to segment map offset
    movl	sp@(4),d0		| Get segment map entry to write.
    movsb	d0,a0@			| Write segment map entry

|*
|* Get the page map entry.
|*
    movl	sp@(8),d0		| Get virtual address
    andw	#VMMACH_PAGE_MAP_MASK,d0| Mask out low bits
    addl	#VMMACH_PAGE_MAP_OFF,d0	| Add in page map offset.
    movl	d0, a0
    movsl	a0@,d0			| d0 <= page map entry

    rts					| Return


|*
|* ----------------------------------------------------------------------------
|*
|* VmMachWritePTE --
|*
|*     	Map the given hardware pmeg into the kernel's address space and 
|*	write the pte at the corresponding address.  There is a reserved
|*	address in the kernel that is used to map this hardware pmeg.
|*
|*	void VmMachWritePTE(pmegNum, addr, pte)
|*	    int	   	pmegNum;	/* The pmeg to write the PTE for. */
|*	    Address	addr;		/* The address to write the PTE for. */
|*	    VmMachPTE	pte;		/* The page table entry to write. */
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     The hardware page table entry is set.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachWritePTE
_VmMachWritePTE:
|* 
|* Set the segment map entry.
|*
    movl	_vmMachPTESegAddr,a0	| Get access address
    addl	#VMMACH_SEG_MAP_OFF,a0	| Bump to segment map offset
    movl	sp@(4),d0		| Get segment map entry to write.
    movsb	d0,a0@			| Write segment map entry

|*
|* Set the page map entry.
|*
    movl	sp@(8),d0		| Get virtual address into a register
    andw	#VMMACH_PAGE_MAP_MASK,d0	| Mask out low bits
    addl	#VMMACH_PAGE_MAP_OFF,d0	| Add in page map offset.
    movl	d0, a0
    movl	sp@(12),d0		| Get page map entry into a register
    movsl	d0,a0@			| Write page map entry

    rts					| Return
#endif
|*
|* ----------------------------------------------------------------------------
|*
|* VmMachGetPageMap --
|*
|*     	Return the page map entry for the given virtual address.
|*	It is assumed that the user context register is set to the context
|*	for which the page map entry is to retrieved.
|*
|*	int Vm_GetPageMap(virtualAddress)
|*	    Address virtualAddress;
|*
|* Results:
|*     The contents of the hardware page map entry.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachGetPageMap
_VmMachGetPageMap:
    movl	sp@(4),d0		| Get virtual address into a register
    andw	#VMMACH_PAGE_MAP_MASK,d0| Get relevant bits from address.
    addl	#VMMACH_PAGE_MAP_OFF,d0	| Add in page map offset.
    movl	d0, a0
    movsl	a0@,d0			| Read page map entry

    rts					| Return
#ifdef notdef
|*
|* ----------------------------------------------------------------------------
|*
|* VmMachGetSegMap --
|*
|*     	Return the segment map entry for the given virtual address.
|*	It is assumed that the user context register is set to the context
|*	for which the segment map entry is to retrieved.
|*
|*	int VmMachGetSegMap(virtualAddress)
|*	    Address virtualAddress;
|*
|* Results:
|*     The contents of the segment map entry.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachGetSegMap
_VmMachGetSegMap:
    movl	sp@(4),d0		| Get virtual address in a register.
    andw	#VMMACH_SEG_MAP_MASK,d0	| Get relevant bits.
    addl	#VMMACH_SEG_MAP_OFF,d0	| Add in segment map offset
    movl	d0, a0
    clrl	d0			| Clear the return register.
    movsb	a0@,d0			| Read segment map entry into return
					| register.

    rts					| Return
#endif
|*
|* ----------------------------------------------------------------------------
|*
|* VmMachSetPageMap --
|*
|*     	Set the page map entry for the given virtual address to the pte valud 
|*      given in pte.  It is assumed that the user context register is 
|*	set to the context for which the page map entry is to be set.
|*
|*	void VmMachSetPageMap(virtualAddress, pte)
|*	    Address 	virtualAddress;
|*	    VmMachPTE	pte;
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     The hardware page map entry is set.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachSetPageMap
_VmMachSetPageMap:
    movl	sp@(4),d0		| Get virtual address into a register
    andw	#VMMACH_PAGE_MAP_MASK,d0| Mask out low bits
    addl	#VMMACH_PAGE_MAP_OFF,d0	| Add in page map offset.
    movl	d0, a0
    movl	sp@(8),d0		| Get page map entry into a register
    movsl	d0,a0@			| Write page map entry

    rts					| Return
#ifdef NOTDEF
|*
|* ----------------------------------------------------------------------------
|*
|* VmMachPMEGZero --
|*
|*     	Set all of the page table entries in the pmeg to 0.  There is a special
|*	address in the kernel's address space (vmMachPTESegAddr) that is used to
|*	map the pmeg in so that it can be zeroed.
|*
|*	void VmMachPMEGZero(pmeg)
|*	    int pmeg;
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     The given pmeg is zeroed.
|*
|* ----------------------------------------------------------------------------
|*

    .globl	_VmMachPMEGZero
_VmMachPMEGZero:
| Write segment map entry
    movl	_vmMachPMEGSegAddr, d1	
    movl	d1, a0			
    addl	#VMMACH_SEG_MAP_OFF, a0	| a0 <= Segment map address	
    movl	sp@(4),d0		| d0 <= PMEG num
    movsb	d0,a0@			| Write PMEG num to segment map

| Now zero out all page table entries.

    movl	d1, a0			| a0 <= Starting address
    addl	#VMMACH_PAGE_MAP_OFF, a0
					| d1 <= Ending address
    addl	#(VMMACH_SEG_SIZE + VMMACH_PAGE_MAP_OFF), d1	
    clrl	d0			| Clear out d0.
1$:
    movsl	d0,a0@			| Write page map entry
    addl	#VMMACH_PAGE_SIZE_INT, a0 | Go to next address.
    cmpl	a0, d1			| See if have initialized all 
    bgt		1$			|     ptes.

    rts					| Return

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachReadAndZeroPMEG --
|*
|*	Read out all page table entries in the given pmeg and then set each to
|*	zero. There is a special address in the kernel's address space 
|*	(vmMachPTESegAddr) that is used to access the PMEG.
|*
|*	void VmMachPMEGZero(pmeg, pteArray)
|*	    int 	pmeg;
|*	    VmMachPTE	pteArray[VMMACH_NUM_PAGES_PER_SEG];
|*
|* Results:
|*      None.
|*
|* Side effects:
|*     The given pmeg is zeroed and *pteArray is filled in with the contents
|*	of the PMEG before it is zeroed.
|*
|* ----------------------------------------------------------------------------
|*

    .globl	_VmMachReadAndZeroPMEG
_VmMachReadAndZeroPMEG:
    movl	_vmMachPMEGSegAddr, a0
    addl	#VMMACH_SEG_MAP_OFF, a0		| a0 <= Segment map address	
    movl	sp@(4),d0			| d0 <= PMEG num
    movsb	d0,a0@				| Write PMEG num to segment map
    movl	sp@(8), a1			| a1 <= pteArray
| Subtract off seg map offset and add page map offset into a0.
    addl	#(VMMACH_PAGE_MAP_OFF - VMMACH_SEG_MAP_OFF), a0
    movl	#VMMACH_NUM_PAGES_PER_SEG_INT, d1	| d1 <= Pmegs per seg.
1$:
    movsl	a0@, d0				| Read out the pte
    movl	d0, a1@+			| a1 <= the pte
    clrl	d0
    movsl	d0, a0@				| Clear out the pte.
    addl	#VMMACH_PAGE_SIZE_INT, a0	| Go to next address.
    subql	#1, d1
    bgt		1$			

    rts			


|*
|* ----------------------------------------------------------------------------
|*
|* VmMachSetSegMap --
|*
|*     	Set the segment map entry for the given virtual address to the given 
|*	value.  It is assumed that the user context register is set to the 
|*	context for which the segment map entry is to be set.
|*
|*	void VmMachSetSegMap(virtualAddress, value)
|*	    Address	virtualAddress;
|*	    int		value;
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     Hardware segment map entry for the current user context is set.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachSetSegMap
_VmMachSetSegMap:
    movl	sp@(4),d0		| Get access address
    andw	#VMMACH_SEG_MAP_MASK,d0	| Mask out low bits
    addl	#VMMACH_SEG_MAP_OFF,d0	| Bump to segment map offset
    movl	d0, a0
    movl	sp@(8),d0		| Get segment map entry to write in a 
				        | register.
    movsb	d0,a0@			| Write segment map entry

    rts				        | return

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachSegMapCopy --
|*
|*     	Copy the software segment map entries into the hardware segment entries.
|*	All segment table entries between address startAddr and address
|*	endAddr are copied.  It is assumed that the user context register is 
|*	set to the context for which the segment map entries are to be set.
|*	
|*	void VmMachSegMapCopy(tablePtr, startAddr, endAddr)
|*	    char *tablePtr;
|*	    int startAddr;
|*	    int endAddr;
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     Hardware segment map entries for the current user context are set.
|*
|* ----------------------------------------------------------------------------
|*
    .globl _VmMachSegMapCopy
_VmMachSegMapCopy:
    movl	sp@(4),a0		| Get segment table address
    movl	sp@(8),d1		| Get start address in a register.
    andw	#VMMACH_SEG_MAP_MASK,d1	| Mask out low bits
    addl	#VMMACH_SEG_MAP_OFF,d1	| Bump to segment map offset
    movl	d1, a1
    movl	sp@(12),d1		| Get end address in a register.
    addl	#VMMACH_SEG_MAP_OFF, d1	| Add in offset.
1$:
    movb	a0@,d0			| Get segment map entry to write.
    movsb	d0,a1@			| Write segment map entry
    addql	#1, a0			| Increment the address to copy from.
    addl	#VMMACH_SEG_SIZE, a1	| Increment the address to copy to.
    cmpl	a1, d1			| See if hit upper bound.  If not 	
    bgt		1$			| continue.

    rts
/*
 * Sun 2's require that the context offset be treated as a word and on Sun-3's
 * it can't be.
 */

#ifdef sun3
#define	VMMACH_UC_OFF VMMACH_CONTEXT_OFF
#define	VMMACH_KC_OFF VMMACH_CONTEXT_OFF
#else
#define	VMMACH_UC_OFF VMMACH_USER_CONTEXT_OFF:w
#define	VMMACH_KC_OFF VMMACH_KERN_CONTEXT_OFF:w
#endif

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachGetContextReg --
|*
|*     	Return the value of the context register (on a Sun-2 the user context
|*	register).
|*
|*	int VmMachGetContextReg()
|*
|* Results:
|*     The value of context register.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*

    .globl	_VmMachGetContextReg
_VmMachGetContextReg:
						| Move context reg into result 
					    	| register.
#ifdef sun3
    movsb	VMMACH_CONTEXT_OFF,d0 		
#else
    movsb	VMMACH_USER_CONTEXT_OFF:w,d0
#endif
    andb	#VMMACH_CONTEXT_MASK,d0		| Clear high-order bits

    rts						| Return

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachSetContextReg --
|*
|*     	Set the user and kernel context registers to the given value.
|*
|*	void VmMachSetContext(value)
|*	    int value;		/* Value to set register to */
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachSetContextReg
_VmMachSetContextReg:

    movl	sp@(4),d0			| Get context value to set 
					    	| into a 
					    	| register

						| Set context register(s).
#ifdef sun3
    movsb	d0, VMMACH_CONTEXT_OFF
#else 
    movsb	d0,VMMACH_USER_CONTEXT_OFF:w 
    movsb	d0,VMMACH_KERN_CONTEXT_OFF:w
#endif

    rts						| Return

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachGetUserContext --
|*
|*     	Return the value of the user context register.
|*
|*	int VmMachGetUserContext()
|*
|* Results:
|*     The value of user context register.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachGetUserContext
_VmMachGetUserContext:
    movsb	VMMACH_UC_OFF,d0 		| Get context reg value
    andb	#VMMACH_CONTEXT_MASK,d0		| Clear high-order bits
    rts						| Return

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachGetKernelContext --
|*
|*     	Return the value of the kernel context register.
|*
|*	int VmMachGetKernelContext()
|*
|* Results:
|*     The value of kernel context register.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
    .globl	_VmMachGetKernelContext
_VmMachGetKernelContext:
    movsb	VMMACH_KC_OFF,d0 		| Get context reg value.
    andb	#VMMACH_CONTEXT_MASK,d0		| Clear high-order bits
    rts

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachSetUserContext --
|*
|*     	Set the user context register to the given value.
|*
|*	void VmMachSetUserContext(value)
|*	    int value;		/* Value to set register to */
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*

    .globl	_VmMachSetUserContext
_VmMachSetUserContext:
    movl	sp@(4),d0			| Get context value to set.
    movsb	d0,VMMACH_UC_OFF 		| Set context register.
    rts						| Return

|*
|* ----------------------------------------------------------------------------
|*
|* VmMachSetKernelContext --
|*
|*     	Set the kernel context register to the given value.
|*
|*	void VmMachSetKernelContext(value)
|*	    int value;		/* Value to set register to */
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     The supervisor context is set.
|*
|* ----------------------------------------------------------------------------
|*

    .globl	_VmMachSetKernelContext
_VmMachSetKernelContext:
    movl	sp@(4),d0			| Get context value to set
    movsb	d0,VMMACH_KC_OFF 		| Set context register
    rts 					| Return
#endif
