/* 
 * vmPrefetch.c --
 *
 *      This file contains routines to do prefetch.
 *	
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vmStat.h"
#include "vmMach.h"
#include "vm.h"
#include "vmInt.h"
#include "user/vm.h"
#include "sync.h"
#include "dbg.h"
#include "list.h"
#include "lock.h"
#include "sys.h"
#include "byte.h"
#include "mem.h"

Boolean	vmPrefetch = FALSE;

/*
 * Information needed to do a prefetch.
 */
typedef struct {
    Vm_VirtAddr	virtAddr;
} PrefetchInfo;

Boolean	StartPrefetch();
void	DoPrefetch();
void	FinishPrefetch();
void	AbortPrefetch();


/*
 * ----------------------------------------------------------------------------
 *
 * VmPrefetch --
 *
 *	Start a fetch for the next page if it isn't resident and its on
 *	swap or in a file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated.
 *
 * ----------------------------------------------------------------------------
 */
void
VmPrefetch(virtAddrPtr, ptePtr)
    register	Vm_VirtAddr	*virtAddrPtr;
    register	Vm_PTE		*ptePtr;
{
    register	PrefetchInfo	*prefetchInfoPtr;
    register	Vm_Segment	*segPtr;

    segPtr = virtAddrPtr->segPtr;
    if (segPtr->type == VM_STACK) {
	if (virtAddrPtr->page == mach_LastUserStackPage) {
	    return;
	}
    } else {
	if (virtAddrPtr->page == segPtr->offset + segPtr->numPages - 1) {
	    return;
	}
    }
    if (!(*ptePtr & VM_VIRT_RES_BIT) || 
        (*ptePtr & (VM_ZERO_FILL_BIT | VM_PHYS_RES_BIT | VM_IN_PROGRESS_BIT | 
		    VM_COR_BIT | VM_COW_BIT)) || 
	 !StartPrefetch(segPtr, ptePtr)) {
	return;
    }
    prefetchInfoPtr = (PrefetchInfo *)Mem_Alloc(sizeof(PrefetchInfo));
    prefetchInfoPtr->virtAddr.segPtr = segPtr;
    prefetchInfoPtr->virtAddr.page = virtAddrPtr->page + 1;
    prefetchInfoPtr->virtAddr.flags = 0;
    switch (segPtr->type) {
	case VM_CODE:
	    vmStat.codePrefetches++;
	    break;
	case VM_HEAP:
	    if (*ptePtr & VM_ON_SWAP_BIT) {
		vmStat.heapSwapPrefetches++;
	    } else {
		vmStat.heapFSPrefetches++;
	    }
	    break;
	case VM_STACK:
	    vmStat.stackPrefetches++;
	    break;
    }
    Proc_CallFunc(DoPrefetch, (ClientData)prefetchInfoPtr, 0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * StartPrefetch
 *
 *	Set up things for the next page to be fetched.
 *
 * Results:
 *	TRUE if decided that the next page should be prefetched.
 *
 * Side effects:
 *	In progress bit set in the pte if a prefetch should be done.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static Boolean
StartPrefetch(segPtr, ptePtr)
    register	Vm_Segment	*segPtr;
    register	Vm_PTE		*ptePtr;
{
    LOCK_MONITOR;

    if (*ptePtr & (VM_ZERO_FILL_BIT | VM_PHYS_RES_BIT | 
	            VM_IN_PROGRESS_BIT | VM_COR_BIT | VM_COW_BIT)) {
	UNLOCK_MONITOR;
	return(FALSE);
    }
    *ptePtr |= VM_IN_PROGRESS_BIT;
    segPtr->ptUserCount++;

    UNLOCK_MONITOR;
    return(TRUE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DoPrefetch --
 *
 *	Fetch the given page for the segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
DoPrefetch(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register	PrefetchInfo	*prefetchInfoPtr;
    unsigned	int		virtFrameNum;
    register	Vm_PTE		*ptePtr;
    ReturnStatus		status;

    prefetchInfoPtr = (PrefetchInfo *)data;
    ptePtr = VmGetPTEPtr(prefetchInfoPtr->virtAddr.segPtr, 
			 prefetchInfoPtr->virtAddr.page);
    /*
     * Fetch a page frame.  Note that we don't block if no memory is
     * available because we are a process out of the call-func process pool.
     * If we block then we can easily use up all of the processes and there
     * will be noone left to clean memory.  Thus it could cause deadlock.
     */
    virtFrameNum = VmPageAllocate(&prefetchInfoPtr->virtAddr, 
				  VM_ABORT_WHEN_DIRTY);
    if (virtFrameNum == VM_NO_MEM_VAL) {
	vmStat.prefetchAborts++;
	AbortPrefetch(&prefetchInfoPtr->virtAddr, ptePtr);
	goto exit;
    }
    *ptePtr |= virtFrameNum;
    if (*ptePtr & VM_ON_SWAP_BIT) {
	vmStat.psFilled++;
	status = VmPageServerRead(&prefetchInfoPtr->virtAddr, virtFrameNum);
    } else {
	vmStat.fsFilled++;
	status = VmFileServerRead(&prefetchInfoPtr->virtAddr, virtFrameNum);
    }
    FinishPrefetch(&prefetchInfoPtr->virtAddr, ptePtr);
    if (status != SUCCESS) {
	VmKillSharers(prefetchInfoPtr->virtAddr.segPtr);
    }
exit:
    VmDecPTUserCount(prefetchInfoPtr->virtAddr.segPtr);
    Mem_Free((Address)data);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FinishPrefetch --
 *
 *	Finish up a prefetch,
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
FinishPrefetch(virtAddrPtr, ptePtr)
    Vm_VirtAddr		*virtAddrPtr;
    register	Vm_PTE	*ptePtr;
{
    register	Vm_Segment	*segPtr;

    LOCK_MONITOR;

    segPtr = virtAddrPtr->segPtr;
    segPtr->resPages++;
    *ptePtr |= VM_PHYS_RES_BIT | VM_PREFETCH_BIT;
    *ptePtr &= ~VM_IN_PROGRESS_BIT;
    Sync_Broadcast(&segPtr->condition);
    VmUnlockPageInt(Vm_GetPageFrame(*ptePtr));

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * AbortPrefetch --
 *
 *	Clear the in progress bit from the page table entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
AbortPrefetch(virtAddrPtr, ptePtr)
    Vm_VirtAddr	*virtAddrPtr;
    Vm_PTE	*ptePtr;
{
    LOCK_MONITOR;

    *ptePtr &= ~VM_IN_PROGRESS_BIT;
    Sync_Broadcast(&virtAddrPtr->segPtr->condition);

    UNLOCK_MONITOR;
}

