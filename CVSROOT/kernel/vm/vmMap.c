/* vmSunMap.c -
 *
 *     	This file contains routines to map pages into the kernel's address
 *	space.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vmSunConst.h"
#include "vm.h"
#include "vmInt.h"
#include "lock.h"
#include "list.h"
#include "machine.h"
#include "mem.h"
#include "proc.h"
#include "sched.h"
#include "sync.h"
#include "sys.h"

static	Sync_Condition	mappingCondition;


/*
 * ----------------------------------------------------------------------------
 *
 * VmMapPage --
 *
 *      Call internal routine to map the given page frame into the kernel's
 *	virtual address space.
 *
 * Results:
 *      The kernel virtual address where the page is mapped.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY Address
VmMapPage(pfNum)
    int		pfNum;	/* The page frame number that is being mapped. */
{
    Address	addr;

    LOCK_MONITOR;

    addr = VmMapPageInt(pfNum);

    UNLOCK_MONITOR;

    return(addr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmMapPageInt --
 *
 *      Map the given physical page into the kernels virtual address space.  
 *	The kernel virtual address where the page is mapped is returned.
 *	This routine is used when a page frame that is in a user's address
 *	space needs to be accessed by the kernel.
 *
 * Results:
 *      The kernel virtual address where the page is mapped.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL Address
VmMapPageInt(pfNum)
    int		pfNum;	/* The page frame number that is being mapped. */
{
    register	Vm_PTE	*ptePtr;
    VmVirtAddr		virtAddr;
    register	int	virtPage;
    register Vm_Segment	*segPtr;

    segPtr = vmSysSegPtr;

    /*
     * Search through the page table until a non-resident page is found or we
     * go past the pte that can be used for mapping.  If none can be found 
     * then sleep.
     */

    while (TRUE) {
	for (virtPage = VM_MAP_BASE_PAGE, 
	     ptePtr = VmGetPTEPtr(segPtr, VM_MAP_BASE_PAGE);
	     virtPage <= VM_MAP_END_PAGE && ptePtr->resident == 1;
	     virtPage++, VmIncPTEPtr(ptePtr, 1)) {
	}
	if (virtPage <= VM_MAP_END_PAGE) {
	    break;
	}

	vmStat.mapPageWait++;
	(void) Sync_Wait(&mappingCondition, FALSE);
    }

    /*
     * Initialize the pte and validate the page.
     */

    virtAddr.segPtr = segPtr;
    virtAddr.page = virtPage;
    virtAddr.offset = 0;
    ptePtr = VmGetPTEPtr(segPtr, virtPage);
    ptePtr->protection = VM_KRW_PROT;
    ptePtr->pfNum = VmVirtToPhysPage(pfNum);
    VmPageValidateInt(&virtAddr);

    return((Address) (virtAddr.page << VM_PAGE_SHIFT));
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmRemapPage --
 *
 *      Map the given physical page into the kernels virtual address space
 *	at the given virtual address.  The addres given must be produced from
 *	VmMapPage.  The purpose of this routine is to reduce overhead for
 *	routines that have to map numerous page frames into the kernel's
 *	virtual address space.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Kernel page table modified.
 *
 * ----------------------------------------------------------------------------
 */

void
VmRemapPage(addr, pfNum)
    Address	addr;	/* Virtual address to map the page frame at. */
    int		pfNum;	/* The page frame number that is being mapped. */
{
    register	Vm_PTE	*ptePtr;
    VmVirtAddr		virtAddr;
    register Vm_Segment	*segPtr;

    segPtr = vmSysSegPtr;
    virtAddr.segPtr = segPtr;
    virtAddr.page = (unsigned int) (addr) >> VM_PAGE_SHIFT;
    ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    ptePtr->pfNum = VmVirtToPhysPage(pfNum);
    VmPageValidate(&virtAddr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmUnmapPage --
 *
 *      Free up a page which has been mapped by VmMapPage.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
VmUnmapPage(mappedAddr)
    Address	mappedAddr;	/* Virtual address of the page that is being
				   unmapped. */
{
    LOCK_MONITOR;

    VmUnmapPageInt(mappedAddr);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmUnmapPageInt --
 *
 *      Free up a page which has been mapped by VmMapPage.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL void
VmUnmapPageInt(mappedAddr)
    Address	mappedAddr;	/* Virtual address of the page that is being
				   unmapped. */
{
    VmVirtAddr		virtAddr;

    virtAddr.segPtr = vmSysSegPtr;
    virtAddr.page = (unsigned int) (mappedAddr) >> VM_PAGE_SHIFT;
    virtAddr.offset = 0;

    VmPageInvalidateInt(&virtAddr);

    Sync_Broadcast(&mappingCondition);
}

static Vm_PTE   intelSavedPTE;          /* The page table entry that is stored
                                           at the address that the intel page
                                           has to overwrite. */
static int      intelPage;              /* The page frame that was allocated. */


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_MapIntelPage --
 *
 *      Allocate and validate a page for the Intel Ethernet chip.  This routine
 *	is required in order to initialize the chip.  The chip expects 
 *	certain stuff to be at a specific virtual address when it is 
 *	initialized.  This routine sets things up so that the expected
 *	virtual address is accessible.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The hardware segment table associated with the software segment
 *      is modified to validate the page.  Also the old pte stored at the
 *	virtual address and the page frame that is allocated are stored in
 *	static globals.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Vm_MapIntelPage(virtAddr) 
    int	virtAddr;	/* Virtual address where a page has to be validated
			   at. */
{
    Vm_PTE	pte;
    VmVirtAddr	virtAddrStruct;

#ifdef SUN3
    return;
#endif

    /*
     * Set up the page table entry.
     */

    pte = vm_ZeroPTE;
    pte.resident = 1;
    pte.protection = VM_KRW_PROT;
    virtAddrStruct.page = (unsigned) (virtAddr) >> VM_PAGE_SHIFT;
    virtAddrStruct.offset = 0;
    virtAddrStruct.segPtr = vmSysSegPtr;
    intelPage = VmPageAllocate(&virtAddrStruct, TRUE);
    pte.pfNum = VmVirtToPhysPage(intelPage);

    /*
     * It is known that there is already a pmeg for the virtual address that we
     * need to map.  Thus all that needs to be done is to store the pte.  
     * However since this virtual page is already in use for VME bus memory 
     * we need to save the pte that is already there before storing the 
     * new one.
     */

    VM_PTE_TO_INT(intelSavedPTE) = Vm_GetPageMap((Address) virtAddr);
    Vm_SetPageMap((Address) virtAddr, pte);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_UnmapIntelPage --
 *
 *      Deallocate and invalidate a page for the intel chip.  This is a special
 *	case routine that is only for the intel ethernet chip.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The hardware segment table associated with the segment
 *      is modified to invalidate the page.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Vm_UnmapIntelPage(virtAddr) 
    int	virtAddr;
{
#ifdef SUN3
    return;
#endif
    /*
     * Restore the saved pte and free the allocated page.
     */

    Vm_SetPageMap((Address) virtAddr, intelSavedPTE);

    (void) VmPageFree(intelPage);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_MakeAccessible --
 *
 *      Make sure that the given range of address are valid.
 *
 * Results:
 *	Return the address passed in in *retAddrPtr and the number of bytes
 *	that are actually valid in *retBytesPtr.
 *
 * Side effects:
 *      If the address that is being made accessible falls into a heap or
 *	stack segment then the heap segment is prevented from being
 *	expanded for the calling process.  This is to ensure that the addresses
 *	remain valid until Vm_MakeUnaccessible is called.
 *
 * ----------------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Vm_MakeAccessible(accessType, numBytes, startAddr, retBytesPtr, retAddrPtr)
    int			accessType;	/* One of VM_READONLY_ACCESS, 
					   VM_OVERWRITE_ACCESS, 
					   VM_READWRITE_ACCESS. */
    int			numBytes;	/* The maximum number of bytes to make 
					   accessible. */
    Address		startAddr;	/* The address in the user's address
					   space to start at. */
    register	int	*retBytesPtr;	/* The actual number of bytes 
					   made accessible. */
    register	Address	*retAddrPtr;	/* The kernel virtual address that
					   can be used to access the bytes. */
{
    register	Vm_Segment	*segPtr;
    register	Vm_PTE		*ptePtr;
    VmVirtAddr			virtAddr;
    int				firstPage;
    int				lastPage;
    Proc_ControlBlock		*procPtr;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());

    /*
     * Parse the virtual address to determine which segment that this page
     * falls into and which page in the segment.  If it is a heap segment
     * or stack segment, then the current process's heap segment will
     * be prevented from being expanded.
     */
    VmVirtAddrParse(procPtr, (int) startAddr, &virtAddr);
    
    segPtr = virtAddr.segPtr;

    /*
     * See if the beginning address falls into a segment.
     */

    if (segPtr == (Vm_Segment *) NIL) {
	*retBytesPtr = 0;
	*retAddrPtr = (Address) NIL;
	return;
    }

    *retBytesPtr = numBytes;
    *retAddrPtr = startAddr;

    firstPage = virtAddr.page;
    lastPage = ((unsigned int) (startAddr) + numBytes - 1) >> VM_PAGE_SHIFT;
    
    if (segPtr->type == VM_STACK) {
	/*
	 * If ending address goes past the end of the
	 * stack then truncate it.  
	 */
	if (lastPage > MACH_LAST_USER_STACK_PAGE) {
	    *retBytesPtr = MACH_MAX_USER_STACK_ADDR - (int) startAddr;
	}
	/* 
	 * Since is the stack segment we know the whole range
	 * of addresses is valid so just return.
	 */
	return;
    }

    /*
     * Truncate range of addresses so that it falls into the code or heap
     * heap segment.
     */

    if (lastPage - segPtr->offset + 1 > segPtr->numPages) {
	lastPage = segPtr->offset + segPtr->numPages - 1;
	*retBytesPtr = ((lastPage + 1) << VM_PAGE_SHIFT) - (int) startAddr;
    }

    if (segPtr->type == VM_CODE) {
	/* 
	 * Code segments are mapped contiguously so we know the whole range
	 * of pages is valid.
	 */
	return;
    }

    /*
     * We are left with a heap segment.  Go through the page table and make
     * sure all of the pages in the range are valid.  Stop as soon as hit an
     * invalid page.  We can look at the page table without fear because the
     * segment has been prevented from being expanded by VmVirtAddrParse.
     */

    for (ptePtr = &(segPtr->ptPtr[virtAddr.page - segPtr->offset]);
         virtAddr.page <= lastPage; 
	 virtAddr.page++, ptePtr++) {
	if (!ptePtr->validPage) {
	    break;
	}
    }

    /*
     * If we couldn't map anything then just return.
     */

    if (virtAddr.page == firstPage) {
        VmDecExpandCount(procPtr->vmPtr->segPtrArray[VM_HEAP]);
	*retBytesPtr = 0;
	*retAddrPtr = (Address) NIL;
	return;
    }

    /* 
     * If we couldn't make all of the requested pages accessible then return 
     * the number of bytes that we actually made accessible.
     */

    if (virtAddr.page <= lastPage) {
	*retBytesPtr = (virtAddr.page << VM_PAGE_SHIFT) - (int) startAddr;
    }
}


/*
 ----------------------------------------------------------------------
 *
 * Vm_MakeUnaccessible
 *
 *	Take the given kernel virtual address and make the range of pages
 *	that it addresses unaccessible.  All that has to be done is to
 *	make the heap segment for the calling process expandable if it was
 *	made unexpandable by Vm_MakeAccessible.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Heap segment made expandable if the address falls into a heap or
 *	stack segment.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ENTRY void
Vm_MakeUnaccessible(addr, numBytes)
    Address		addr;
    int			numBytes;
{
    Proc_ControlBlock  	*procPtr;
    register Vm_Segment	*segPtr;

    LOCK_MONITOR;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    segPtr = procPtr->vmPtr->segPtrArray[VM_HEAP];

    if (((unsigned int) (addr) >> VM_PAGE_SHIFT) >= segPtr->offset) {
	/*
	 * This address falls into a stack or heap segment.  The heap segment
	 * was prevented from being expanded by Vm_MakeAccessible so we have
	 * to let it be expanded now.
	 */
        segPtr->notExpandCount--;
        if (segPtr->notExpandCount < 0) {
            Sys_Panic(SYS_FATAL, "Vm_MakeUnaccessible: expand count < 0\n");
        }
        if (segPtr->notExpandCount == 0) {
            Sync_Broadcast(&vmSegExpandCondition);
        }
    }

    UNLOCK_MONITOR;
}
