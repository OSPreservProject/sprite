/* 
 * vmCOW.c --
 *
 * This file contains routines that implement copy-on-write.
 *
 * Copy-on-write (COW) is implemented by linking all related instances of a
 * segment in a list. "Related instances" means all segments related by a fork
 * of some initial segment, including childre, grandchildren, etc.  Each page
 * in each related segment is marked either OK (meaning the page is no longer
 * involved in any COW activity), COW or COR (meaning it's copy-on-reference).
 * COR pages have no associated backing store or main-memory contents yet, but
 * the PTE for each COR page names the segment from which this page is to be 
 * copied by setting the page frame number equal to the segment number of the
 * source of the page.  Pages are marked COW and COR through the use of the
 * protection bits in the PTE.  If the page is COW, the protection is set to
 * VM_UR_PROT.  If the page is COR, then the protection is set to VM_KRW_PROT.
 * If the page is neither, then the protection is VM_URW_PROT.  Note that
 * only heap and stack segments are ever copy-on-write.
 *
 * When a COR page is referenced, the routine VmCOR is called.  The source of
 * the page is found through the page frame number and a copy of the page is 
 * made.  The source is left COW.  When a COW page (call it A) is written then
 * the routine VmCOW is called.  Another page (call it B) is found by
 * traversing the list of segments.  Page A is then copied to page B and A is
 * marked OK (no longer COW).  If there are any other pages that were COR off 
 * of A besides B, then B is marked COW and each of the other pages is marked
 * COR off of B.  Otherwise B is marked OK.
 *
 * During a fork the routine VmSegFork is called.  The newly created segment
 * is added to the list of related segments which contains the new segment's
 * parent.  For each page that is COR in the parent, the child is marked COR
 * off of the same page.  For each page that is marked COW in the parent,
 * the child is marked COR off of the parent.  For each parent page that is
 * resident or on swap space, the child is marked COR off of the parent.
 * All other pages (zero fill or demand load from the file system) are set up
 * to be the same for the child.
 *
 * When a segment or a portion of a segment which contains COW pages is
 * deleted then all of the COW pages must be duplicated.  This is done
 * by calling the routine VmCOWDeleteFromSeg.  For each COW
 * page (call it A), if there is segment which is COR off of A, then A is
 * copied to B.  If A is resident this is done by remapping the page in A
 * onto B.  Otherwise the swap space behind A is copied to B's backing store.
 * 
 * When a segment is migrated, a copy of it has to be made.  This is done by
 * calling the routine VmCOWCopy.  For each COR page a COR fault is simulated.
 * For each COW page a COW fault is simulated.
 *
 * SYNCHRONIZATION
 *
 * The routines in this file are designed to be called by routines in 
 * vmPage.c and vmSeg.c.  Whenever any of these routines are called  it is
 * assumed that the segment cannot have ranges of virtual addresses validated
 * or invalidated from the segment.  Also only one operation can occur to a
 * copy-on-write chain at one time.  This is assured by embedding a lock into 
 * a common data structure that all  segments in a copy-on-write chain 
 * reference (VmCOWInfo).  Since only one operation can happen at a time and a
 * segment's virtual address space cannot be mucked with, this greatly 
 * simplifies the code.
 *
 * CLEANUP
 *
 * There are several cases in which the COW chain must be cleaned up.  The
 * cases and how they are handled are:
 *
 * 1) After a COW or a COR fault occurs for page A, there may be no more pages
 *    that are COR off of A.  However, there is no way of knowing this without
 *    searching the entire chain.  In this case A is left COW and then if
 *    a subsequent COW fault occurs on A, A will be marked OK.
 * 2) After a COW or COR fault there may be no more COR or COW pages in the
 *    faulting segment.  In this case the segment is deleted from the list.
 * 3) After a COR fault, COW fault, or a segment is deleted there may be
 *    only one segement left in the list.  In this case all pages are marked
 *    OK in the segment, the segment is removed from the chain and the
 *    VmCOWInfo struct is freed.
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
ReturnStatus	COR();
void		COW();
int		GetMasterPF();

Vm_PTE	resPTE;
Vm_PTE	swapPTE;


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
    resPTE = vm_ZeroPTE;
    resPTE.validPage = 1;
    resPTE.resident = 1;
    resPTE.referenced = 1;
    resPTE.modified = 1;
    resPTE.protection = VM_URW_PROT;
    swapPTE = vm_ZeroPTE;
    swapPTE.validPage = 1;
    swapPTE.onSwap = 1;
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
    register	int	virtPage;
    register	int	lastPage;
    register	int	numCORPages = 0;
    register	int	numCOWPages = 0;
    VmVirtAddr		virtAddr;
    VmCOWInfo		*cowInfoPtr;
    Vm_PTE		corPTE;

    LOCK_MONITOR;

    corPTE = vm_ZeroPTE;
    corPTE.validPage = 1;
    corPTE.protection = VM_KRW_PROT;
    corPTE.pfNum = srcSegPtr->segNum;

    if (srcSegPtr->type == VM_HEAP) {
	virtPage = srcSegPtr->offset;
	lastPage = virtPage + srcSegPtr->numPages - 1;
	srcPTEPtr = srcSegPtr->ptPtr;
	destPTEPtr = destSegPtr->ptPtr;
    } else {
	virtPage = MACH_LAST_USER_STACK_PAGE - srcSegPtr->numPages + 1;
	lastPage = MACH_LAST_USER_STACK_PAGE;
	srcPTEPtr = VmGetPTEPtr(srcSegPtr, virtPage);
	destPTEPtr = VmGetPTEPtr(destSegPtr, virtPage);
    }

    virtAddr.segPtr = srcSegPtr;
    virtAddr.flags = 0;
    for (; virtPage <= lastPage;
	 virtPage++, VmIncPTEPtr(srcPTEPtr, 1), VmIncPTEPtr(destPTEPtr, 1)) {
	if (!srcPTEPtr->validPage) {
	    *destPTEPtr = vm_ZeroPTE;
	    continue;
	}
	while (srcPTEPtr->inProgress) {
	    (void)Sync_Wait(&srcSegPtr->condition, FALSE);
	}
	if (srcPTEPtr->protection == VM_URW_PROT) {
	    if (srcPTEPtr->resident || srcPTEPtr->onSwap) {
		/*
		 * Need to make the src copy-on-write and the dest copy-on-ref.
		 */
		virtAddr.page = virtPage;
		VmSetPageProtInt(&virtAddr, srcPTEPtr, VM_UR_PROT);
		numCOWPages++;
		*destPTEPtr = corPTE;
		numCORPages++;
	    } else {
		/*
		 * Just a normal everyday pte (zerofill or load from FS).
		 */
		*destPTEPtr = *srcPTEPtr;
	    }
	} else if (srcPTEPtr->protection == VM_UR_PROT) {
	    /*
	     * This page is already copy-on-write.  Make child copy-on-ref
	     * off of the parent segment.
	     */
	    *destPTEPtr = corPTE;
	    numCORPages++;
	} else {
	    /*
	     * This page is already copy-on-reference.  Make the child be
	     * copy-on-ref on the same segment.
	     */
	    *destPTEPtr = *srcPTEPtr;
	    numCORPages++;
	}
    }

    cowInfoPtr = srcSegPtr->cowInfoPtr;
    if (numCORPages > 0) {
	vmStat.numCOWPages += numCOWPages;
	vmStat.numCORPages += numCORPages;
	srcSegPtr->numCOWPages += numCOWPages;
	destSegPtr->numCORPages = numCORPages;
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
 * VmCOWCopySeg --
 *
 *	Make of the copy of the given segment.  This includes handling all
 *	copy-on-write and copy-on-reference pages.  This segment will
 *	be removed from its copy-on-write chain.  It is assumed that the
 *	calling segment cannot be expanded.
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
VmCOWCopySeg(segPtr)
    register	Vm_Segment	*segPtr;
{
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;
    VmVirtAddr			virtAddr;
    int				firstPage;
    int				lastPage;

    if (segPtr->type == VM_STACK) {
	firstPage = MACH_LAST_USER_STACK_PAGE - segPtr->numPages + 1;
    } else {
	firstPage = segPtr->offset;
    }
    lastPage = firstPage + segPtr->numPages - 1;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(segPtr, &cowInfoPtr)) {
	return;
    }
    virtAddr.segPtr = segPtr;
    virtAddr.flags = 0;
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
	    /*
	     * This page is copy-on-write.
	     */
	    COW(&virtAddr, ptePtr, IsResident(ptePtr), FALSE);
	} else {
	    /*
	     * This page is copy-on-reference.
	     */
	    COR(&virtAddr, ptePtr);
	}
    }

    cowInfoPtr = (VmCOWInfo *)NIL;
    COWEnd(segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * VmCOWDeleteFromSeg --
 *
 *	Invalidate all cow or cor pages from the given range of pages in
 *	the given segment.  It is assumed that this routine is called with
 *	the segment not expandable.  It does not modify the protection
 *	in the page table under the assumption that the caller will clean
 *	things up in the page table.
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
    register	int		firstPage;	/* First page to delete. -1
						 * if want the lowest possible
						 * page. */
    register	int		lastPage;	/* Last page to delete. -1
						 * if want the highest possible
						 * page. */
{
    register	Vm_Segment	*mastSegPtr;
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;
    VmVirtAddr			virtAddr;

    if (firstPage == -1) {
	/*
	 * Caller wants to invalidate all pages for this segment.  This
	 * is only done when the segment is deleted.
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
    virtAddr.segPtr = segPtr;
    virtAddr.flags = 0;
    for (ptePtr = VmGetPTEPtr(segPtr, firstPage);
	 firstPage <= lastPage;
	 firstPage++, VmIncPTEPtr(ptePtr, 1)) {
	if (!ptePtr->validPage || ptePtr->protection == VM_URW_PROT) {
	    /*
	     * Page isn't valid or it is not COW or COR.
	     */
	    continue;
	}
	if (ptePtr->protection == VM_UR_PROT) {
	    /*
	     * Page is copy-on-write.
	     */
	    virtAddr.page = firstPage;
	    COW(&virtAddr, ptePtr, IsResident(ptePtr), TRUE);
	} else {
	    /*
	     * Page is copy-on-reference.
	     */
	    segPtr->numCORPages--;
	}
    }

    cowInfoPtr = (VmCOWInfo *)NIL;
    COWEnd(segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * COWStart --
 *
 *	Mark the segment as copy-on-write in progress.  After this routine
 *	is called all future copy-on-writes are blocked until released
 *	by a call to COWEnd.
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
    Boolean			*othersPtr;	/* Set to TRUE if there are
						 * other COW children. */
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
 *	truly COR then don't do anything.  It is assumed that the given
 *	segment is not-expandable.
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
    VmCOWInfo			*cowInfoPtr;
    ReturnStatus		status;

    vmStat.numCORFaults++;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(virtAddrPtr->segPtr, &cowInfoPtr)) {
	vmStat.quickCORFaults++;
	return(SUCCESS);
    }
    ptePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
    if (ptePtr->protection != VM_KRW_PROT) {
	vmStat.quickCORFaults++;
	return(SUCCESS);
    }

    status = COR(virtAddrPtr, ptePtr);

    cowInfoPtr = (VmCOWInfo *)NIL;
    COWEnd(virtAddrPtr->segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }

    return(status);
}



/*
 *----------------------------------------------------------------------
 *
 * COR --
 *
 *	Handle a copy-on-reference fault.
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
COR(virtAddrPtr, ptePtr)
    register	VmVirtAddr	*virtAddrPtr;
    register	Vm_PTE		*ptePtr;
{
    register	Vm_Segment	*mastSegPtr;
    int				virtFrameNum;
    int				mastVirtPF;
    ReturnStatus		status;
    Vm_PTE			pte;

    mastSegPtr = VmGetSegPtr(ptePtr->pfNum);
    virtFrameNum = VmPageAllocate(virtAddrPtr, TRUE);
    mastVirtPF = GetMasterPF(mastSegPtr, virtAddrPtr->page);
    if (mastVirtPF > 0) {
	/*
	 * The page is resident in memory so copy it.
	 */
	CopyPage(mastVirtPF, virtFrameNum);
	VmUnlockPage(mastVirtPF);
    } else {
	/*
	 * Load the page off of swap space.
	 */
	VmVirtAddr	virtAddr;

	virtAddr.segPtr = mastSegPtr;
	virtAddr.page = virtAddrPtr->page;
	virtAddr.flags = 0;
	status = VmPageServerRead(&virtAddr, virtFrameNum);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "VmCOR: Couldn't read page, status <%x>\n",
				   status);
	    return(status);
	}
    }

    virtAddrPtr->segPtr->numCORPages--;
    pte = resPTE;
    pte.pfNum = VmVirtToPhysPage(virtFrameNum);
    SetPTE(virtAddrPtr, pte);
    VmUnlockPage(virtFrameNum);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * GetMasterPF --
 *
 *	Return the page frame from the master segments page table
 *	entry.  0 if the page is not resident.
 *
 * Results:
 *	The page frame from the masters PTE.  0 if not resident.
 *	Otherwise SUCCESS.
 *
 * Side effects:
 *	Page frame returned locked if resident.
 *
 *----------------------------------------------------------------------
 */
ENTRY int
GetMasterPF(mastSegPtr, virtPage)
    Vm_Segment	*mastSegPtr;
    int		virtPage;
{
    int			pf;
    register	Vm_PTE	*mastPTEPtr;

    LOCK_MONITOR;

    mastPTEPtr = VmGetPTEPtr(mastSegPtr, virtPage);
    if (mastPTEPtr->resident) {
	pf = VmPhysToVirtPage(mastPTEPtr->pfNum);
	VmLockPageInt(pf);
    } else {
	pf = 0;
    }

    UNLOCK_MONITOR;

    return(pf);
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
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;
    register	Vm_Segment	*mastSegPtr;
    Boolean			others;

    vmStat.numCOWFaults++;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(virtAddrPtr->segPtr, &cowInfoPtr)) {
	vmStat.quickCOWFaults++;
	return;
    }
    ptePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
    if (ptePtr->protection != VM_UR_PROT) {
	vmStat.quickCOWFaults++;
	return;
    }

    if (!IsResident(ptePtr)) {
	return;
    }

    COW(virtAddrPtr, ptePtr, TRUE, FALSE);

    cowInfoPtr = (VmCOWInfo *)NIL;
    COWEnd(virtAddrPtr->segPtr, &cowInfoPtr);
    if (cowInfoPtr != (VmCOWInfo *)NIL) {
	Mem_Free((Address)cowInfoPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * COW --
 *
 *	Handle a copy-on-write fault.
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
COW(virtAddrPtr, ptePtr, isResident, deletePage)
    register	VmVirtAddr	*virtAddrPtr;	/* Virtual address to copy.*/
    register	Vm_PTE		*ptePtr;	/* Pointer to the page table
						 * entry. */
    Boolean			isResident;	/* TRUE => The page is resident
						 *	   in memory and locked
						 *	   down. */
    Boolean			deletePage;	/* TRUE => Delete the page
						 *         after copying. */
{
    register	Vm_Segment	*mastSegPtr;
    VmVirtAddr			virtAddr;
    Boolean			others;
    int				virtFrameNum;
    Vm_PTE			pte;

    mastSegPtr = FindNewMasterSeg(virtAddrPtr->segPtr, virtAddrPtr->page,
				  &others);
    if (mastSegPtr != (Vm_Segment *)NIL) {
	mastSegPtr->numCORPages--;
	if (others) {
	    mastSegPtr->numCOWPages++;
	}
	virtAddr.segPtr = mastSegPtr;
	virtAddr.page = virtAddrPtr->page;
	virtAddr.flags = 0;
	if (isResident) {
	    /*
	     * The page is resident and locked down by our caller. 
	     */
	    if (deletePage) {
		/*
		 * This page is being invalidated. In this case just give
		 * away the page, no need to copy it.
		 */
		GiveAwayPage(virtAddrPtr->segPtr, virtAddrPtr->page,
			     mastSegPtr, others);
	    } else {
		/*
		 * Copy the page.
		 */
		virtFrameNum = VmPageAllocate(&virtAddr, TRUE);
		CopyPage(VmPhysToVirtPage(ptePtr->pfNum), virtFrameNum);
		pte = resPTE;
		pte.pfNum = VmVirtToPhysPage(virtFrameNum);
		pte.protection = others ? VM_UR_PROT : VM_URW_PROT;
		SetPTE(&virtAddr, pte);
		VmUnlockPage(virtFrameNum);
		VmUnlockPage(VmPhysToVirtPage(ptePtr->pfNum));
	    }
	} else {
	    /*
	     * The page is on swap space.
	     */
	    VmCopySwapPage(virtAddrPtr->segPtr, virtAddrPtr->page, mastSegPtr);
	    pte = swapPTE;
	    pte.protection = others ? VM_UR_PROT : VM_URW_PROT;
	    SetPTE(&virtAddr, pte);
	}
	if (mastSegPtr->numCOWPages == 0 && mastSegPtr->numCORPages == 0) {
	    mastSegPtr->cowInfoPtr->numSegs--;
	    List_Remove((List_Links *)mastSegPtr);
	    mastSegPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	}
    } else {
	vmStat.quickCOWFaults++;
    }

    /*
     * Change callers protection back to normal if the pte is not being 
     * deleted.  No need to waste our time otherwise.
     */
    if (!deletePage) {
	/*
	 * Change callers protection back to normal if the pte is still valid.
	 * No need to waste our time otherwise.
	 */
	VmSetPageProt(virtAddrPtr, ptePtr, VM_URW_PROT);
    }
    virtAddrPtr->segPtr->numCOWPages--;
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
    register	Vm_Segment	*srcSegPtr;	/* Segment to take page from.*/
    int				virtPage;	/* Virtual page to give away.*/
    register	Vm_Segment	*destSegPtr;	/* Segment to give page to. */
    Boolean			others;		/* TRUE => other segments that
						 * are copy-on-write on 
						 * the destination segment. */
{
    VmVirtAddr	virtAddr;
    Vm_PTE	pte;

    LOCK_MONITOR;

    virtAddr.segPtr = srcSegPtr;
    virtAddr.page = virtPage;
    virtAddr.flags = 0;
    pte = VmGetPTE(&virtAddr);
    VmPageInvalidateInt(&virtAddr);
    virtAddr.segPtr = destSegPtr;
    if (!others) {
	pte.protection = VM_URW_PROT;
    }
    VmSetPTE(&virtAddr, pte, TRUE);
    VmPageSwitch(VmPhysToVirtPage(pte.pfNum), destSegPtr);

    UNLOCK_MONITOR;
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

    VmSetPTE(virtAddrPtr, pte, pte.resident);

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
