
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

    .data
    .asciz "$Header$ SPRITE (Berkeley)"
    .even
    .text


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
|* VmMachTracePMEG --
|*
|*	Read out all page table entries in the given pmeg, generate trace
|*	records for each with ref or mod bit set and then clear the ref
|*	and mod bits.
|*
|*	void VmMachTracePMEG(pmeg)
|*
|* Results:
|*      None.
|*
|* Side effects:
|*	The reference and modified bits are cleared for all pages in
|*	this PMEG.
|*
|* ----------------------------------------------------------------------------
|*

#define	PTE_MASK (VMMACH_RESIDENT_BIT + VMMACH_REFERENCED_BIT + VMMACH_MODIFIED_BIT)

    .globl	_VmMachTracePMEG
_VmMachTracePMEG:
    movl	sp@(4), d0			| d0 <= PMEG num
    moveml	#0x3020, sp@-			| Save d2, d3 and a2

    movl	_vmMachPMEGSegAddr, a2
    addl	#VMMACH_SEG_MAP_OFF, a2		| a2 <= Segment map address	
    movsb	d0,a2@				| Write PMEG num to segment map
| Subtract off seg map offset and add page map offset into a2.
    addl	#(VMMACH_PAGE_MAP_OFF - VMMACH_SEG_MAP_OFF), a2
    movl	#VMMACH_NUM_PAGES_PER_SEG_INT, d3	| d3 <= Pmegs per seg.
1$:
    movsl	a2@, d2				| Read out the pte

|*
|* Trace this page if it is resident and the reference and modify bits
|* are set.
|*

    movl	d2, d0
    andl	#PTE_MASK, d0
    beq		2$
    cmpl	#VMMACH_RESIDENT_BIT, d0
    beq		2$
    movl	d3, sp@-			| Push page num onto stack.
    movl	d2, sp@-			| Push pte onto stack.
						| Clear the ref and mod bits.
    andl	#(~(VMMACH_REFERENCED_BIT + VMMACH_MODIFIED_BIT)), d2
    movsl	d2, a2@
    jsr		_VmMachTracePage		| VmMachTrace(pte, pageNum)
    addql	#8, sp

2$:
|* 
|* Go to the next page.
|*
    addl	#VMMACH_PAGE_SIZE_INT, a2	| Go to next address.
    subql	#1, d3
    bgt		1$			

    moveml	sp@+, #0x040c			| Restore a2, d3 and d2

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


|*
|* ----------------------------------------------------------------------
|*
|* Vm_Copy{In,Out}
|*
|*	Copy numBytes from *sourcePtr in to *destPtr.
|*	This routine is optimized to do transfers when sourcePtr and 
|*	destPtr are both 4-byte aligned.
|*
|*	void
|*	Vm_Copy{In,Out}(numBytes, sourcePtr, destPtr)
|*	    register int numBytes;      /* The number of bytes to copy */
|*	    Address sourcePtr;          /* Where to copy from. */
|*	    Address destPtr;            /* Where to copy to. */
|*
|*	NOTE: The trap handler assumes that this routine does not push anything
|*	      onto the stack.  It uses this fact to allow it to return to the
|*	      caller of this routine upon an address fault.  If you must push
|*	      something onto the stack then you had better go and modify 
|*	      "CallTrapHandler" in asmDefs.h appropriately.
|*
|* Results:
|*	Returns SUCCESS if the copy went OK (which is almost always).  If
|*	a bus error (other than a page fault) occurred while reading or
|*	writing user memory, then SYS_ARG_NO_ACCESS is returned (this return
|*	occurs from the trap handler, rather than from this procedure).
|*
|* Side effects:
|*	The area that destPtr points to is modified.
|*
|* ----------------------------------------------------------------------
|*
    .globl _VmMachDoCopy
    .globl _Vm_CopyIn
_VmMachDoCopy:
_Vm_CopyIn:
    movl	sp@(4),d1			| Get number of bytes into a
						|     register.
    movl    	sp@(8),a0			| Get source and dest addresses
    movl    	sp@(12),a1			|     into a register.

|*
|* If the source or dest are not 2 byte aligned then everything must be
|* done as byte copies.
|*

gotArgs:
    movl    	a0,d0
    orl		sp@(12),d0
    andl	#1,d0
    jne		3$

|*
|* Do as many 64-byte copies as possible.
|*

1$:
    cmpl    	#64,d1
    jlt     	2$
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    subl   	#64,d1
    jra     	1$

|*
|* Copy up to 64 bytes of remainder, in 4-byte chunks.  Do this quickly
|* by dispatching into the middle of a sequence of move instructions.
|*

2$:
    movl	d1,d0
    andl	#3,d1
    subl	d1,d0
    asrl	#1,d0
    negl	d0
    jmp		pc@(34, d0:w)	
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+
    movl   	a0@+, a1@+

|*
|* Do one byte copies until done.
|*

3$:
    tstl    	d1
    jle     	4$
    movb   	a0@+,a1@+
    subql   	#1,d1
    jra     	3$

|* 
|* Return.
|*

4$: 
    clrl	d0
    rts

|* Vm_CopyOut is just like Vm_CopyIn except that it checks to make sure
|* that the destination is in the user area (otherwise this would be a
|* trap door to write to kernel space).

    .globl _Vm_CopyOut, _mach_FirstUserAddr, _mach_LastUserAddr
_Vm_CopyOut:
    movl	sp@(4),d1			| Get number of bytes into a
						|     register.
    movl    	sp@(8),a0			| Get source and dest addresses
    movl    	sp@(12),a1			|     into a register.
    cmpl	_mach_FirstUserAddr,a1
    jcs		5$
    movl	a1,d0
    subql	#1,d0
    addl	d1,d0
    jcs		5$
    cmpl	_mach_LastUserAddr,d0
    jls		gotArgs

|* User address out of range.  Check for a zero byte count before
|* returning an error, though;  there appear to be kernel routines
|* that call Vm_CopyOut with a zero count but bogus other arguments.

5$:
    tstl	d1
    jne		6$
    clrl	d0
    rts
6$:
    movl	#0x20000,d0
    rts

|*
|* ----------------------------------------------------------------------
|*
|* Vm_StringNCopy
|*
|*	Copy the NULL terminated string from *sourcePtr to *destPtr up
|*	numBytes worth of bytes.
|*
|*	void
|*	Vm_StringNCopy(numBytes, sourcePtr, destPtr, bytesCopiedPtr)
|*	    register int numBytes;      /* The number of bytes to copy */
|*	    Address sourcePtr;          /* Where to copy from. */
|*	    Address destPtr;            /* Where to copy to. */
|*	    int	*bytesCopiedPtr;	/* Number of bytes copied. */
|*
|*	NOTE: The trap handler assumes that this routine does not push anything
|*	      onto the stack.  It uses this fact to allow it to return to the
|*	      caller of this routine upon an address fault.  If you must push
|*	      something onto the stack then you had better go and modify 
|*	      "CallTrapHandler" in asmDefs.h appropriately.
|*
|* Results:
|*	Normally returns SUCCESS.  If a non-recoverable bus error occurs,
|*	then the trap handler fakes up a SYS_ARG_NO_ACCESS return from
|*	this procedure.
|*
|* Side effects:
|*	The area that destPtr points to is modified and *bytesCopiedPtr 
|*	contains the number of bytes copied.
|*
|* ----------------------------------------------------------------------
|*
    .globl  _Vm_StringNCopy
_Vm_StringNCopy:
    movl    	sp@(4),d1			| Get number of bytes into a
						|     register.
    movl    	sp@(8),a0			| Get source and dest addresses
    movl    	sp@(12),a1			|     into a register.

1$: 
    movb	a0@, a1@+			| Copy the character.
    cmpb	#0, a0@+			| See if hit null in string.
    beq		2$
    subl	#1, d1				| Decrement the byte counter.
    bne		1$				| Copy more chars if haven't
						|     reached the limit.
2$: 
    movl	sp@(4), d0			| Compute the number of bytes
    subl	d1, d0				|     copied and store the
    movl	sp@(16), a0			|     result.
    movl	d0, a0@
    clrl	d0				| Return SUCCESS.
    rts


|*
|* ----------------------------------------------------------------------
|*
|* Vm_TouchPages --
|*
|*	Touch the range of pages.
|*
|*	void
|*	Vm_TouchPages(firstPage, numPages)
|*	    int	firstPage;	/* First page to touch. */
|*	    int	numPages;	/* Number of pages to touch. */
|*
|*	NOTE: The trap handler assumes that this routine does not push anything
|*	      onto the stack.  It uses this fact to allow it to return to the
|*	      caller of this routine upon an address fault.  If you must push
|*	      something onto the stack then you had better go and modify 
|*	      "CallTrapHandler" in asmDefs.h appropriately.
|*
|* Results:
|*	Returns SUCCESS if were able to touch the page (which is almost
|*	always).  If a bus error (other than a page fault) occurred while 
|*	reading user memory, then SYS_ARG_NO_ACCESS is returned (this return
|*	occurs from the trap handler, rather than from this procedure).
|*
|* Side effects:
|*	None.
|*
|* ----------------------------------------------------------------------
|*
    .globl _Vm_TouchPages
_Vm_TouchPages:
    movl	sp@(4),d0	| d0 <= firstPage
    movl	#VMMACH_PAGE_SHIFT, d1
    asll	d1, d0		| d0 <= d0 << VMMACH_PAGE_SIZE
    movl	d0, a0		| a0 <= Starting address	
    movl	sp@(8), d0	| d0 <= numPages
1$:
    tstl	d0		| Quit when d0 == 0
    jeq		2$
    movl	a0@, d1		| Touch the page at the address in a0
    subql	#1, d0		| Go back around to touch the next page.
    addl	#VMMACH_PAGE_SIZE, a0
    jra		1$

2$:
    rts

|*
|* The address marker below is there so that the trap handler knows the
|* end of code that may take a page fault while copying into/out of
|* user space.

    .globl _VmMachCopyEnd
_VmMachCopyEnd:
