/* vmMap.c -
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
#include "vmMach.h"
#include "vm.h"
#include "vmInt.h"
#include "lock.h"
#include "list.h"
#include "mem.h"
#include "proc.h"
#include "sched.h"
#include "sync.h"
#include "sys.h"

static	Sync_Condition	mappingCondition;

int	vmNumMappedPages = 16;
int	vmMapBasePage;
int	vmMapEndPage;
Address vmMapBaseAddr;
Address vmMapEndAddr;


/*
 * ----------------------------------------------------------------------------
 *
 * VmMapPage --
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
 *      Kernel page table modified to validate the mapped page.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY Address
VmMapPage(pfNum)
    unsigned int	pfNum;	/* The page frame number that to map. */
{
    register Vm_PTE	*ptePtr;
    Vm_VirtAddr		virtAddr;
    register int	virtPage;
    register Vm_Segment	*segPtr;

    LOCK_MONITOR;

    segPtr = vm_SysSegPtr;
    /*
     * Search through the page table until a non-resident page is found or we
     * go past the pte that can be used for mapping.  If none can be found 
     * then sleep.
     */
    while (TRUE) {
	for (virtPage = vmMapBasePage,
		 ptePtr = VmGetPTEPtr(segPtr, vmMapBasePage);
	     virtPage < vmMapEndPage;
	     virtPage++, VmIncPTEPtr(ptePtr, 1)) {
	    if (!(*ptePtr & VM_PHYS_RES_BIT)) {
		virtAddr.segPtr = segPtr;
		virtAddr.page = virtPage;
		virtAddr.offset = 0;
		*ptePtr |= VM_PHYS_RES_BIT | pfNum;
		VmMach_PageValidate(&virtAddr, *ptePtr);
		UNLOCK_MONITOR;
		return((Address) (virtPage << vmPageShift));
	    }
	}
	vmStat.mapPageWait++;
	(void) Sync_Wait(&mappingCondition, FALSE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmRemapPage --
 *
 *      Map the given physical page into the kernels virtual address space
 *	at the given virtual address.  The address given must be produced from
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
ENTRY void
VmRemapPage(addr, pfNum)
    Address		addr;	/* Virtual address to map the page frame at. */
    unsigned int	pfNum;	/* The page frame number to map. */
{
    register	Vm_PTE	*ptePtr;
    Vm_VirtAddr		virtAddr;
    register Vm_Segment	*segPtr;

   
    LOCK_MONITOR;

    segPtr = vm_SysSegPtr;
    virtAddr.segPtr = segPtr;
    virtAddr.page = (unsigned int) (addr) >> vmPageShift;
    /*
     * Clean the old page from the cache.
     */
    VmMach_FlushPage(&virtAddr);
    ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    *ptePtr &= ~VM_PAGE_FRAME_FIELD;
    *ptePtr |= pfNum;
    VmMach_PageValidate(&virtAddr, *ptePtr);

    UNLOCK_MONITOR;
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
 *      Kernel page table modified to invalidate the page.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmUnmapPage(mappedAddr)
    Address	mappedAddr;	/* Virtual address of the page that is being
				   unmapped. */
{
    Vm_VirtAddr		virtAddr;
    Vm_PTE		*ptePtr;

    LOCK_MONITOR;

    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = (unsigned int) (mappedAddr) >> vmPageShift;
    virtAddr.offset = 0;

    ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);
    *ptePtr &= ~(VM_PHYS_RES_BIT | VM_PAGE_FRAME_FIELD);
    VmMach_PageInvalidate(&virtAddr, Vm_GetPageFrame(*ptePtr), FALSE);

    Sync_Broadcast(&mappingCondition);

    UNLOCK_MONITOR;
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
 *	stack segment then the heap segment for the currently executing
 *	process has the page table in-use count incremented.  This is to
 *	ensure that the addresses remain valid until Vm_MakeUnaccessible
 *	is called.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Vm_MakeAccessible(accessType, numBytes, startAddr, retBytesPtr, retAddrPtr)
    int			accessType;	/* One of VM_READONLY_ACCESS, 
					 * VM_OVERWRITE_ACCESS, 
					 * VM_READWRITE_ACCESS. */
    int			numBytes;	/* The maximum number of bytes to make 
					 * accessible. */
    Address		startAddr;	/* The address in the user's address
					 * space to start at. */
    register	int	*retBytesPtr;	/* The actual number of bytes 
					 * made accessible. */
    register	Address	*retAddrPtr;	/* The kernel virtual address that
					 * can be used to access the bytes. */
{
    register	Vm_Segment	*segPtr;
    register	Vm_PTE		*ptePtr;
    Vm_VirtAddr			virtAddr;
    int				firstPage;
    int				lastPage;
    Proc_ControlBlock		*procPtr;

    procPtr = Proc_GetCurrentProc();

    /*
     * Parse the virtual address to determine which segment that this page
     * falls into and which page in the segment.
     */
    VmVirtAddrParse(procPtr, startAddr, &virtAddr);
    
    segPtr = virtAddr.segPtr;
    /*
     * See if the beginning address falls into a segment.
     */
    if (segPtr == (Vm_Segment *) NIL) {
	*retBytesPtr = 0;
	*retAddrPtr = (Address) NIL;
	return;
    }

    procPtr->vmPtr->numMakeAcc++;
    *retBytesPtr = numBytes;
    *retAddrPtr = startAddr;
    firstPage = virtAddr.page;
    lastPage = ((unsigned int) (startAddr) + numBytes - 1) >> vmPageShift;
    if (segPtr->type == VM_STACK) {
	/*
	 * If ending address goes past the end of the
	 * stack then truncate it.  
	 */
	if (lastPage > mach_LastUserStackPage) {
	    *retBytesPtr = (int)mach_MaxUserStackAddr - (int)startAddr;
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
	*retBytesPtr = ((lastPage + 1) << vmPageShift) - (int) startAddr;
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
	if (!(*ptePtr & VM_VIRT_RES_BIT)) {
	    break;
	}
    }
    /*
     * If we couldn't map anything then just return.
     */
    if (virtAddr.page == firstPage) {
        VmDecPTUserCount(procPtr->vmPtr->segPtrArray[VM_HEAP]);
	procPtr->vmPtr->numMakeAcc--;
	*retBytesPtr = 0;
	*retAddrPtr = (Address) NIL;
	return;
    }
    /* 
     * If we couldn't make all of the requested pages accessible then return 
     * the number of bytes that we actually made accessible.
     */
    if (virtAddr.page <= lastPage) {
	*retBytesPtr = (virtAddr.page << vmPageShift) - (int) startAddr;
    }
}


/*
 ----------------------------------------------------------------------
 *
 * Vm_MakeUnaccessible
 *
 *	Take the given kernel virtual address and make the range of pages
 *	that it addresses unaccessible.  All that has to be done is to
 *	decrement the in-use count on the page table for the calling process's
 *	heap segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Heap segment page table in use count decremented if the address 
 *	falls into a heap or stack segment.
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

    procPtr = Proc_GetCurrentProc();
    segPtr = procPtr->vmPtr->segPtrArray[VM_HEAP];
    procPtr->vmPtr->numMakeAcc--;

    if (((unsigned int) (addr) >> vmPageShift) >= segPtr->offset) {
	/*
	 * This address falls into a stack or heap segment.  The heap segment
	 * was prevented from being expanded by Vm_MakeAccessible so we have
	 * to let it be expanded now.
	 */
        segPtr->ptUserCount--;
        if (segPtr->ptUserCount < 0) {
            panic("Vm_MakeUnaccessible: expand count < 0\n");
        }
        if (segPtr->ptUserCount == 0) {
            Sync_Broadcast(&segPtr->condition);
        }
    }

    UNLOCK_MONITOR;
}
