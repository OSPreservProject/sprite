/* 
 * vmPage.c --
 *
 *      This file contains routines that manage the core map, allocate
 *      list, free list, dirty list and reserve page list.  The core map
 *	contains one entry for each page frame in physical memory.  The four
 *	lists run through the core map.  The dirty list contains all pages
 *	that are being written to disk.  Dirty pages are written out by
 *	a set of pageout processes.  Pages are put onto the dirty list by the
 *	page allocation routine.  The allocate list contains all pages that
 *	are being used by user processes and are not on the dirty list.  It is
 *	kept in approximate LRU order by a version of the clock algorithm. 
 *	The free list contains pages that aren't being used by any user
 *	processes or the kernel.  The reserve list is a few pages
 *	that are set aside for emergencies when the kernel needs memory but 
 *	all of memory is dirty.
 *
 *	LOCKING PAGES
 *
 *	In general all pages that are on the allocate page list are eligible 
 *	to be given to any process.  However, if a page needs to be locked down
 *	so that it cannot be taken away from its owner, there is a lock count
 *	field in the core map entry for a page frame to allow this.  As long
 *	as the lock count is greater than zero, the page cannot be taken away
 *	from its owner.
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
#include "vmInt.h"
#include "vm.h"
#include "user/vm.h"
#include "sync.h"
#include "dbg.h"
#include "list.h"
#include "machine.h"
#include "timer.h"
#include "lock.h"
#include "sys.h"
#include "byte.h"
#include "cvt.h"

Boolean	vmDebug	= FALSE;

/*
 * Variables global to this file.
 */
static	VmCore          *coreMap;	/* Pointer to core map that is 
					   allocated in VmCoreMapAlloc. */

/*
 * Minimum fraction of pages that VM wants for itself.  It keeps
 * 1 / MIN_VM_PAGE_FRACTION of the available pages at boot time for itself.
 */
#define	MIN_VM_PAGE_FRACTION	16

/*
 * Variables to define the number of page procs working at a time and the
 * maximum possible number that can be working at a time.
 */
static	int		numPageOutProcs = 0;
static	int		maxPageOutProcs = 3;

/*
 * Page lists.  There are four different lists and a page can be on at most
 * one list.  The allocate list is a list of in use pages that is kept in
 * LRU order. The dirty list is a list of in use pages that are being
 * written to swap space.  The free list is a list of pages that are not
 * being used by any process.  The reserve list is a list with
 * NUM_RESERVE_PAGES on it that is kept for the case when the kernel needs
 * new memory but all of memory is dirty.
 */
#define	NUM_RESERVE_PAGES	3
static	List_Links      allocPageListHdr;
static	List_Links      dirtyPageListHdr;
static	List_Links	freePageListHdr;
static	List_Links	reservePageListHdr;
#define allocPageList	(&allocPageListHdr)
#define dirtyPageList	(&dirtyPageListHdr)
#define	freePageList	(&freePageListHdr)
#define	reservePageList	(&reservePageListHdr)

/*
 * Variables used to instrument virtual memory.
 */
static	Boolean		forceRef = FALSE;
static	Boolean		ignoreDirt = FALSE;
static	Boolean		forceSwap = FALSE;

/*
 * Conditions to wait on.
 */
static	Sync_Condition	cleanCondition;		/* Used to wait for a
						 * clean page to be put
						 * onto the allocate list. */
static	Sync_Condition	pageinCondition;	/* Used to wait for page in
						   to complete. */
static	Sync_Condition	segmentCondition;	/* Used to wait for page out
						   daemon to clean a page
						   for a dieing segment. */

/*
 * Variables to allow recovery.
 */
static	Boolean		swapDown = FALSE;
static	Sync_Condition	swapDownCondition;

void	PageOut();
void	PutOnReserveList();
void	PutOnFreeList();


/*
 * ----------------------------------------------------------------------------
 *
 * VmCoreMapAlloc --
 *
 *     	Allocate space for the core map.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Core map allocated.
 * ----------------------------------------------------------------------------
 */
void
VmCoreMapAlloc()
{
    vmStat.numPhysPages = VmMachGetNumPages();
    Sys_Printf("Available memory %d\n", vmStat.numPhysPages * VM_PAGE_SIZE);
    coreMap = (VmCore *) Vm_BootAlloc(sizeof(VmCore) * vmStat.numPhysPages);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmCoreMapInit --
 *
 *     	Initialize the core map.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Core map initialized.
 * ----------------------------------------------------------------------------
 */
void
VmCoreMapInit()
{
    register	int	i;
    register	VmCore	*corePtr;

    /*   
     * Initialize the core allocate, dirty, free and reserve lists.
     */

    List_Init(allocPageList);
    List_Init(dirtyPageList);
    List_Init(freePageList);
    List_Init(reservePageList);

    /*
     * Initialize the core map.  All pages up to vmFirstFreePage are
     * owned by the kernel and the rest are free.  Note that vmFirstFreePage
     * is the first free physical page not the first free virtual page.
     */
    for (i = 0, corePtr = coreMap; i < vmFirstFreePage; i++, corePtr++) {
	corePtr->links.nextPtr = (List_Links *) NIL;
	corePtr->links.prevPtr = (List_Links *) NIL;
        corePtr->lockCount = 1;
        corePtr->flags = 0;
        corePtr->virtPage.segPtr = vmSysSegPtr;
        corePtr->virtPage.page = i + (MACH_KERNEL_START >> VM_PAGE_SHIFT);
    }
    /*
     * The first NUM_RESERVED_PAGES are put onto the reserve list.
     */
    for (i = vmFirstFreePage, vmStat.numReservePages = 0;
         vmStat.numReservePages < NUM_RESERVE_PAGES;
	 i++, corePtr++) {
	corePtr->links.nextPtr = (List_Links *) NIL;
	corePtr->links.prevPtr = (List_Links *) NIL;
	PutOnReserveList(corePtr);
    }
    /*
     * The remaining pages are put onto the free list.
     */
    for (vmStat.numFreePages = 0; i < vmStat.numPhysPages; i++, corePtr++) {
	corePtr->links.nextPtr = (List_Links *) NIL;
	corePtr->links.prevPtr = (List_Links *) NIL;
	PutOnFreeList(corePtr);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Routines to manage the four lists.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * PutOnAllocListFront --
 *
 *     	Put this core map entry onto the front of the allocate list.  If
 *	the reserve list is short of pages then this page will end up on
 *	the reserve list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Alloc or reserve lists modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnAllocListFront(corePtr)
    register	VmCore	*corePtr;
{
    if (vmStat.numReservePages < NUM_RESERVE_PAGES) {
	VmPageInvalidateInt(&(corePtr->virtPage));
	PutOnReserveList(corePtr);
    } else {
	VmListInsert((List_Links *) corePtr, LIST_ATFRONT(allocPageList));
	vmStat.numUserPages++;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutOnAllocListRear --
 *
 *     	Put this core map entry onto the rear of the allocate list
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Alloc list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnAllocListRear(corePtr)
    VmCore	*corePtr;
{
    VmListInsert((List_Links *) corePtr, LIST_ATREAR(allocPageList));
    vmStat.numUserPages++;
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutOnAllocList --
 *
 *     	Put the given core map entry onto the end of the allocate list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	Core map entry put onto end of allocate list.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY static void
PutOnAllocList(virtAddrPtr, page)
    VmVirtAddr	*virtAddrPtr;	/* The translated virtual address that 
				   indicates the segment and virtual page 
				   that this physical page is being allocated 
				   for */
    int		page;
{
    register	VmCore	*corePtr; 
    Time		curTime;

    Timer_GetTimeOfDay(&curTime, (int *) NIL, (Boolean *) NIL);

    corePtr = &coreMap[page];

    /*
     * Move the page to the end of the allocate list and initialize the core 
     * map entry.  If page is for a kernel process then don't put it onto
     * the end of the allocate list.
     */
    if (virtAddrPtr->segPtr != vmSysSegPtr) {
	PutOnAllocListRear(corePtr);
    }

    corePtr->virtPage = *virtAddrPtr;
    corePtr->flags = 0;
    corePtr->lockCount = 1;
    corePtr->lastRef = curTime.seconds;
}


/*
 * ----------------------------------------------------------------------------
 *
 * TakeOffAllocList --
 *
 *     	Take this core map entry off of the allocate list
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Alloc list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
TakeOffAllocList(corePtr)
    VmCore	*corePtr;
{
    VmListRemove((List_Links *) corePtr);
    vmStat.numUserPages--;
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutOnReserveList --
 *
 *     	Put this core map entry onto the reserve page list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Reserve list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnReserveList(corePtr)
    register	VmCore	*corePtr;
{
    corePtr->flags = 0;
    corePtr->lockCount = 1;
    VmListInsert((List_Links *) corePtr, LIST_ATREAR(reservePageList));
    Sys_Printf("Replenishing reserve list\n");
    vmStat.numReservePages++;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmGetReservePage --
 *
 *     	Take a core map entry off of the reserve list and return its 
 *	page frame number.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Reserve list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL int
VmGetReservePage(virtAddrPtr)
    VmVirtAddr	*virtAddrPtr;
{
    VmCore	*corePtr;

    if (List_IsEmpty(reservePageList)) {
	return(-1);
    }
    Sys_Printf("Taking from reserve list\n");
    vmStat.reservePagesUsed++;
    corePtr = (VmCore *) List_First(reservePageList);
    List_Remove((List_Links *) corePtr);
    vmStat.numReservePages--;
    corePtr->virtPage = *virtAddrPtr;

    return(corePtr - coreMap);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutOnFreeList --
 *
 *     	Put this core map entry onto the free list.  The page will actually
 *	end up on the reserve list if the reserve list needs more pages.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Free list or reserve list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnFreeList(corePtr)
    register	VmCore	*corePtr;
{
    if (vmStat.numReservePages < NUM_RESERVE_PAGES) {
	PutOnReserveList(corePtr);
    } else {
	corePtr->flags = VM_FREE_PAGE;
	corePtr->lockCount = 0;
	VmListInsert((List_Links *) corePtr, LIST_ATREAR(freePageList));
	vmStat.numFreePages++;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * TakeOffFreeList --
 *
 *     	Take this core map entry off of the free list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Free list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
TakeOffFreeList(corePtr)
    VmCore	*corePtr;
{
    VmListRemove((List_Links *) corePtr);
    vmStat.numFreePages--;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPutOnFreePageList --
 *
 *      Put the given page frame onto the free list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL void
VmPutOnFreePageList(pfNum)
    int	pfNum;		/* The page frame to be freed. */
{
    if (pfNum == 0) {
	/*
	 * Page frame number 0 is special because a page frame of 0 on a
	 * user page fault has special meaning.  Thus if the kernel decides
	 * to free page frame 0 then we can't make this page elgible for user
	 * use.  Instead of throwing it away put it onto the reserve list
	 * because only the kernel uses pages on the reserve list.
	 */
	PutOnReserveList(&coreMap[pfNum]);
    } else {
	PutOnFreeList(&coreMap[pfNum]);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * PutOnDirtyList --
 *
 *	Put the given core map entry onto the dirty list and wakeup the page
 *	out daemon.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	Page added to dirty list, number of dirty pages is incremented and 
 *	number of active page out processes may be incremented.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnDirtyList(corePtr)
    register	VmCore	*corePtr;
{
    vmStat.numDirtyPages++;
    VmListInsert((List_Links *) corePtr, LIST_ATREAR(dirtyPageList));
    corePtr->flags |= VM_DIRTY_PAGE;
    if (vmStat.numDirtyPages - numPageOutProcs > 0 &&
	numPageOutProcs < maxPageOutProcs) { 
	Proc_CallFunc(PageOut, (ClientData) numPageOutProcs, 0);
	numPageOutProcs++;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * TakeOffDirtyList --
 *
 *     	Take this core map entry off of the dirty list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Dirty list modified and core map entry modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
TakeOffDirtyList(corePtr)
    VmCore	*corePtr;
{
    VmListRemove((List_Links *) corePtr);
    vmStat.numDirtyPages--;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPutOnDirtyList --
 *
 *     	Put the given page onto the front of the dirty list.  It is assumed
 *	the page is currently on either the allocate list or the dirty list.
 *	In either case mark the page such that it will not get freed until
 *	it is written out.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The dirty list is modified.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmPutOnDirtyList(pfNum)
    int	pfNum;
{
    register	VmCore	*corePtr; 

    LOCK_MONITOR;

    corePtr = &(coreMap[pfNum]);
    if (!(corePtr->flags & VM_DIRTY_PAGE)) {
	TakeOffAllocList(corePtr);
	PutOnDirtyList(corePtr);
    }
    corePtr->flags |= VM_DONT_FREE_UNTIL_CLEAN;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Routines to lock and unlock pages.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * VmLockPageInt --
 *
 *     	Increment the lock count on a page.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The core map entry for the page has its lock count incremented.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL void
VmLockPageInt(pfNum)
    int		pfNum;
{
    coreMap[pfNum].lockCount++;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmUnlockPage --
 *
 *     	Decrement the lock count on a page.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The core map entry for the page has its lock count decremented.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmUnlockPage(pfNum)
    int		pfNum;
{
    LOCK_MONITOR;
    coreMap[pfNum].lockCount--;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 *	Routines to get reference times of VM pages.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * Vm_GetRefTim --
 *
 *     	Return the age of the LRU page (0 if is a free page).
 *
 * Results:
 *     	Age of LRU page (0 if there is a free page).
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY int
Vm_GetRefTime()
{
    register	VmCore	*corePtr; 
    int			refTime;

    LOCK_MONITOR;

    vmStat.fsAsked++;

    if (swapDown || (vmStat.numFreePages + vmStat.numUserPages + 
		     vmStat.numDirtyPages <= vmStat.minVMPages)) {
	/*
	 * We are already at or below the minimum amount of memory that
	 * we are guaranteed for our use so refuse to give any memory to
	 * the file system.
	 */
	UNLOCK_MONITOR;
	return((int) 0x7fffffff);
    }

    if (!List_IsEmpty(freePageList)) {
	vmStat.haveFreePage++;
	refTime = 0;
	if (vmDebug) {
	    Sys_Printf("Vm_GetRefTime: VM has free page\n");
	}
    } else {
	refTime = (int) 0x7fffffff;
	if (!List_IsEmpty(dirtyPageList)) {
	    corePtr = (VmCore *) List_First(dirtyPageList);
	    refTime = corePtr->lastRef;
	}
	if (!List_IsEmpty(allocPageList)) {
	    corePtr = (VmCore *) List_First(allocPageList);
	    if (corePtr->lastRef < refTime) {
		refTime = corePtr->lastRef;
	    }
	}
	if (vmDebug) {
	    Sys_Printf("Vm_GetRefTime: Reftime = %d\n", refTime);
	}
    }

    UNLOCK_MONITOR;

    return(refTime);
}

/*
 * ----------------------------------------------------------------------------
 *
 * GetRefTime --
 *
 *     	Return either the first free page on the allocate list or the
 *	last reference time of the first page on the list.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	First page removed from allocate list if one is free.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
GetRefTime(refTimePtr, pagePtr)
    register	int	*refTimePtr;
    int			*pagePtr;
{
    register	VmCore	*corePtr; 

    LOCK_MONITOR;

    if (!List_IsEmpty(freePageList)) {
	vmStat.gotFreePage++;
	corePtr = (VmCore *) List_First(freePageList);
	TakeOffFreeList(corePtr);
	*pagePtr = corePtr - coreMap;
    } else {
	*refTimePtr = (int) 0x7fffffff;
	if (!List_IsEmpty(dirtyPageList)) {
	    corePtr = (VmCore *) List_First(dirtyPageList);
	    *refTimePtr = corePtr->lastRef;
	}
	if (!List_IsEmpty(allocPageList)) {
	    corePtr = (VmCore *) List_First(allocPageList);
	    if (corePtr->lastRef < *refTimePtr) {
		*refTimePtr = corePtr->lastRef;
	    }
	}
	*pagePtr = -1;
    }

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 *	Routines to allocate pages.
 *
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 *
 * DoPageAllocate --
 *
 *     	Grab the monitor lock and call VmPageAllocate.
 *
 * Results:
 *     	The physical page number that is allocated.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static int
DoPageAllocate(virtAddrPtr, canBlock)
    VmVirtAddr	*virtAddrPtr;	/* The translated virtual address that 
				   indicates the segment and virtual page 
				   that this physical page is being allocated 
				   for */
    Boolean	canBlock;	/* TRUE if can block if hit enough consecutive
				   dirty pages. */
{
    int page;

    LOCK_MONITOR;

    while (swapDown) {
	(void)Sync_Wait(&swapDownCondition, FALSE);
    }
    page = VmPageAllocateInt(virtAddrPtr, canBlock);

    UNLOCK_MONITOR;
    return(page);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageAllocate --
 *
 *     	This routine will return the page frame number of the first free or
 *     	unreferenced, unmodified, unlocked page that it can find on the 
 *	allocate list.  Calls VmPageAllocateInt to do the work.
 *
 * Results:
 *     	The physical page number that is allocated.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */

int
VmPageAllocate(virtAddrPtr, canBlock)
    VmVirtAddr	*virtAddrPtr;	/* The translated virtual address that 
				   indicates the segment and virtual page 
				   that this physical page is being allocated 
				   for */
    Boolean	canBlock;	/* TRUE if can block if hit enough consecutive
				   dirty pages. */
{
    int			page;
    int			refTime;

    vmStat.numAllocs++;

    GetRefTime(&refTime, &page);
    if (page == -1) {
	Fs_GetPageFromFS(refTime, &page);
	if (page == -1) {
	    vmStat.pageAllocs++;
	    return(DoPageAllocate(virtAddrPtr, canBlock));
	} else {
	    vmStat.gotPageFromFS++;
	    if (vmDebug) {
		Sys_Printf("VmPageAllocate: Took page from FS (refTime = %d)\n",
			    refTime);
	    }
	}
    }

    /*
     * Move the page to the end of the allocate list and initialize the core 
     * map entry.
     */
    PutOnAllocList(virtAddrPtr, page);

    return(page);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageAllocateInt --
 *
 *     	This routine will return the page frame number of the first free or
 *     	unreferenced, unmodified, unlocked page that it can find on the 
 *	allocate list.  The core map entry for this page will be initialized to 
 *	contain the virtual page number and the lock count will be set to 
 *	1 to indicate that this page is locked down.
 *
 *	This routine will sleep if the entire allocate list is dirty in order
 *	to give the page-out daemon some time to clean pages.
 *
 * Results:
 *     	The physical page number that is allocated.
 *
 * Side effects:
 *     	The allocate list is modified and the  dirty list may be modified.
 *	In addition the appropriate core map entry is intialized.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL int
VmPageAllocateInt(virtAddrPtr, canBlock)
    VmVirtAddr	*virtAddrPtr;	/* The translated virtual address that 
				   indicates the segment and virtual page 
				   that this physical page is being allocated 
				   for */
    Boolean	canBlock;	/* TRUE if can block if hit enough consecutive
				   dirty pages. */
{
    register	VmCore	*corePtr; 
    Vm_PTE		pte;
    Time		curTime;
    List_Links		endMarker;
    int			pageCount;

    Timer_GetTimeOfDay(&curTime, (int *) NIL, (Boolean *) NIL);

    vmStat.numListSearches++;

again:

    if (!List_IsEmpty(freePageList)) {
	corePtr = (VmCore *) List_First(freePageList);
	TakeOffFreeList(corePtr);
	vmStat.usedFreePage++;
    } else {
	/*
	 * Put a marker at the end of the core map so that we can detect loops.
	 */
	endMarker.nextPtr = (List_Links *) NIL;
	endMarker.prevPtr = (List_Links *) NIL;
	VmListInsert(&endMarker, LIST_ATREAR(allocPageList));
	pageCount = 0;
    
	/*
	 * Loop examining the page on the front of the allocate list until 
	 * a free or unreferenced, unmodified, unlocked page frame is found.
	 * If the whole list is examined and nothing found, then return -1.
	 */
	while (TRUE) {
	    corePtr = (VmCore *) List_First(allocPageList);
	    pageCount++;
    
	    /*
	     * See if have gone all of the way through the list without finding
	     * anything.
	     */
	    if (corePtr == (VmCore *) &endMarker) {	
		VmListRemove((List_Links *) &endMarker);
		if (!canBlock) {
		    Sys_Printf("VmPageAllocateInt: All of memory is dirty (%d pages examined).\n", pageCount);
		    return(-1);
		} else {
		    /*
		     * There were no pages available.   This can only happen
		     * if all of memory is dirty.  Therefore wait for a clean
		     * page to appear on the allocate list.
		     */
		    Sync_Wait(&cleanCondition, FALSE);
		    goto again;
		}
	    }
    
	    /*
	     * Make sure that the page is not locked down.
	     */
	    if (corePtr->lockCount > 0) {
		vmStat.lockSearched++;
		VmListMove((List_Links *) corePtr, LIST_ATREAR(allocPageList));
		continue;
	    }
		
	    /*
	     * Now make sure that the page has not been referenced.  It it has
	     * then clear the reference bit and put it onto the end of the
	     * allocate list.
	     */
	    pte = VmGetPTE(&corePtr->virtPage);
	    if (pte.referenced) {
		vmStat.refSearched++;
		pte.referenced = 0;
		corePtr->lastRef = curTime.seconds;
		VmSetPTE(&corePtr->virtPage, pte);
		VmListMove((List_Links *) corePtr, LIST_ATREAR(allocPageList));
		/*
		 * Set the last page marker so that we will try to examine this
		 * page again if we go all the way around without finding 
		 * anything.  
		 *
		 * NOTE: This is only a uni-processor solution since
		 *       on a multi-processor a process could be continually 
		 *       touching pages while we are scanning the list.
		 */
		VmListMove(&endMarker, LIST_ATREAR(allocPageList));
		pageCount = 0;
		continue;
	    }
    
	    /*
	     * The page is available and it has not been referenced.  Now
	     * it must be determined if it is dirty.  If it is then put it onto
	     * the dirty list.
	     */
	    if (pte.modified || (forceSwap && !pte.onSwap)) {
		vmStat.dirtySearched++;
		TakeOffAllocList(corePtr);
		PutOnDirtyList(corePtr);
		continue;
	    }
    
	    /*
	     * We can take this page.  Invalidate the page for the old segment.
	     */
	    VmPageInvalidateInt(&(corePtr->virtPage));
	    TakeOffAllocList(corePtr);
	    VmListRemove(&endMarker);
	    break;
	}
    }

    /*
     * If this page is being allocated for the kernel segment then don't put
     * it back onto the allocate list because kernel pages don't exist on
     * the allocate list.  Otherwise move it to the rear of the allocate list.
     */
    if (virtAddrPtr->segPtr != vmSysSegPtr) {
	PutOnAllocListRear(corePtr);
    }
    corePtr->virtPage = *virtAddrPtr;
    corePtr->flags = 0;
    corePtr->lockCount = 1;
    corePtr->lastRef = curTime.seconds;

    return(corePtr - coreMap);
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmPageFreeInt --
 *
 *      This routine will put the given page frame onto the front of the
 *      free list if it is not on the dirty list.  If the page frame is on 
 *	the dirty list then this routine will sleep until the page has been
 *	cleaned.  The page-out daemon will put the page onto the front of the
 *	allocate list when it finishes cleaning the page.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The free list is modified and the core map entry is set to free
 *	with a lockcount of 0.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL void
VmPageFreeInt(pfNum)
    int	pfNum;		/* The page frame to be freed. */
{
    register	VmCore	*corePtr; 

    corePtr = &(coreMap[pfNum]);

    corePtr->flags |= VM_FREE_PAGE;
    corePtr->lockCount = 0;

    if (corePtr->virtPage.segPtr == vmSysSegPtr) {
        /*
	 * Pages given to the kernel are removed from the allocate list when
	 * they are allocated.  Therefore just put it back onto the free list.
	 */
	if (corePtr->flags & (VM_DIRTY_PAGE | VM_PAGE_BEING_CLEANED)) {
	    Sys_Panic(SYS_FATAL, "VmPageFreeInt: Kernel page on dirty list\n");
	}
	PutOnFreeList(corePtr);
    } else {
	/*
	 * If the page is being written then wait for it to finish.
	 * Once it has been cleaned it will automatically be put onto the free
	 * list.  We must wait for it to be cleaned because 
	 * the segment may die otherwise while the page is still waiting to be 
	 * cleaned.  This would be a disaster because the page-out daemon uses
	 * the segment table entry to determine where to write the page.
	 */
	if (corePtr->flags & 
			(VM_PAGE_BEING_CLEANED | VM_DONT_FREE_UNTIL_CLEAN)) {
	    do {
		corePtr->flags |= VM_SEG_PAGEOUT_WAIT;
		vmStat.cleanWait++;
		(void) Sync_Wait(&segmentCondition, FALSE);
	    } while (corePtr->flags & 
			(VM_PAGE_BEING_CLEANED | VM_DONT_FREE_UNTIL_CLEAN));
	} else {
	    if (corePtr->flags & VM_DIRTY_PAGE) {
		TakeOffDirtyList(corePtr);
	    } else {
		TakeOffAllocList(corePtr);
	    }
	    PutOnFreeList(corePtr);
	}
    }
}
	    

/*
 * ----------------------------------------------------------------------------
 *
 * VmPageFree --
 *
 *      Free the given page.  Call VmPageFreeInt to do the work.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmPageFree(pfNum)
    int	pfNum;		/* The page frame to be freed. */
{
    LOCK_MONITOR;

    VmPageFreeInt(pfNum);

    UNLOCK_MONITOR;
}
	    

/*-----------------------------------------------------------------------
 *
 * 		Routines to handle page faults.
 *
 * Page fault handling is divided into four routines.  The first
 * routine is Vm_PageIn which is the external entry point to the page-in 
 * process.  It determines which segment the faulting address falls into and
 * calls the internal page-in routine VmDoPageIn.  VmDoPageIn is split into
 * work it does at non-monitor level and at monitor level.  It first 
 * expands the segment if necessary by calling a hardware-dependent  monitored 
 * routine.  It then calls the monitored routine PreparePage which determines
 * if the page is already present.  If the page is not present, it calls
 * VmPageAllocate to actually allocate a physical page frame.  Next
 * VmDoPageIn fills the page at non-monitor level.  Finally it calls the 
 * monitored routine FinishPage which validates the page and cleans up state.
 */


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_PageIn --
 *
 *     This routine is called to read in the page at the given virtual address.
 *
 * Results:
 *     SUCCESS if the page in was successful and FAILURE otherwise.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Vm_PageIn(virtAddr)
    int virtAddr;	/* The virtual address of the desired page */

{
    VmVirtAddr	 	transVirtAddr;	
    ReturnStatus 	status;
    Proc_ControlBlock	*procPtr;
				
    vmStat.totalUserFaults++;

    /*
     * Determine which segment that this virtual address falls into.
     */
    VmVirtAddrParse(virtAddr, &transVirtAddr);

    if (transVirtAddr.segPtr == (Vm_Segment *) NIL) {
	return(FAILURE);
    }

    /*
     * Actually page in the page and don't leave the page locked down.
     */
    status = VmDoPageIn(FALSE, &transVirtAddr);

    /*
     * If the segment that was faulted in was a stack or heap segment then the
     * heap segment was prevented from being expanded.  Let it be expanded
     * now.
     */
    if (transVirtAddr.segPtr->type != VM_CODE) {
	procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
	VmDecExpandCount(procPtr->vmPtr->segPtrArray[VM_HEAP]);
    }

    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PreparePage --
 *
 *	This routine performs the first half of the page-in process.
 *	If the desired page is either already in the process of being
 *	faulted in, then this routine will return as soon as the page-in
 *	has finished.  If the desired page is already in memory then
 *	it will validate the page and return immediately.  Otherwise
 *	it will allocate a page frame and return the page frame in the given
 *	page table entry.
 *
 * Results:
 *	If a page had to be allocated then it returns FALSE in *donePtr.
 *	Otherwise it returns TRUE.
 *
 * Side effects:
 *	If the lockPage flag is set then the core map entry is marked as 
 *	locked.  In addition if a page had to be allocated then the page
 *	frame is marked as page-in in progress.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void static
PreparePage(lockPage, virtAddrPtr, ptePtrPtr, donePtr)
    Boolean	lockPage;		/* Set if the page should be locked 
					   down. */
    register VmVirtAddr *virtAddrPtr; 	/* The translated virtual address */
    Vm_PTE	**ptePtrPtr;		/* The page table entry for the virtual 
					   address */
    Boolean	*donePtr;		/* A flag that is set to true if this
					   routine has completed the page-in
					   process */
{
    register	Vm_PTE	*curPtePtr;

    LOCK_MONITOR;

    curPtePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
again:

    if (curPtePtr->inProgress) {
	/*
	 * The page is being faulted on by someone else.  In this case wait
	 * for the page fault to complete.  When wakeup have to get the new
	 * pte and see how things have changed.
	 */
	vmStat.collFaults++;
	(void) Sync_Wait(&pageinCondition, FALSE);
	goto again;
    } else if (curPtePtr->pfNum != 0) {
	/*
	 * The page must already be in memory.
	 */
	vmStat.quickFaults++;
	VmPageValidateInt(virtAddrPtr);
	if (lockPage) {
	    coreMap[curPtePtr->pfNum].lockCount++;
	}
        *donePtr = TRUE;
    } else {
	curPtePtr->inProgress = 1;
	*donePtr = FALSE;
    }
    *ptePtrPtr = curPtePtr;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * FinishPage --
 *	This routine finishes the page-in process.  First the page is 
 *	validated.  Next if the lock page flag is not set, the lock count is 
 *	decremented.  Finally the page-in in progress is cleared and all 
 *	other processes waiting for the page-in to complete are awakened.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page-in in progress cleared and lockcount may be decremented in the
 * 	core map entry.
 *	
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
FinishPage(lockPage, transVirtAddrPtr, ptePtr) 
    Boolean	lockPage;
    VmVirtAddr	*transVirtAddrPtr;
    Vm_PTE	*ptePtr;
{
    LOCK_MONITOR;

    /*
     * Make the page accessible.
     */
    VmPageValidateInt(transVirtAddrPtr);

    /*
     * Clear the zero fill bit here since we just filled the page so if it
     * was zero fill it can't be anymore.
     */
    ptePtr->zeroFill = 0;

    if (!lockPage) {
	coreMap[VmPhysToVirtPage(ptePtr->pfNum)].lockCount--;
    }

    /*
     * Wakeup processes waiting for this pagein to complete.
     */
    ptePtr->inProgress = 0;
    Sync_Broadcast(&pageinCondition);

    UNLOCK_MONITOR;
}

void	KillSharers();


/*
 *----------------------------------------------------------------------
 *
 * VmDoPageIn --
 *
 *	Actually perform the page-in for the given parsed virtual address.
 *	It is assumed that if this page fault is for a heap or stack segment
 *	that the heap segment of the process can not grow while the page
 *	fault is being handled.  This assures that the parsed virtual address
 *	stored in *transVirtAddrPtr will remain correct.
 *
 * Results:
 *	SUCCESS if the page could be paged in and FAILURE if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
VmDoPageIn(lockPage, virtAddrPtr)
    Boolean	lockPage;	/* Set if the page should be left
				   locked down. */
    VmVirtAddr	*virtAddrPtr;	/* The virtual address of the page
					   that needs to be read in. */
{
    register	Vm_PTE 	*ptePtr;
    Vm_PTE 		*tPtePtr;
    Boolean		done;
    ReturnStatus	status;
    int			lastPage;
    int			virtFrameNum;


    vmStat.totalFaults++;

    /*
     * If the address did not fall into a segment, then the page-in fails.
     */
    if (virtAddrPtr->segPtr == (Vm_Segment *) NIL) {
	return(FAILURE);
    }

    /*
     * Make sure that the virtual address is within the allocated part of the
     * segment.  If not, then either return error if heap or code segment,
     * or automatically expand the stack if stack segment.
     */
    if (!VmCheckBounds(virtAddrPtr)) {

	if (virtAddrPtr->segPtr->type == VM_STACK) {

	    /*
	     * If this is a stack segment, then automatically grow it.
	     */
	    lastPage = MACH_LAST_USER_STACK_PAGE - 
					virtAddrPtr->segPtr->numPages;
	    status = VmAddToSeg(virtAddrPtr->segPtr, virtAddrPtr->page, 
				lastPage);
	    if (status != SUCCESS) {
		return(status);
	    }
	} else {
	    return(FAILURE);
	}
    }

    switch (virtAddrPtr->segPtr->type) {
	case VM_CODE:
	    vmStat.codeFaults++;
	    break;
	case VM_HEAP:
	    vmStat.heapFaults++;
	    break;
	case VM_STACK:
	    vmStat.stackFaults++;
	    break;
    }

    /*
     * Do the first part of the page-in.  If PreparePage finished things, then
     * return.
     */
    PreparePage(lockPage, virtAddrPtr, &tPtePtr, &done);

    if (done) {
	return(SUCCESS);
    }
    ptePtr = tPtePtr;

    /*
     * Allocate a page.
     */
    virtFrameNum = VmPageAllocate(virtAddrPtr, TRUE);
    ptePtr->pfNum = VmVirtToPhysPage(virtFrameNum);

    /*
     * Call the appropriate routine to fill the page.
     */
    if (ptePtr->zeroFill) {
	vmStat.zeroFilled++;
	VmZeroPage((int) virtFrameNum);
	VmSetModBit(ptePtr);
	status = SUCCESS;
    } else if (ptePtr->onSwap) {
	vmStat.psFilled++;
	status = VmPageServerRead(virtAddrPtr, (int) virtFrameNum);
    } else {
	vmStat.fsFilled++;
	status = VmFileServerRead(virtAddrPtr, (int) virtFrameNum);
    }

    VmSetRefBit(ptePtr);
    
    /*
     * Finish up the page-in process.
     */
    FinishPage(lockPage, virtAddrPtr, ptePtr);

    /*
     * Now check to see if the read suceeded.  If not destroy all processes
     * that are sharing the code segment.
     */
    if (status != SUCCESS) {
	KillSharers(virtAddrPtr->segPtr);
    }

    return(status);
}


/*-----------------------------------------------------------------------
 * 			Routines for writing out dirty pages		
 *
 * Dirty pages are written to the swap file by the function PageOut.
 * PageOut is called by using the Proc_CallFunc routine which invokes
 * a process on PageOut.  When a page is put onto the dirty list a new
 * incantation of PageOut will be created unless there are already
 * more than maxPageOutProcs already writing out the dirty list.  Thus the
 * dirty list will be cleaned by at most maxPageOutProcs working in parallel.
 *
 * The work done by PageOut is split into work done at non-monitor level and
 * monitor level.  It calls the monitored routine PageOutPutAndGet to get the 
 * next page off of the dirty list.  It then writes the page out to the 
 * file server at non-monitor level.  Next it calls the monitored routine 
 * PageOutPutAndGet to put the page onto the front of the allocate list and
 * get the next dirty page.  Finally when there are no more pages to clean it
 * returns (and dies).
 */


/*
 * ----------------------------------------------------------------------------
 *
 * PutOnFront --
 *
 *	Take one of two actions.  If page frame is already marked as free
 *	then put it onto the front of the free list.  Otherwise put it onto
 *	the front of the allocate list.  
 *
 * Results:
 *	None.	
 *
 * Side effects:
 *	Allocate list or free list modified.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL static void
PutOnFront(corePtr)
    register	VmCore	*corePtr;
{
    if (corePtr->flags & VM_SEG_PAGEOUT_WAIT) {
	Sync_Broadcast(&segmentCondition);
    }
    corePtr->flags &= ~(VM_DIRTY_PAGE | VM_PAGE_BEING_CLEANED | 
		        VM_SEG_PAGEOUT_WAIT | VM_DONT_FREE_UNTIL_CLEAN);
    if (corePtr->flags & VM_FREE_PAGE) {
	PutOnFreeList(corePtr);
    } else {
	PutOnAllocListFront(corePtr);
    }
    vmStat.numDirtyPages--; 
    Sync_Broadcast(&cleanCondition);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PageOutPutAndGet --
 *
 *	This routine does two things.  First it puts the page pointed to by
 *	*corePtrPtr (if any) onto the front of the allocate list and wakes
 *	up any dieing processes waiting for this page to be cleaned.
 *	It then takes the first page off of the dirty list and returns a 
 *	pointer to it.  Before returning the pointer it clears the 
 *      modified bit of the page frame.
 *
 * Results:
 *     A pointer to the first page on the dirty list.  If there are no pages
 *     then *corePtrPtr is set to NIL.
 *
 * Side effects:
 *	The dirty list and allocate lists may both be modified.  In addition
 *      the onSwap bit is set to indicate that the page is now on swap space.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
PageOutPutAndGet(corePtrPtr, status, recStreamPtrPtr)
    VmCore	 **corePtrPtr;		/* On input points to page frame
					 * to be put back onto allocate list.
					 * On output points to page frame
					 * to be cleaned. */
    ReturnStatus status;		/* Status from the write. */
    Fs_Stream	 **recStreamPtrPtr;	/* Pointer to stream to do recovery
					 * on if necessary.  A NIL stream
					 * pointer is returned if no recovery
					 * is necessary. */
{
    Vm_PTE		pte;
    register	VmCore	*corePtr;

    LOCK_MONITOR;

    *recStreamPtrPtr = (Fs_Stream *)NIL;
    corePtr = *corePtrPtr;
    if (corePtr == (VmCore *)NIL) {
	if (swapDown) {
	    numPageOutProcs--;
	    UNLOCK_MONITOR;
	    return;
	}
    } else {
	switch (status) {
	    case RPC_TIMEOUT:
	    case FS_STALE_HANDLE:
		if (vmSwapStreamPtr != (Fs_Stream *)NIL) {
		    if (!swapDown) {
			/*
			 * We have not realized that we have an error yet.
			 * Determine which stream pointer caused the error
			 * (the segments swap stream if the swap file is 
			 * already open or the swap directory stream
			 * otherwise) and mark the swap as down.
			 */
			if (corePtr->virtPage.segPtr->swapFilePtr != 
							    (Fs_Stream *)NIL) {
			    *recStreamPtrPtr =
					corePtr->virtPage.segPtr->swapFilePtr;
			} else {
			    *recStreamPtrPtr = vmSwapStreamPtr;
			}
			swapDown = TRUE;
		    }
		    corePtr->flags &= ~VM_PAGE_BEING_CLEANED;
		    VmListInsert((List_Links *)corePtr,
				 LIST_ATREAR(dirtyPageList));
		    *corePtrPtr = (VmCore *)NIL;
		    numPageOutProcs--;
		    UNLOCK_MONITOR;
		    return;
		}
		break;
	    default:
		break;
	}
	PutOnFront(corePtr);
	corePtr = (VmCore *) NIL;	
    }

    while (!List_IsEmpty(dirtyPageList)) {
        /*
	 * Get the first page off of the dirty list.
	 */
	corePtr = (VmCore *) List_First(dirtyPageList);
	VmListRemove((List_Links *) corePtr);
	/*
	 * If this segment is being deleted then invalidate the page and
	 * then free it.
	 */
        if (corePtr->virtPage.segPtr->flags & VM_SEG_DEAD) {
	    vmStat.numDirtyPages--;
	    VmPageInvalidateInt(&corePtr->virtPage);
	    PutOnFreeList(corePtr);
	    corePtr = (VmCore *) NIL;
	} else {
	    break;
	}
    }

    if (corePtr != (VmCore *) NIL) {
	/*
	 * This page will now be on the page server so set the pte accordingly.
	 * In addition the modified bit must be cleared here since the page
	 * could get modified while it is being cleaned.
	 */
	pte = VmGetPTE(&corePtr->virtPage);
	pte.onSwap = 1;
	pte.modified = 0;
	VmSetPTE(&corePtr->virtPage, pte);
	corePtr->flags |= VM_PAGE_BEING_CLEANED;
    } else {
	/*
	 * No dirty pages.  Decrement the number of page out procs and
	 * return nil.  PageOut will kill itself when it receives NIL.
	 */
	numPageOutProcs--;

	if (numPageOutProcs == 0 && vmStat.numDirtyPages != 0) {
	    Sys_Panic(SYS_FATAL, "PageOutPutAndGet: Dirty pages but no pageout procs\n");
	}
    }

    *corePtrPtr = corePtr;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * KillSharers --
 *
 *	Go down the list of processes sharing this segment and send a
 *	kill signal to each one.  This is called when a page from a segment
 *	couldn't be written to swap space
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All processes sharing this segment are destroyed.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY static void
KillSharers(segPtr) 
    register	Vm_Segment	*segPtr;
{
    register	VmProcLink	*procLinkPtr;

    LOCK_MONITOR;

    LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	Sig_SendProc(procLinkPtr->procPtr, SIG_KILL, PROC_VM_READ_ERROR); 
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * PageOut --
 *
 *	Function to write out pages on dirty list.  It will keep retrieving
 *	pages from the dirty list until there are no more left.  This function
 *	is designed to be called through Proc_CallFunc.
 *	
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The dirty list is emptied.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PageOut(data, callInfoPtr)
    ClientData		data;		/* Ignored. */
    Proc_CallInfo	*callInfoPtr;	/* Ignored. */
{
    VmCore		*corePtr;
    ReturnStatus	status = SUCCESS;
    Fs_Stream		*recStreamPtr;

    vmStat.pageoutWakeup++;

    corePtr = (VmCore *) NIL;
    while (TRUE) {
	PageOutPutAndGet(&corePtr, status, &recStreamPtr);
	if (recStreamPtr != (Fs_Stream  *)NIL) {
	    (void) Fs_WaitForHost(recStreamPtr,
				  FS_NAME_SERVER | FS_NON_BLOCKING, status);
	}

	if (corePtr == (VmCore *) NIL) {
	    break;
	}
	status = VmPageServerWrite(&corePtr->virtPage, corePtr - coreMap);
	if (status != SUCCESS) {
	    if (vmSwapStreamPtr == (Fs_Stream *)NIL ||
	        (status != RPC_TIMEOUT && status != FS_STALE_HANDLE)) {
		/*
		 * Non-recoverable error on page write, so kill all users of 
		 * this segment.
		 */
		KillSharers(corePtr->virtPage.segPtr);
	    }
	}
    }

}


/*
 * Variables for the clock daemon.  pagesToCheck is the number of page 
 * frames to examine each time that the clock daemon wakes up.  clockSleep
 * is the amount of time for the clock daemon before it runs again.
 */
static	unsigned int	clockSleep;		
static	int		pagesToCheck = 100;
static	int		clockHand = 0;/* The hand of the clock */

/*
 * ----------------------------------------------------------------------------
 *
 * Vm_Clock --
 *
 *	Main loop for the clock daemon process.  It will wakeup every 
 *	few seconds, examine a few page frames, and then go back to sleep.
 *	It is used to keep the allocate list in approximate LRU order.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The allocate and dirty lists are modified.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
Vm_Clock(data, callInfoPtr)
	ClientData	data;
	Proc_CallInfo	*callInfoPtr;
{
    static Boolean initialized = FALSE;

    VmCore	*corePtr;	/* Pointer to the page in the core table
				   that is currently being examined */
    Vm_PTE	pte;		/* The pte for the page being 
				   examined */
    int		i;
    VmVirtAddr	virtAddr;	/* The virtual address structure for
				   the page being examined */
    Time	curTime;

    LOCK_MONITOR;

    Timer_GetTimeOfDay(&curTime, (int *) NIL, (Boolean *) NIL);

    /*
     * Examine pagesToCheck pages.
     */

    for (i = 0; i < pagesToCheck; i++) {
	corePtr = &(coreMap[clockHand]);

	/*
	 * Move to the next page in the core map.  If have reached the
	 * end of the core map then go back to the first page that may not
	 * be used by the kernel.
	 */
	if (clockHand == vmStat.numPhysPages - 1) {
	    clockHand = vmFirstFreePage;
	} else {
	    clockHand++;
	}

	/*
	 * If the page is free, locked, in the middle of a page-in, 
	 * or in the middle of a pageout, then we we aren't concerned 
	 * with this page.
	 */
	if ((corePtr->flags & (VM_DIRTY_PAGE | VM_FREE_PAGE)) ||
	    corePtr->lockCount > 0) {
	    continue;
	}
	    
	virtAddr = corePtr->virtPage;

	pte = VmGetPTE(&virtAddr);

	/*
	 * If the page has been referenced, then put it on the end of the
	 * allocate list.
	 *
	 * NOTE: forceRef is for instrumenting the virtual memory system.
	 */
	if (forceRef || pte.referenced) {
	    VmListMove((List_Links *) corePtr, LIST_ATREAR(allocPageList));
	    pte.referenced = 0;
	    corePtr->lastRef = curTime.seconds;
	    VmSetPTE(&virtAddr, pte);
	}
    }

    if (!initialized) {
        clockSleep = timer_IntOneSecond;
	initialized = TRUE;
    }

    callInfoPtr->interval = clockSleep;

    UNLOCK_MONITOR;
}

static int	copySize = 4096;
static char	buffer[8192];


/*
 *----------------------------------------------------------------------
 *
 * Vm_Cmd --
 *
 *      This routine allows a user level program to give commands to
 *      the virtual memory system.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Some parameter of the virtual memory system will be modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Vm_Cmd(command, arg)
    Vm_Command  command;
    int         arg;
{
    int			numBytes;
    ReturnStatus	status = SUCCESS;
 
    switch (command) {
	case VM_SET_PAGEOUT_PROCS:
	    maxPageOutProcs = arg;
	    break;
        case VM_SET_CLOCK_PAGES:
            pagesToCheck = arg;
            break;
        case VM_SET_CLOCK_INTERVAL:
	    clockSleep = arg * timer_IntOneSecond;
            break;
        case VM_FORCE_REF:
            forceRef = arg;
            break;
	case VM_FORCE_SWAP:
	    forceSwap = arg;
	    break;
        case VM_IGNORE_DIRT:
            ignoreDirt = arg;
            break;
	case VM_SET_COPY_SIZE:
	    copySize = arg;
	    break;
	case VM_DO_COPY_IN:
	    Vm_CopyIn(copySize, (Address) arg, buffer);
	    break;
	case VM_DO_COPY_OUT:
	    Vm_CopyOut(copySize, buffer, (Address) arg);
	    break;
	case VM_DO_MAKE_ACCESS_IN:
	    Vm_MakeAccessible(0, copySize, (Address) arg, &numBytes,
			      (Address *) &arg);
	    Byte_Copy(copySize, (Address) arg, buffer);
	    Vm_MakeUnaccessible((Address) arg, numBytes);
	    break;
	case VM_DO_MAKE_ACCESS_OUT:
	    Vm_MakeAccessible(0, copySize, (Address) arg, &numBytes,
			      (Address *) &arg);
	    Byte_Copy(copySize, buffer, (Address) arg);
	    Vm_MakeUnaccessible((Address) arg, numBytes);
	    break;
	case VM_GET_STATS:
	    vmStat.kernMemPages = 
	    		((int) vmMemEnd - MACH_KERNEL_START) / VM_PAGE_SIZE;
	    if (Vm_CopyOut(sizeof(Vm_Stat), (Address) &vmStat, 
			   (Address) arg) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	    break;
        default:
            Sys_Panic(SYS_WARNING, "Vm_Cmd: Unknown command.\n");
            break;
    }
 
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetPageSize --
 *
 *      Return the page size.
 *
 * Results:
 *      The page size.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Vm_GetPageSize()
{
    return(VM_PAGE_SIZE);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_MapBlock --
 *
 *      Allocate and validate enough pages at the given address to map
 *	one fs cache block.
 *
 * Results:
 *      The number of pages that were allocated.
 *
 * Side effects:
 *      Pages added to kernels address space.
 *
 *----------------------------------------------------------------------
 */
int
Vm_MapBlock(addr)
    Address	addr;	/* Address where to map in pages. */
{
    register	Vm_PTE	*ptePtr;
    VmVirtAddr		virtAddr;
    int			page;

    vmStat.fsMap++;

    virtAddr.page = (unsigned int) addr >> VM_PAGE_SHIFT;
    virtAddr.offset = 0;
    virtAddr.segPtr = vmSysSegPtr;
    ptePtr = VmGetPTEPtr(vmSysSegPtr, virtAddr.page);

    /*
     * Allocate a block.  We know that the page size is not smaller than
     * the block size so that one page will suffice.
     */
    ptePtr->protection = VM_KRW_PROT;
    page = DoPageAllocate(&virtAddr, FALSE);
    if (page == -1) {
	/*
	 * Couldn't get any memory.  
	 */
	return(0);
    }
    ptePtr->pfNum = VmVirtToPhysPage(page);
    VmPageValidate(&virtAddr);

    return(1);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_UnmapBlock --
 *
 *      Free and invalidate enough pages at the given address to unmap
 *	one fs cache block.
 *
 * Results:
 *      The number of pages that were deallocated.
 *
 * Side effects:
 *      Pages removed from kernels address space.
 *
 *----------------------------------------------------------------------
 */
int
Vm_UnmapBlock(addr, retOnePage, pageNumPtr)
    Address	addr;		/* Address where to map in pages. */
    Boolean	retOnePage;	/* TRUE => don't put one of the pages on
				 * the free list and return its value in
				 * *pageNumPtr. */
    int		*pageNumPtr;	/* One of the pages that was unmapped. */
{
    register	Vm_PTE	*ptePtr;
    VmVirtAddr		virtAddr;

    vmStat.fsUnmap++;

    virtAddr.page = (unsigned int) addr >> VM_PAGE_SHIFT;
    virtAddr.offset = 0;
    virtAddr.segPtr = vmSysSegPtr;
    ptePtr = VmGetPTEPtr(vmSysSegPtr, virtAddr.page);

    if (retOnePage) {
	*pageNumPtr = (int) VmPhysToVirtPage(ptePtr->pfNum);
    } else {
	/*
	 * If we aren't supposed to return the page, then free it.
	 */
	VmPageFree((int) VmPhysToVirtPage(ptePtr->pfNum));
    }
    VmPageInvalidate(&virtAddr);

    return(1);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_FsCacheSize --
 *
 *	Return the virtual addresses of the start and end of the file systems
 *	cache.
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
Vm_FsCacheSize(startAddrPtr, endAddrPtr)
    Address	*startAddrPtr;	/* Lowest virtual address. */
    Address	*endAddrPtr;	/* Highest virtual address. */
{
    int	numPages;

    /*
     * Compute the minimum number of pages that are reserved for VM.  The number
     * of free pages is the maximum number of pages that will ever exist
     * for user processes.
     */
    vmStat.minVMPages = vmStat.numFreePages / MIN_VM_PAGE_FRACTION;

    *startAddrPtr = (Address) VM_BLOCK_CACHE_BASE;
    /*
     * We aren't going to get any more free pages so limit the maximum number
     * of blocks in the cache to the number of free pages that we have minus
     * the minimum amount of free pages that we keep for user
     * processes to run.
     */
    numPages = (VM_BLOCK_CACHE_END - VM_BLOCK_CACHE_BASE) / VM_PAGE_SIZE;
    if (numPages > vmStat.numFreePages - vmStat.minVMPages) {
	numPages = vmStat.numFreePages - vmStat.minVMPages;
    }
    Sys_Printf("%d virtual pages for cache\n", numPages);
    *endAddrPtr = (Address) (VM_BLOCK_CACHE_BASE + numPages * VM_PAGE_SIZE - 1);
}

/*---------------------------------------------------------------------------
 * 
 *	Routines for recovery
 *
 * VM needs to be able to recover when the server of swap crashes.  This is
 * done in the following manner:
 *
 *    1) At boot time the directory "/swap/host_number" is open by the
 *	 routine Vm_OpenSwapDirectory and the stream is stored in 
 *	 vmSwapStreamPtr.
 *    2) If an error occurs on a page write then the variable swapDown
 *	 is set to TRUE which prohibits all further actions that would dirty
 *	 physical memory pages (e.g. page faults) and prohibits dirty pages
 *	 from being written to swap.
 *    3) Next the routine Fs_WaitForHost is called to asynchronously wait
 *	 for the server to come up.  When it detects that the server is
 *	 in fact up and the file system is alive, it calls Vm_Recovery.
 *    4) Vm_Recovery when called will set swapDown to FALSE and start cleaning
 *	 dirty pages if necessary.
 */


/*
 *----------------------------------------------------------------------
 *
 * Vm_Recovery --
 *
 *	The swap area has just come back up.  Wake up anyone waiting for it to
 *	come back and start up page cleaners if there are dirty pages to be
 *	written out.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	swapDown flag set to FALSE.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Vm_Recovery()
{
    LOCK_MONITOR;

    swapDown = FALSE;
    Sync_Broadcast(&swapDownCondition);
    while (vmStat.numDirtyPages - numPageOutProcs > 0 &&
	   numPageOutProcs < maxPageOutProcs) { 
	Proc_CallFunc(PageOut, (ClientData) numPageOutProcs, 0);
	numPageOutProcs++;
    }

    UNLOCK_MONITOR;
}


