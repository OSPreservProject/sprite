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
#include "machine.h"

Boolean	vm_CanCOW = TRUE;

void		DoFork();
void		GiveAwayPage();
void		ReleaseCOW();
Boolean		COWStart();
Vm_Segment	*FindNewMasterSeg();
Boolean		IsResident();
void		COWEnd();
void		SetPTE();
void		CopyPage();
ReturnStatus	COR();
void		COW();
unsigned int	GetMasterPF();


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
    register	Vm_PTE	corPTE;
    Vm_VirtAddr		virtAddr;
    VmCOWInfo		*cowInfoPtr;

    LOCK_MONITOR;

    corPTE = VM_VIRT_RES_BIT | VM_COR_BIT | srcSegPtr->segNum;

    if (srcSegPtr->type == VM_HEAP) {
	virtPage = srcSegPtr->offset;
	lastPage = virtPage + srcSegPtr->numPages - 1;
	srcPTEPtr = srcSegPtr->ptPtr;
	destPTEPtr = destSegPtr->ptPtr;
    } else {
	virtPage = mach_LastUserStackPage - srcSegPtr->numPages + 1;
	lastPage = mach_LastUserStackPage;
	srcPTEPtr = VmGetPTEPtr(srcSegPtr, virtPage);
	destPTEPtr = VmGetPTEPtr(destSegPtr, virtPage);
    }

    virtAddr.segPtr = srcSegPtr;
    virtAddr.flags = 0;
    for (; virtPage <= lastPage;
	 virtPage++, VmIncPTEPtr(srcPTEPtr, 1), VmIncPTEPtr(destPTEPtr, 1)) {
	if (!(*srcPTEPtr & VM_VIRT_RES_BIT)) {
	    *destPTEPtr = 0;
	    continue;
	}
	while (*srcPTEPtr & VM_IN_PROGRESS_BIT) {
	    (void)Sync_Wait(&srcSegPtr->condition, FALSE);
	}
	if (*srcPTEPtr & VM_COW_BIT) {
	    /*
	     * This page is already copy-on-write.  Make child copy-on-ref
	     * off of the parent segment.
	     */
	    *destPTEPtr = corPTE;
	    numCORPages++;
	} else if (*srcPTEPtr & VM_COR_BIT) {
	    /*
	     * This page is already copy-on-reference.  Make the child be
	     * copy-on-ref on the same segment.
	     */
	    *destPTEPtr = *srcPTEPtr;
	    numCORPages++;
	} else if (*srcPTEPtr & (VM_PHYS_RES_BIT | VM_ON_SWAP_BIT)) {
	    /*
	     * Need to make the src copy-on-write and the dest copy-on-ref.
	     */
	    virtAddr.page = virtPage;
	    *srcPTEPtr |= VM_COW_BIT;
	    VmMach_SetPageProt(&virtAddr, *srcPTEPtr);
	    numCOWPages++;
	    *destPTEPtr = corPTE;
	    numCORPages++;
	} else {
	    /*
	     * Just a normal everyday pte (zerofill or load from FS).
	     */
	    *destPTEPtr = *srcPTEPtr;
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
ReturnStatus
VmCOWCopySeg(segPtr)
    register	Vm_Segment	*segPtr;
{
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;
    Vm_VirtAddr			virtAddr;
    int				firstPage;
    int				lastPage;
    ReturnStatus		status;

    if (segPtr->type == VM_STACK) {
	firstPage = mach_LastUserStackPage - segPtr->numPages + 1;
    } else {
	firstPage = segPtr->offset;
    }
    lastPage = firstPage + segPtr->numPages - 1;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(segPtr, &cowInfoPtr)) {
	return(SUCCESS);
    }
    virtAddr.segPtr = segPtr;
    virtAddr.flags = 0;
    for (virtAddr.page = firstPage, ptePtr = VmGetPTEPtr(segPtr, firstPage);
	 virtAddr.page <= lastPage;
	 virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	if (*ptePtr & VM_COW_BIT) {
	    COW(&virtAddr, ptePtr, IsResident(ptePtr), FALSE);
	} else if (*ptePtr & VM_COR_BIT) {
	    status = COR(&virtAddr, ptePtr);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
    }

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
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;
    Vm_VirtAddr			virtAddr;

    if (firstPage == -1) {
	/*
	 * Caller wants to invalidate all pages for this segment.  This
	 * is only done when the segment is deleted.
	 */
	if (segPtr->type == VM_STACK) {
	    firstPage = mach_LastUserStackPage - segPtr->numPages + 1;
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
	if (*ptePtr & VM_COW_BIT) {
	    virtAddr.page = firstPage;
	    COW(&virtAddr, ptePtr, IsResident(ptePtr), TRUE);
	} else if (*ptePtr & VM_COR_BIT) {
	    segPtr->numCORPages--;
	    if (segPtr->numCORPages < 0) {
		Sys_Panic(SYS_FATAL, "VmCOWDeleteFromSeg: numCORPages < 0\n");
	    }
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
	if ((*ptePtr & VM_COR_BIT) &&
	    Vm_GetPageFrame(*ptePtr) == segPtr->segNum) {
	    if (mastSegPtr != (Vm_Segment *)NIL) {
		*ptePtr &= ~VM_PAGE_FRAME_FIELD;
		*ptePtr |= mastSegPtr->segNum;
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

    if (*ptePtr & VM_PHYS_RES_BIT) {
	VmLockPageInt(Vm_GetPageFrame(*ptePtr));
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
	register Vm_Segment	*cowSegPtr;
	register Vm_PTE		*ptePtr;
	int			firstPage;
	int			lastPage;
	register int		i;
	/*
	 * Only one segment left.  Return this segment back to normal
	 * protection and clean up.
	 */
	*cowInfoPtrPtr = cowInfoPtr;
	cowSegPtr = (Vm_Segment *)List_First(&cowInfoPtr->cowList);
	cowSegPtr->cowInfoPtr = (VmCOWInfo *)NIL;
	if (cowSegPtr->type == VM_STACK) {
	    firstPage = mach_LastUserStackPage - cowSegPtr->numPages + 1;
	} else {
	    firstPage = cowSegPtr->offset;
	}
	lastPage = firstPage + cowSegPtr->numPages - 1;
	for (ptePtr = VmGetPTEPtr(cowSegPtr, firstPage),
		i = cowSegPtr->numPages;
	     i > 0;
	     i--, VmIncPTEPtr(ptePtr, 1)) {
	    *ptePtr &= ~(VM_COW_BIT | VM_COR_BIT);
	}
	VmMach_SetSegProt(cowSegPtr, firstPage, lastPage, TRUE);
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
    register	Vm_VirtAddr	*virtAddrPtr;
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
    if (!(*ptePtr & VM_COR_BIT)) {
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

Boolean	cowStop = FALSE;


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
    register	Vm_VirtAddr	*virtAddrPtr;
    register	Vm_PTE		*ptePtr;
{
    register	Vm_Segment	*mastSegPtr;
    unsigned	int		virtFrameNum;
    unsigned	int		mastVirtPF;
    ReturnStatus		status;

    if (cowStop && virtAddrPtr->page == 
	virtAddrPtr->segPtr->offset + virtAddrPtr->segPtr->numPages - 1) {
	Sys_Panic(SYS_FATAL, "COR stop\n");
    }
    mastSegPtr = VmGetSegPtr((int) (Vm_GetPageFrame(*ptePtr)));
    virtFrameNum = VmPageAllocate(virtAddrPtr, TRUE);
    mastVirtPF = GetMasterPF(mastSegPtr, virtAddrPtr->page);
    if (mastVirtPF != 0) {
	/*
	 * The page is resident in memory so copy it.
	 */
	CopyPage(mastVirtPF, virtFrameNum);
	VmUnlockPage(mastVirtPF);
    } else {
	/*
	 * Load the page off of swap space.
	 */
	Vm_VirtAddr	virtAddr;

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
    if (virtAddrPtr->segPtr->numCORPages < 0) {
	Sys_Panic(SYS_FATAL, "COR: numCORPages < 0\n");
    }
    virtAddrPtr->segPtr->resPages++;
    SetPTE(virtAddrPtr, (Vm_PTE)(VM_VIRT_RES_BIT | VM_PHYS_RES_BIT | 
			 VM_REFERENCED_BIT | VM_MODIFIED_BIT | virtFrameNum));
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
ENTRY unsigned int
GetMasterPF(mastSegPtr, virtPage)
    Vm_Segment	*mastSegPtr;
    int		virtPage;
{
    unsigned	int	pf;
    register	Vm_PTE	*mastPTEPtr;

    LOCK_MONITOR;

    mastPTEPtr = VmGetPTEPtr(mastSegPtr, virtPage);
    if (*mastPTEPtr & VM_PHYS_RES_BIT) {
	pf = Vm_GetPageFrame(*mastPTEPtr);
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
    register	Vm_VirtAddr	*virtAddrPtr;
{
    register	Vm_PTE		*ptePtr;
    VmCOWInfo			*cowInfoPtr;

    vmStat.numCOWFaults++;
    cowInfoPtr = (VmCOWInfo *)NIL;
    if (!COWStart(virtAddrPtr->segPtr, &cowInfoPtr)) {
	vmStat.quickCOWFaults++;
	return;
    }
    ptePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
    if (!(*ptePtr & VM_COW_BIT)) {
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
    register	Vm_VirtAddr	*virtAddrPtr;	/* Virtual address to copy.*/
    register	Vm_PTE		*ptePtr;	/* Pointer to the page table
						 * entry. */
    Boolean			isResident;	/* TRUE => The page is resident
						 *	   in memory and locked
						 *	   down. */
    Boolean			deletePage;	/* TRUE => Delete the page
						 *         after copying. */
{
    register	Vm_Segment	*mastSegPtr;
    Vm_VirtAddr			virtAddr;
    Boolean			others;
    unsigned int		virtFrameNum;
    Vm_PTE			pte;

    if (cowStop && virtAddrPtr->page == 
	virtAddrPtr->segPtr->offset + virtAddrPtr->segPtr->numPages - 1) {
	Sys_Panic(SYS_FATAL, "COW stop\n");
    }
    mastSegPtr = FindNewMasterSeg(virtAddrPtr->segPtr, virtAddrPtr->page,
				  &others);
    if (mastSegPtr != (Vm_Segment *)NIL) {
	mastSegPtr->numCORPages--;
	if (mastSegPtr->numCORPages < 0) {
	    Sys_Panic(SYS_FATAL, "COW: numCORPages < 0\n");
	}
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
		GiveAwayPage(virtAddrPtr->segPtr, virtAddrPtr->page, ptePtr,
			     mastSegPtr, others);
	    } else {
		/*
		 * Copy the page.
		 */
		virtFrameNum = VmPageAllocate(&virtAddr, TRUE);
		CopyPage(Vm_GetPageFrame(*ptePtr), virtFrameNum);
		pte = VM_VIRT_RES_BIT | VM_PHYS_RES_BIT | 
		      VM_REFERENCED_BIT | VM_MODIFIED_BIT | virtFrameNum;
		if (others) {
		    pte |= VM_COW_BIT;
		}
		mastSegPtr->resPages++;
		SetPTE(&virtAddr, pte);
		VmUnlockPage(virtFrameNum);
		VmUnlockPage(Vm_GetPageFrame(*ptePtr));
	    }
	} else {
	    /*
	     * The page is on swap space.
	     */
	    VmCopySwapPage(virtAddrPtr->segPtr, virtAddrPtr->page, mastSegPtr);
	    pte = VM_VIRT_RES_BIT | VM_ON_SWAP_BIT;
	    if (others) {
		pte |= VM_COW_BIT;
	    }
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
     * Change from copy-on-write back to normal if it has not already been
     * done by some other routine.
     */
    if (*ptePtr & VM_COW_BIT) {
	ReleaseCOW(ptePtr);
    }
    virtAddrPtr->segPtr->numCOWPages--;
    if (virtAddrPtr->segPtr->numCOWPages < 0) {
	Sys_Panic(SYS_FATAL, "COW: numCOWPages < 0\n");
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
GiveAwayPage(srcSegPtr, virtPage, srcPTEPtr, destSegPtr, others)
    register	Vm_Segment	*srcSegPtr;	/* Segment to take page from.*/
    int				virtPage;	/* Virtual page to give away.*/
    register	Vm_PTE		*srcPTEPtr;	/* PTE for the segment to take
						 * the page away from. */
    register	Vm_Segment	*destSegPtr;	/* Segment to give page to. */
    Boolean			others;		/* TRUE => other segments that
						 * are copy-on-write on 
						 * the destination segment. */
{
    Vm_VirtAddr		virtAddr;
    register	Vm_PTE	*destPTEPtr;
    unsigned	int	pageFrame;
    Boolean		referenced;
    Boolean		modified;

    LOCK_MONITOR;

    virtAddr.segPtr = srcSegPtr;
    virtAddr.page = virtPage;
    virtAddr.flags = 0;
    pageFrame = Vm_GetPageFrame(*srcPTEPtr);
    destPTEPtr = VmGetPTEPtr(destSegPtr, virtPage);
    *destPTEPtr = *srcPTEPtr & ~VM_COW_BIT;
    *srcPTEPtr = 0;
    VmMach_GetRefModBits(&virtAddr, pageFrame, &referenced, &modified);
    if (referenced) {
	*destPTEPtr |= VM_REFERENCED_BIT;
    }
    if (modified) {
	*destPTEPtr |= VM_MODIFIED_BIT;
    }
    VmMach_PageInvalidate(&virtAddr, pageFrame, FALSE);
    if (others) {
	*destPTEPtr |= VM_COW_BIT;
    }
    VmPageSwitch(Vm_GetPageFrame(*destPTEPtr), destSegPtr);

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
    Vm_VirtAddr	*virtAddrPtr;
    Vm_PTE	pte;
{
    Vm_PTE	*ptePtr;
    LOCK_MONITOR;

    ptePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
    *ptePtr = pte;

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
    unsigned	int	srcPF;
    unsigned	int	destPF;
{
    register	Address	srcMappedAddr;
    register	Address	destMappedAddr;

    srcMappedAddr = VmMapPage(srcPF);
    destMappedAddr = VmMapPage(destPF);
    Byte_Copy(vm_PageSize, srcMappedAddr, destMappedAddr);
    VmUnmapPage(srcMappedAddr);
    VmUnmapPage(destMappedAddr);
}


/*
 *----------------------------------------------------------------------
 *
 * ReleaseCOW --
 *
 *	Make the page no longer copy-on-write.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
ReleaseCOW(ptePtr)
    Vm_PTE	*ptePtr;
{
    LOCK_MONITOR;

    *ptePtr &= ~VM_COW_BIT;

    UNLOCK_MONITOR;
}
