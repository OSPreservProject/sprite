/* 
 * vmCOW.c --
 *
 *      This file contains routines that implement copy-on-write.
 *	
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vmStat.h"
#include "vmMachInt.h"
#include "vm.h"
#include "vmInt.h"
#include "user/vm.h"
#include "sync.h"
#include "dbg.h"
#include "list.h"
#include "machine.h"
#include "lock.h"
#include "sys.h"
#include "byte.h"

Boolean	vm_CanCOW = TRUE;

void		DoFork();
void		GiveAwayPage();
Boolean		COWStart();
Vm_Segment	*FindNewMasterSeg();
Boolean		IsResident();
void		COWEnd();
void		SetPTE();
void		CopyPage();

Vm_PTE	cowPTE;


/*
 *----------------------------------------------------------------------
 *
 * VmCOWInit --
 *
 *	Initialize copy-on-write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table entry setup.
 *
 *----------------------------------------------------------------------
 */
void
VmCOWInit()
{
    cowPTE = vm_ZeroPTE;
    cowPTE.protection = VM_UR_PROT;
    cowPTE.validPage = 1;
    cowPTE.resident = 1;
    cowPTE.referenced = 1;
    cowPTE.modified = 1;
}


/*
 *----------------------------------------------------------------------
 *
 * VmSegFork --
 *
 *	Make a copy-on-reference copy of the given segment.  It is assumed
 *	that this routine is called with the source segment not expandable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
VmSegFork(srcSegPtr, destSegPtr)
    Vm_Segment	*srcSegPtr;
    Vm_Segment	*destSegPtr;
{
    VmCOWInfo	*cowInfoPtr;

    cowInfoPtr = (VmCOWInfo *)Mem_Alloc(sizeof(VmCOWInfo));
    (void)COWStart(srcSegPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }
    DoFork(srcSegPtr, destSegPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * DoFork --
 *
 *	Make the dest segment copy-on-reference off of the src segment.
 *	It is assumed that this routine is called with the source segment 
 *	not expandable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Source and destination page tables modified to set up things
 *	as copy-on-write and copy-on-reference.  Also number of COW and COR
 *	pages in each segment is modified.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
DoFork(srcSegPtr, destSegPtr)
    register	Vm_Segment	*srcSegPtr;
    register	Vm_Segment	*destSegPtr;
{
    register	Vm_PTE	*destPTEPtr;
    register	Vm_PTE	*srcPTEPtr;
    int			lastPage;
    int			destPage;
    VmVirtAddr		virtAddr;
    Boolean		isCOW = FALSE;
    VmCOWInfo		*cowInfoPtr;
    int			i;

    LOCK_MONITOR;

    if (srcSegPtr->type == VM_HEAP) {
	virtAddr.page = srcSegPtr->offset;
	lastPage = srcSegPtr->offset + srcSegPtr->numPages - 1;
	srcPTEPtr = srcSegPtr->ptPtr;
	destPTEPtr = destSegPtr->ptPtr;
    } else {
	virtAddr.page = MACH_LAST_USER_STACK_PAGE - srcSegPtr->numPages + 1;
	lastPage = MACH_LAST_USER_STACK_PAGE;
	srcPTEPtr = VmGetPTEPtr(srcSegPtr, virtAddr.page);
	destPTEPtr = VmGetDupPTEPtr(srcSegPtr, destSegPtr, virtAddr.page);
    }

    for (virtAddr.segPtr = srcSegPtr;
	 virtAddr.page <= lastPage;
	 virtAddr.page++, VmIncPTEPtr(srcPTEPtr, 1), 
			  VmIncPTEPtr(destPTEPtr, 1)) {
	*destPTEPtr = vm_ZeroPTE;
	if (!srcPTEPtr->validPage) {
	    continue;
	}
	while (srcPTEPtr->inProgress) {
	    (void)Sync_Wait(&srcSegPtr->condition, FALSE);
	}
	destPTEPtr->validPage = 1;
	if (srcPTEPtr->protection == VM_UR_PROT) {
	    /*
	     * This page is already copy-on-write.  Make child copy-on-ref
	     * off of the parent segment.
	     */
	    destPTEPtr->pfNum =  (unsigned int)srcSegPtr->segNum;
	    destPTEPtr->protection = VM_KRW_PROT;
	    destSegPtr->numCORPages++;
	    vmStat.numCORPages++;
	    isCOW = TRUE;
	} else if (srcPTEPtr->protection != VM_URW_PROT) {
	    /*
	     * This page is already copy-on-reference.  Make the child be
	     * copy-on-ref on the same segment.
	     */
	    destPTEPtr->pfNum = srcPTEPtr->pfNum;
	    destPTEPtr->protection = VM_KRW_PROT;
	    destSegPtr->numCORPages++;
	    vmStat.numCORPages++;
	    isCOW = TRUE;
	} else if (srcPTEPtr->resident || srcPTEPtr->onSwap) {
	    /*
	     * Need to make the src copy-on-write and the dest copy-on-ref.
	     */
	    srcSegPtr->numCOWPages++;
	    vmStat.numCOWPages++;
	    VmSetPageProtInt(&virtAddr, srcPTEPtr, VM_UR_PROT);
	    destPTEPtr->pfNum = (unsigned int)srcSegPtr->segNum;
	    destPTEPtr->protection = VM_KRW_PROT;
	    destSegPtr->numCORPages++;
	    vmStat.numCORPages++;
	    isCOW = TRUE;
	} else {
	    /*
	     * Just a normal everyday pte (zerofill or load from FS).
	     */
	    *destPTEPtr = *srcPTEPtr;
	}
    }

    cowInfoPtr = srcSegPtr->cowInfoPtr;
    if (isCOW) {
	/*
	 * Insert the child into the COW list for the parent segment.
	 */
	destSegPtr->cowInfoPtr = cowInfoPtr;
	List_Insert((List_Links *)destSegPtr, 
		    LIST_ATREAR(&cowInfoPtr->cowList));
	cowInfoPtr->numSegs++;
    }
    cowInfoPtr->copyInProgress = 0;
    Sync_Broadcast(&cowInfoPtr->condition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmCOWDeleteFromSeg --
 *
 *	Invalidate all cow or cor pages from the given range of pages in
 *	the given segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All cow or cor pages in the given range of pages are invalidated.
 *
 *----------------------------------------------------------------------
 */
void
VmCOWDeleteFromSeg(segPtr, firstPage, lastPage)
    register	Vm_Segment	*segPtr;
    int				firstPage;	/* First page to delete. -1
						 * if want the lowest possible
						 * page. */
    int				lastPage;	/* Last page to delete. -1
						 * if want the highest possible
						 * page. */
{
    register	Vm_Segment	*mastSegPtr;
    register	Vm_PTE		*mastPTEPtr;
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;
    VmVirtAddr			virtAddr;

    if (firstPage == -1) {
	/*
	 * Caller wants to invalidate all page for this segment.
	 */
	if (segPtr->type == VM_STACK) {
	    firstPage = MACH_LAST_USER_STACK_PAGE - segPtr->numPages + 1;
	} else {
	    firstPage = segPtr->offset;
	}
	lastPage = firstPage + segPtr->numPages - 1;
    }
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(segPtr, &cowInfoPtr)) {
	return;
    }
    virtAddr.page = firstPage;
    for (virtAddr.page = firstPage, ptePtr = VmGetPTEPtr(segPtr, firstPage);
	 virtAddr.page <= lastPage;
	 virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	if (!ptePtr->validPage || ptePtr->protection == VM_URW_PROT) {
	    /*
	     * Page isn't valid or it is not COW or COR.
	     */
	    continue;
	}
	if (ptePtr->protection == VM_UR_PROT) {
	    Boolean	others;
	    /*
	     * This page is copy-on-write.
	     */
	    mastSegPtr = FindNewMasterSeg(segPtr, virtAddr.page, &others);
	    if (mastSegPtr != (Vm_Segment *)NIL) {
		if (IsResident(ptePtr)) {
		    /*
		     * Resident page.  Give it away to the new master.
		     */
		    GiveAwayPage(segPtr, virtAddr.page, mastSegPtr, others);
		    *ptePtr = vm_ZeroPTE;
		} else {
		    /*
		     * The page is on swap.  Copy it.  Note that we
		     * update the software page table entry here and then
		     * change the protection in hardware so that the page 
		     * can be faulted in.
		     */
		    vmStat.swapPagesCopied++;
		    VmCopySwapPage(segPtr, virtAddr.page, mastSegPtr);
		    mastPTEPtr = VmGetPTEPtr(mastSegPtr, virtAddr.page);
		    mastPTEPtr->onSwap = 1;
		    mastPTEPtr->pfNum = 0;
		    virtAddr.segPtr = mastSegPtr;
		    VmSetPageProt(&virtAddr, mastPTEPtr, 
				  others ? VM_UR_PROT : VM_URW_PROT);
		}
		mastSegPtr->numCORPages--;
		if (others) {
		    mastSegPtr->numCOWPages++;
		} else {
		    if (mastSegPtr->numCORPages == 0 && 
		        mastSegPtr->numCOWPages == 0) {
			mastSegPtr->cowInfoPtr->numSegs--;
			List_Remove((List_Links *)mastSegPtr);
			mastSegPtr->cowInfoPtr = (VmCOWInfo *)NIL;
		    }
		}
	    }
	    segPtr->numCOWPages--;
	} else {
	    /*
	     * This page is copy-on-reference.
	     */
	    segPtr->numCORPages--;
	}
    }

    COWEnd(segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GiveAwayPage --
 *
 *	Transfer a page from one segment to another.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Source segments page table has the page zapped and the
 *	destination page table takes over ownership of the page.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
GiveAwayPage(srcSegPtr, virtPage, destSegPtr, others)
    register	Vm_Segment	*srcSegPtr;
    int				virtPage;
    register	Vm_Segment	*destSegPtr;
    Boolean			others;
{
    VmVirtAddr	virtAddr;
    Vm_PTE	pte;

    LOCK_MONITOR;

    virtAddr.segPtr = srcSegPtr;
    virtAddr.page = virtPage;

    pte = VmGetPTE(&virtAddr);
    VmPageInvalidateInt(&virtAddr);
    virtAddr.segPtr = destSegPtr;
    if (!others) {
	pte.protection = VM_URW_PROT;
    }
    VmSetPTE(&virtAddr, pte);
    VmPageSwitch(VmPhysToVirtPage(pte.pfNum), destSegPtr);
    srcSegPtr->resPages--;
    destSegPtr->resPages++;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * COWStart --
 *
 *	Mark the segment as copy-on-write in progress.  After this routine
 *	is called all future copy-on-writes are blocked until released
 *	by a call to VmCOWDeleteFromSeg.
 *
 * Results:
 *	If the segment is not currently involved in a copy-on-write chain and
 *	*cowInfoPtrPtr is NIL then this routine returns FALSE.  Otherwise
 *	it returns TRUE.
 *
 * Side effects:
 *	Copy-in-progress flag is set.  Also if there is no copy-on-write
 *	chain yet and *cowInfoPtrPtr is not equal to NIL, then a new
 *	one is created and *cowInfoPtrPtr is set to NIL.
 *
 *----------------------------------------------------------------------
 */
ENTRY static Boolean
COWStart(segPtr, cowInfoPtrPtr)
    register	Vm_Segment	*segPtr;	/* Segment to begin COW for.*/
    register	VmCOWInfo	**cowInfoPtrPtr;/* Pointer to pointer to
						 * COW info struct that can
						 * be used if no struct yet
						 * exists. */ 
{
    register	VmCOWInfo	*cowInfoPtr;

    LOCK_MONITOR;

again:

    if (segPtr->cowInfoPtr == (VmCOWInfo *)NIL) {
	cowInfoPtr = *cowInfoPtrPtr;
	if (cowInfoPtr == (VmCOWInfo *)NIL) {
	    UNLOCK_MONITOR;
	    return(FALSE);
	}
	segPtr->cowInfoPtr = cowInfoPtr;
	cowInfoPtr->copyInProgress = TRUE;
	List_Init(&cowInfoPtr->cowList);
	List_Insert((List_Links *)segPtr, LIST_ATREAR(&cowInfoPtr->cowList));
	cowInfoPtr->numSegs = 1;
	*cowInfoPtrPtr = (VmCOWInfo *)NIL;
    } else {
	cowInfoPtr = segPtr->cowInfoPtr;
	if (cowInfoPtr->copyInProgress) {
	    (void)Sync_Wait(&cowInfoPtr->condition, FALSE);
	    goto again;
	} else {
	    cowInfoPtr->copyInProgress = TRUE;
	}
    }

    UNLOCK_MONITOR;
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * FindNewMasterSeg --
 *
 *	Find a segment that is sharing the given page copy-on-reference.
 *
 * Results:
 *	Pointer to segment that is sharing the given page copy-on-reference.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Vm_Segment *
FindNewMasterSeg(segPtr, page, othersPtr)
    register	Vm_Segment	*segPtr;	/* Current master. */
    int				page;		/* Virtual page. */
    Boolean			*othersPtr;	/* TRUE if there are other
						 * COW children. */
{
    register	List_Links	*cowList;
    register	Vm_Segment	*newSegPtr;
    register	Vm_PTE		*ptePtr;
    Vm_Segment			*mastSegPtr = (Vm_Segment *)NIL;

    *othersPtr = FALSE;
    cowList = &segPtr->cowInfoPtr->cowList;
    newSegPtr = (Vm_Segment *)List_Next((List_Links  *)segPtr);
    while (!List_IsAtEnd(cowList, (List_Links *)newSegPtr)) {
	ptePtr = VmGetPTEPtr(newSegPtr, page);
	if (ptePtr->validPage && ptePtr->protection == VM_KRW_PROT &&
	    ptePtr->pfNum == segPtr->segNum) {
	    if (mastSegPtr != (Vm_Segment *)NIL) {
		ptePtr->pfNum = mastSegPtr->segNum;
		*othersPtr = TRUE;
	    } else {
		mastSegPtr = newSegPtr;
	    }
	}
	newSegPtr = (Vm_Segment *)List_Next((List_Links  *)newSegPtr);
    }
    return(mastSegPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * IsResident --
 *
 *	Determine if the page is resident in the given page table entry.
 *
 * Results:
 *	TRUE if the page is resident. 
 *
 * Side effects:
 *	Page locked down if resident.
 *
 *----------------------------------------------------------------------
 */
ENTRY static Boolean
IsResident(ptePtr)
    Vm_PTE	*ptePtr;
{
    Boolean	retVal;

    LOCK_MONITOR;

    if (ptePtr->resident) {
	VmLockPageInt(VmPhysToVirtPage(ptePtr->pfNum));
	retVal = TRUE;
    } else {
	retVal = FALSE;
    }

    UNLOCK_MONITOR;

    return(retVal);
}



/*
 *----------------------------------------------------------------------
 *
 * COWEnd --
 *
 *	Clean up after a copy-on-write or copy-on-ref has happened.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*cowInfoPtrPtr is set to point to COW info struct to free if there
 *	are no segments left COW.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
COWEnd(segPtr, cowInfoPtrPtr)
    register	Vm_Segment	*segPtr;	 /* Segment that was involved
						  * in the COW or COR. */
    VmCOWInfo			**cowInfoPtrPtr; /* Set to point to a COW
						  * info struct to free. */
{
    register	VmCOWInfo	*cowInfoPtr;

    LOCK_MONITOR;

    cowInfoPtr = segPtr->cowInfoPtr;
    cowInfoPtr->copyInProgress = FALSE;
    Sync_Broadcast(&cowInfoPtr->condition);
    if (segPtr->numCOWPages == 0 && segPtr->numCORPages == 0) {
	List_Remove((List_Links *)segPtr);
	segPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	cowInfoPtr->numSegs--;
    }
    if (cowInfoPtr->numSegs == 0) {
	*cowInfoPtrPtr = cowInfoPtr;
    } else if (cowInfoPtr->numSegs == 1) {
	Vm_Segment	*cowSegPtr;
	int		firstPage;

	/*
	 * Only one segment left.  Return this segment back to normal
	 * protection and clean up.
	 */
	*cowInfoPtrPtr = cowInfoPtr;
	cowSegPtr = (Vm_Segment *)List_First(&cowInfoPtr->cowList);
	cowSegPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	if (cowSegPtr->type == VM_STACK) {
	    firstPage = MACH_LAST_USER_STACK_PAGE - cowSegPtr->numPages + 1;
	} else {
	    firstPage = cowSegPtr->offset;
	}
	VmSetSegProtInt(cowSegPtr, firstPage, 
			firstPage + cowSegPtr->numPages - 1,
		        VM_URW_PROT, TRUE);
	cowSegPtr->numCORPages = 0;
	cowSegPtr->numCOWPages = 0;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * VmCOR --
 *
 *	Handle a copy-on-reference fault.  If the virtual address is not
 *	truly COR then don't do anything.
 *
 * Results:
 *	Status from VmPageServerRead if had to read from swap space.  
 *	Otherwise SUCCESS.
 *
 * Side effects:
 *	Page table for current segment is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmCOR(virtAddrPtr)
    register	VmVirtAddr	*virtAddrPtr;
{
    register	Vm_PTE		*ptePtr;
    register	Vm_PTE		*mastPTEPtr;
    register	Vm_Segment	*mastSegPtr;
    Vm_Segment			*segPtr;
    int				virtFrameNum;
    ReturnStatus		status;
    VmCOWInfo			*cowInfoPtr;
    VmVirtAddr			virtAddr;

    vmStat.numCORFaults++;
    segPtr = virtAddrPtr->segPtr;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(segPtr, &cowInfoPtr)) {
	vmStat.quickCORFaults++;
	return;
    }
    ptePtr = VmGetPTEPtr(segPtr, virtAddrPtr->page);
    if (ptePtr->protection != VM_KRW_PROT) {
	vmStat.quickCORFaults++;
	return;
    }
    mastSegPtr = VmGetSegPtr(ptePtr->pfNum);
    mastPTEPtr = VmGetPTEPtr(mastSegPtr, virtAddrPtr->page);
    virtFrameNum = VmPageAllocate(virtAddrPtr, TRUE);
    if (IsResident(mastPTEPtr)) {
	/*
	 * The page is resident in memory so copy it.
	 */
	CopyPage(VmPhysToVirtPage(mastPTEPtr->pfNum), virtFrameNum);
	VmUnlockPage(VmPhysToVirtPage(mastPTEPtr->pfNum));
    } else {
	/*
	 * Load the page off of swap space.
	 */
	virtAddr.segPtr = mastSegPtr;
	virtAddr.page = virtAddrPtr->page;
	status = VmPageServerRead(&virtAddr, virtFrameNum);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "VmCOR: Couldn't read page, status <%x>\n",
				   status);
	    return(status);
	}
    }

    segPtr->numCORPages--;
    /*
     * Change the protection and such for this page.  Note that we don't
     * validate the page.  That is our callers job.
     */
    ptePtr->protection = VM_URW_PROT;
    ptePtr->pfNum = VmVirtToPhysPage(virtFrameNum);
    ptePtr->modified = 1;
    ptePtr->resident = 1;
    VmUnlockPage(virtFrameNum);

    cowInfoPtr = (VmCOWInfo *)NIL;
    COWEnd(segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * VmCOW --
 *
 *	Handle a copy-on-write fault.  If the current virtual address is
 *	not truly COW then we don't do anything.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page table for the given virtual address is modified and the page
 *	table of the new master (if any) is modified.
 *
 *----------------------------------------------------------------------
 */
void
VmCOW(virtAddrPtr)
    register	VmVirtAddr	*virtAddrPtr;
{
    register	Vm_Segment	*segPtr;
    register	Vm_PTE		*ptePtr;
    register	Vm_Segment	*mastSegPtr;
    VmVirtAddr			virtAddr;
    VmCOWInfo			*cowInfoPtr;
    Boolean			others;

    vmStat.numCOWFaults;
    segPtr = virtAddrPtr->segPtr;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(segPtr, &cowInfoPtr)) {
	vmStat.quickCOWFaults;
	return;
    }
    ptePtr = VmGetPTEPtr(segPtr, virtAddrPtr->page);
    if (ptePtr->protection != VM_UR_PROT) {
	vmStat.quickCOWFaults;
	return;
    }
    if (!IsResident(ptePtr)) {
	return;
    }

    mastSegPtr = FindNewMasterSeg(segPtr, virtAddrPtr->page, &others);
    if (mastSegPtr != (Vm_Segment *)NIL) {
	int	virtFrameNum;
	Vm_PTE	pte;

	/*
	 * Copy the page to the new master.
	 */
	mastSegPtr->numCORPages--;
	virtAddr.segPtr = mastSegPtr;
	virtAddr.page = virtAddrPtr->page;
	virtFrameNum = VmPageAllocate(&virtAddr, TRUE);
	CopyPage(VmPhysToVirtPage(ptePtr->pfNum), virtFrameNum);
	pte = cowPTE;
	pte.pfNum = VmVirtToPhysPage(virtFrameNum);
	if (others) {
	    mastSegPtr->numCOWPages++;
	} else {
	    pte.protection = VM_URW_PROT;
	}
	/*
	 * Atomically update the page table entry for the copy-on-write 
	 * child.  This has to be done under the monitor because the child
	 * could be executing as we modify its protection.
	 */
	SetPTE(&virtAddr, pte);
	VmUnlockPage(virtFrameNum);
	if (!others) {
	    if (mastSegPtr->numCORPages == 0 && mastSegPtr->numCOWPages == 0) {
		mastSegPtr->cowInfoPtr->numSegs--;
		List_Remove((List_Links *)mastSegPtr);
		mastSegPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	    }
	}
    } else {
	vmStat.quickCOWFaults;
    }

    /*
     * Change callers protection back to normal.
     */
    VmSetPageProt(virtAddrPtr, ptePtr, VM_URW_PROT);
    segPtr->numCOWPages--;

    cowInfoPtr = (VmCOWInfo *)NIL;
    COWEnd(segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * SetPTE --
 *
 *	Set the given pte at the given virtual address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
SetPTE(virtAddrPtr, pte)
    VmVirtAddr	*virtAddrPtr;
    Vm_PTE	pte;
{
    LOCK_MONITOR;

    VmSetPTE(virtAddrPtr, pte);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * CopyPage --
 *
 *	Copy the given page frame to the given destination.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CopyPage(srcPF, destPF)
    int	srcPF;
    int	destPF;
{
    register	Address	srcMappedAddr;
    register	Address	destMappedAddr;

    srcMappedAddr = VmMapPage(srcPF);
    destMappedAddr = VmMapPage(destPF);
    Byte_Copy(VM_PAGE_SIZE, srcMappedAddr, destMappedAddr);
    VmUnmapPage(srcMappedAddr);
    VmUnmapPage(destMappedAddr);
}
