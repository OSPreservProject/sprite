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
#include "vmMach.h"
#include "vm.h"
#include "vmInt.h"
#include "vmTrace.h"
#include "user/vm.h"
#include "sync.h"
#include "dbg.h"
#include "list.h"
#include "timer.h"
#include "lock.h"
#include "sys.h"
#include "byte.h"
#include "cvt.h"

Boolean	vmDebug	= FALSE;

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
static	int	numPageOutProcs = 0;
int		vmMaxPageOutProcs = 3;

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
 * Condition to wait for a clean page to be put onto the allocate list.
 */
static	Sync_Condition	cleanCondition;	

/*
 * Variables to allow recovery.
 */
static	Boolean		swapDown = FALSE;
static	Sync_Condition	swapDownCondition;

/*
 * Maximum amount of pages that can be on the dirty list before waiting for
 * a page to be cleaned.  It is a function of the amount of free memory at
 * boot time.
 */
#define	MAX_DIRTY_PAGE_FRACTION	4
int	vmMaxDirtyPages;

Boolean	vmFreeWhenClean = TRUE;	
Boolean	vmAlwaysRefuse = FALSE;	
Boolean	vmAlwaysSayYes = FALSE;	

int	vmFSPenalty = 0;
int	vmNumPageGroups = 10;
int	vmPagesPerGroup;
int	vmCurPenalty;
int	vmBoundary;
Boolean	vmCORReadOnly = FALSE;

/*
 * Limit to put on the number of pages the machine can have.  Used for
 * benchmarking purposes only.
 */
int	vmPhysPageLimit = -1;

void		PageOut();
void		PutOnReserveList();
void		PutOnFreeList();


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
    if (vmPhysPageLimit > 0 && vmPhysPageLimit < vmStat.numPhysPages) {
	vmStat.numPhysPages = vmPhysPageLimit;
    }
    Sys_Printf("Available memory %d\n", vmStat.numPhysPages * vm_PageSize);
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
    int			firstKernPage;

    /*   
     * Initialize the allocate, dirty, free and reserve lists.
     */
    List_Init(allocPageList);
    List_Init(dirtyPageList);
    List_Init(freePageList);
    List_Init(reservePageList);

    firstKernPage = (unsigned int)mach_KernStart >> vmPageShift;
    /*
     * Initialize the core map.  All pages up to vmFirstFreePage are
     * owned by the kernel and the rest are free.
     */
    for (i = 0, corePtr = coreMap; i < vmFirstFreePage; i++, corePtr++) {
	corePtr->links.nextPtr = (List_Links *) NIL;
	corePtr->links.prevPtr = (List_Links *) NIL;
        corePtr->lockCount = 1;
	corePtr->wireCount = 0;
        corePtr->flags = 0;
        corePtr->virtPage.segPtr = vm_SysSegPtr;
        corePtr->virtPage.page = i + firstKernPage;
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
	VmPageInvalidateInt(&(corePtr->virtPage),
	    VmGetPTEPtr(corePtr->virtPage.segPtr, corePtr->virtPage.page));
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
    Vm_VirtAddr		*virtAddrPtr;	/* The translated virtual address that 
					 * indicates the segment and virtual
					 * page that this physical page is
					 * being allocated for */
    unsigned int	page;
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
    if (virtAddrPtr->segPtr != vm_SysSegPtr) {
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
INTERNAL unsigned int
VmGetReservePage(virtAddrPtr)
    Vm_VirtAddr	*virtAddrPtr;
{
    VmCore	*corePtr;

    if (List_IsEmpty(reservePageList)) {
	return(VM_NO_MEM_VAL);
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
    unsigned	int	pfNum;		/* The page frame to be freed. */
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
	numPageOutProcs < vmMaxPageOutProcs) { 
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
    unsigned	int	pfNum;
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
 *	Routines to validate and invalidate pages.
 *
 * ----------------------------------------------------------------------------
 */


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageValidate --
 *
 *     	Validate the page at the given virtual address.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmPageValidate(virtAddrPtr)
    Vm_VirtAddr	*virtAddrPtr;
{
    LOCK_MONITOR;

    VmPageValidateInt(virtAddrPtr, 
		      VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page));

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageValidateInt --
 *
 *     	Validate the page at the given virtual address.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Page table modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL void
VmPageValidateInt(virtAddrPtr, ptePtr)
    Vm_VirtAddr		*virtAddrPtr;
    register	Vm_PTE	*ptePtr;
{
    if  (!(*ptePtr & VM_PHYS_RES_BIT)) {
	virtAddrPtr->segPtr->resPages++;
	*ptePtr |= VM_PHYS_RES_BIT;
    }
    VmMach_PageValidate(virtAddrPtr, *ptePtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageInvalidate --
 *
 *     	Invalidate the page at the given virtual address.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmPageInvalidate(virtAddrPtr)
    register	Vm_VirtAddr	*virtAddrPtr;
{
    LOCK_MONITOR;

    VmPageInvalidateInt(virtAddrPtr, 
	VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page));

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageInvalidateInt --
 *
 *     	Invalidate the page at the given virtual address.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Page table modified.
 * ----------------------------------------------------------------------------
 */
INTERNAL void
VmPageInvalidateInt(virtAddrPtr, ptePtr)
    Vm_VirtAddr		*virtAddrPtr;
    register	Vm_PTE	*ptePtr;
{
    if (*ptePtr & VM_PHYS_RES_BIT) {
	virtAddrPtr->segPtr->resPages--;
	VmMach_PageInvalidate(virtAddrPtr, Vm_GetPageFrame(*ptePtr), FALSE);
	*ptePtr &= ~(VM_PHYS_RES_BIT | VM_PAGE_FRAME_FIELD);
    }
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
    unsigned	int		pfNum;
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
    unsigned	int	pfNum;
{
    LOCK_MONITOR;
    coreMap[pfNum].lockCount--;
    if (coreMap[pfNum].lockCount < 0) {
	Sys_Panic(SYS_FATAL, "VmUnlockPage: Coremap lock count < 0\n");
    }
    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmUnlockPageInt --
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
INTERNAL void
VmUnlockPageInt(pfNum)
    unsigned	int	pfNum;
{
    coreMap[pfNum].lockCount--;
    if (coreMap[pfNum].lockCount < 0) {
	Sys_Panic(SYS_FATAL, "VmUnlockPage: Coremap lock count < 0\n");
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageSwitch --
 *
 *     	Move the given page from the current owner to the new owner.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The segment pointer int the core map entry for the page is modified and
 *	the page is unlocked.
 *
 * ----------------------------------------------------------------------------
 */
INTERNAL void
VmPageSwitch(pageNum, newSegPtr)
    unsigned	int	pageNum;
    Vm_Segment		*newSegPtr;
{
    coreMap[pageNum].virtPage.segPtr = newSegPtr;
    coreMap[pageNum].lockCount--;
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
 * Vm_GetRefTime --
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

    if (vmAlwaysRefuse) {
	refTime = 0x7fffffff;
    } else if (vmAlwaysSayYes) {
	refTime = 0;
    }
    refTime += vmCurPenalty;

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
    unsigned	int	*pagePtr;
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
	*pagePtr = VM_NO_MEM_VAL;
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
 *     	The virtual page number that is allocated.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static unsigned	int
DoPageAllocate(virtAddrPtr, flags)
    Vm_VirtAddr	*virtAddrPtr;	/* The translated virtual address that 
				   indicates the segment and virtual page 
				   that this physical page is being allocated 
				   for */
    int		flags;		/* VM_CAN_BLOCK | VM_ABORT_WHEN_DIRTY */
{
    unsigned	int page;

    LOCK_MONITOR;

    while (swapDown) {
	(void)Sync_Wait(&swapDownCondition, FALSE);
    }
    page = VmPageAllocateInt(virtAddrPtr, flags);

    UNLOCK_MONITOR;
    return(page);
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmPageAllocate --
 *
 *     	Return a page frame.  Will either get a page from VM or FS depending
 *	on the LRU comparison and if there is a free page or not.
 *
 * Results:
 *     	The page frame number that is allocated.
 *
 * Side effects:
 *     	None.
 *
 * ----------------------------------------------------------------------------
 */
unsigned int
VmPageAllocate(virtAddrPtr, flags)
    Vm_VirtAddr	*virtAddrPtr;	/* The translated virtual address that this
				 * page frame is being allocated for */
    int		flags;		/* VM_CAN_BLOCK | VM_ABORT_WHEN_DIRTY. */
{
    unsigned	int	page;
    int			refTime;
    int			tPage;

    vmStat.numAllocs++;

    GetRefTime(&refTime, &page);
    if (page == VM_NO_MEM_VAL) {
	Fs_GetPageFromFS(refTime + vmCurPenalty, &tPage);
	if (tPage == -1) {
	    vmStat.pageAllocs++;
	    return(DoPageAllocate(virtAddrPtr, flags));
	} else {
	    page = tPage;
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
INTERNAL unsigned int
VmPageAllocateInt(virtAddrPtr, flags)
    Vm_VirtAddr	*virtAddrPtr;	/* The translated virtual address that 
				   this page frame is being allocated for */
    int		flags;		/* VM_CAN_BLOCK if can block if non memory is
				 * available. VM_ABORT_WHEN_DIRTY if should
				 * abort even if VM_CAN_BLOCK is set if have
				 * exceeded the maximum number of dirty pages
				 * on the dirty list. */
{
    register	VmCore	*corePtr; 
    register	Vm_PTE	*ptePtr;
    Time		curTime;
    List_Links		endMarker;
    Boolean		referenced;
    Boolean		modified;

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

	/*
	 * Loop examining the page on the front of the allocate list until 
	 * a free or unreferenced, unmodified, unlocked page frame is found.
	 * If the whole list is examined and nothing found, then return 
	 * VM_NO_MEM_VAL.
	 */
	while (TRUE) {
	    corePtr = (VmCore *) List_First(allocPageList);

	    /*
	     * See if have gone all of the way through the list without finding
	     * anything.
	     */
	    if (((flags & (VM_CAN_BLOCK | VM_ABORT_WHEN_DIRTY)) && 
	         vmStat.numDirtyPages > vmMaxDirtyPages) ||
	        corePtr == (VmCore *) &endMarker) {	
		VmListRemove((List_Links *) &endMarker);
		if (!(flags & VM_CAN_BLOCK)) {
		    return(VM_NO_MEM_VAL);
		} else {
		    /*
		     * There were no pages available.  Wait for a clean
		     * page to appear on the allocate list.
		     */
		    (void)Sync_Wait(&cleanCondition, FALSE);
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

	    ptePtr = VmGetPTEPtr(corePtr->virtPage.segPtr, 
				 corePtr->virtPage.page);
	    VmMach_GetRefModBits(&corePtr->virtPage, Vm_GetPageFrame(*ptePtr),
				 &referenced, &modified);
	    /*
	     * Now make sure that the page has not been referenced.  It it has
	     * then clear the reference bit and put it onto the end of the
	     * allocate list.
	     */
	    if ((*ptePtr & VM_REFERENCED_BIT) || referenced) {
		vmStat.refSearched++;
		corePtr->lastRef = curTime.seconds;
		VmMach_ClearRefBit(&corePtr->virtPage, Vm_GetPageFrame(*ptePtr));
		*ptePtr &= ~VM_REFERENCED_BIT;
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
		continue;
	    }

	    if (corePtr->virtPage.segPtr->type != VM_CODE) {
		vmStat.potModPages++;
	    }
	    /*
	     * The page is available and it has not been referenced.  Now
	     * it must be determined if it is dirty.  If it is then put it onto
	     * the dirty list.
	     */
	    if ((*ptePtr & VM_MODIFIED_BIT) || modified) {
		vmStat.dirtySearched++;
		TakeOffAllocList(corePtr);
		PutOnDirtyList(corePtr);
		if (!modified) {
		    vmStat.notHardModPages++;
		}
		if (vmFreeWhenClean) {
		    /*
		     * Invalidate the page in hardware.  This will force
		     * a fault to occur if the page is to be referenced.
		     */
		    VmMach_PageInvalidate(&corePtr->virtPage, 
					  Vm_GetPageFrame(*ptePtr), FALSE);
		}
		continue;
	    }

	    if (corePtr->virtPage.segPtr->type != VM_CODE) {
		vmStat.notModPages++;
	    }
	    /*
	     * We can take this page.  Invalidate the page for the old segment.
	     */
	    VmPageInvalidateInt(&(corePtr->virtPage), ptePtr);
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
    if (virtAddrPtr->segPtr != vm_SysSegPtr) {
	PutOnAllocListRear(corePtr);
    }
    corePtr->virtPage = *virtAddrPtr;
    corePtr->flags = 0;
    corePtr->lockCount = 1;
    corePtr->wireCount = 0;
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
    unsigned	int	pfNum;		/* The page frame to be freed. */
{
    register	VmCore	*corePtr; 

    corePtr = &(coreMap[pfNum]);

    corePtr->flags |= VM_FREE_PAGE;
    corePtr->lockCount = 0;

    if (corePtr->virtPage.segPtr == vm_SysSegPtr) {
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
		(void) Sync_Wait(&corePtr->virtPage.segPtr->condition, FALSE);
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
    unsigned	int	pfNum;		/* The page frame to be freed. */
{
    LOCK_MONITOR;

    VmPageFreeInt(pfNum);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_ReservePage --
 *
 *      Take a page out of the available pages because this page is
 *	being used by the hardware dependent module.  This routine is
 *	called at boot time.
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
Vm_ReservePage(pfNum)
    unsigned	int	pfNum;		/* The page frame to be freed. */
{
    register	VmCore	*corePtr;

    LOCK_MONITOR;

    corePtr = &coreMap[pfNum];
    TakeOffAllocList(corePtr);
    corePtr->virtPage.segPtr = vm_SysSegPtr;
    corePtr->flags = 0;
    corePtr->lockCount = 1;

    UNLOCK_MONITOR;
}
	    

/*-----------------------------------------------------------------------
 *
 * 		Routines to handle page faults.
 *
 * Page fault handling is divided into three routines.  The first
 * routine is Vm_PageIn.  It calls two monitored routines PreparePage and
 * FinishPage to do most of the monitor level work.
 */

typedef enum {
    IS_COR,	/* This page is copy-on-reference. */
    IS_COW, 	/* This page is copy-on-write. */
    IS_DONE, 	/* The page-in has already completed. */
    NOT_DONE,	/* The page-in is not yet done yet. */
} PrepareResult;

PrepareResult	PreparePage();
void		FinishPage();


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_PageIn --
 *
 *     This routine is called to read in the page at the given virtual address.
 *
 * Results:
 *     SUCCESS if the page-in was successful and FAILURE otherwise.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Vm_PageIn(virtAddr, protFault)
    Address 	virtAddr;	/* The virtual address of the desired page */
    Boolean	protFault;	/* TRUE if fault is because of a protection
				 * violation. */
{
    register	Vm_PTE 		*ptePtr;
    register	Vm_Segment	*segPtr;
    register	int		page;
    Vm_VirtAddr	 		transVirtAddr;	
    ReturnStatus 		status;
    Proc_ControlBlock		*procPtr;
    unsigned	int		virtFrameNum;
    PrepareResult		result;

    vmStat.totalFaults++;

    procPtr = Proc_GetCurrentProc();
    /*
     * Determine which segment that this virtual address falls into.
     */
    VmVirtAddrParse(procPtr, virtAddr, &transVirtAddr);
    segPtr = transVirtAddr.segPtr;
    page = transVirtAddr.page;
    if (segPtr == (Vm_Segment *) NIL) {
	return(FAILURE);
    }

    if (protFault && segPtr->type == VM_CODE) {
	/*
	 * Access violation.
	 */
	return(FAILURE);
    }

    /*
     * Make sure that the virtual address is within the allocated part of the
     * segment.  If not, then either return error if heap or code segment,
     * or automatically expand the stack if stack segment.
     */
    if (!VmCheckBounds(&transVirtAddr)) {
	if (segPtr->type == VM_STACK) {
	    int	lastPage;
	    /*
	     * If this is a stack segment, then automatically grow it.
	     */
	    lastPage = mach_LastUserStackPage - segPtr->numPages;
	    status = VmAddToSeg(segPtr, page, lastPage);
	    if (status != SUCCESS) {
		goto pageinDone;
	    }
	} else {
	    status = FAILURE;
	    goto pageinDone;
	}
    }

    switch (segPtr->type) {
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

    ptePtr = VmGetPTEPtr(segPtr, page);
    /*
     * Fetch the next page.
     */
    if (vmPrefetch) {
	VmPrefetch(&transVirtAddr, ptePtr + 1);
    }

    while (TRUE) {
	/*
	 * Do the first part of the page-in.
	 */
	result = PreparePage(&transVirtAddr, protFault, ptePtr);
	if (!vm_CanCOW && (result == IS_COR || result == IS_COW)) {
	    Sys_Panic(SYS_FATAL, "Vm_PageIn: Bogus COW or COR\n");
	}
	if (result == IS_COR) {
	    status = VmCOR(&transVirtAddr);
	    if (status != SUCCESS) {
		status = FAILURE;
		goto pageinDone;
	    }
	} else if (result == IS_COW) {
	    VmCOW(&transVirtAddr);
	} else {
	    break;
	}
    }
    if (result == IS_DONE) {
	status = SUCCESS;
	goto pageinDone;
    }

    /*
     * Allocate a page.
     */
    virtFrameNum = VmPageAllocate(&transVirtAddr, TRUE);
    *ptePtr |= virtFrameNum;

    /*
     * Call the appropriate routine to fill the page.
     */
    if (*ptePtr & VM_ZERO_FILL_BIT) {
	vmStat.zeroFilled++;
	VmZeroPage(virtFrameNum);
	*ptePtr |= VM_MODIFIED_BIT;
	status = SUCCESS;
    } else if (*ptePtr & VM_ON_SWAP_BIT) {
	vmStat.psFilled++;
	status = VmPageServerRead(&transVirtAddr, virtFrameNum);
    } else {
	vmStat.fsFilled++;
	status = VmFileServerRead(&transVirtAddr, virtFrameNum);
    }

    *ptePtr |= VM_REFERENCED_BIT;

    /*
     * Finish up the page-in process.
     */
    FinishPage(&transVirtAddr, ptePtr);

    /*
     * Now check to see if the read suceeded.  If not destroy all processes
     * that are sharing the code segment.
     */
    if (status != SUCCESS) {
	VmKillSharers(segPtr);
    }

pageinDone:

    if (transVirtAddr.flags & VM_HEAP_PT_IN_USE) {
	/*
	 * The heap segment has been made not expandable by VmVirtAddrParse
	 * so that the address parse would remain valid.  Decrement the
	 * in use count now.
	 */
	VmDecPTUserCount(procPtr->vmPtr->segPtrArray[VM_HEAP]);
    }

    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PreparePage --
 *
 *	This routine performs the first half of the page-in process.
 *	It will return a status to the caller telling them what the status
 *	of the page is.
 *
 * Results:
 *	IS_DONE if the page is already resident in memory and it is not a 
 *	COW faults.  IS_COR is it is for a copy-on-reference fault.  IS_COW
 *	if is for a copy-on-write fault.  Otherwise returns NOT_DONE.
 *
 * Side effects:
 *	*ptePtrPtr is set to point to the page table entry for this virtual
 *	page.  In progress bit set if the NOT_DONE status is returned.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static PrepareResult
PreparePage(virtAddrPtr, protFault, curPTEPtr)
    register Vm_VirtAddr *virtAddrPtr; 	/* The translated virtual address */
    Boolean		protFault;	/* TRUE if faulted because of a
					 * protection fault. */
    register	Vm_PTE	*curPTEPtr;	/* Page table pointer for the page. */
{
    PrepareResult	retVal;

    LOCK_MONITOR;

again:
    if (*curPTEPtr & VM_IN_PROGRESS_BIT) {
	/*
	 * The page is being faulted on by someone else.  In this case wait
	 * for the page fault to complete.
	 */
	vmStat.collFaults++;
	(void) Sync_Wait(&virtAddrPtr->segPtr->condition, FALSE);
	goto again;
    } else if (*curPTEPtr & VM_COR_BIT) {
	/*
	 * Copy-on-reference fault.
	 */
	retVal = IS_COR;
    } else if (protFault && (*curPTEPtr & VM_COW_BIT) && 
	       (*curPTEPtr & VM_PHYS_RES_BIT)) {
	/*
	 * Copy-on-write fault.
	 */
	retVal = IS_COW;
    } else if (*curPTEPtr & VM_PHYS_RES_BIT) {
	/*
	 * The page is already in memory.  Validate it in hardware and set
	 * the reference bit since we are about to reference it.
	 */
	if (protFault && (*curPTEPtr & VM_COR_CHECK_BIT)) {
	    if (virtAddrPtr->segPtr->type == VM_HEAP) {
		vmStat.numCORCOWHeapFaults++;
	    } else {
		vmStat.numCORCOWStkFaults++;
	    }
	    *curPTEPtr &= ~(VM_COR_CHECK_BIT | VM_READ_ONLY_PROT);
	} else {
	    vmStat.quickFaults++;
	}
	if (*curPTEPtr & VM_PREFETCH_BIT) {
	    switch (virtAddrPtr->segPtr->type) {
		case VM_CODE:
		    vmStat.codePrefetchHits++;
		    break;
		case VM_HEAP:
		    if (*curPTEPtr & VM_ON_SWAP_BIT) {
			vmStat.heapSwapPrefetchHits++;
		    } else {
			vmStat.heapFSPrefetchHits++;
		    }
		    break;
		case VM_STACK:
		    vmStat.stackPrefetchHits++;
		    break;
	    }
	    *curPTEPtr &= ~VM_PREFETCH_BIT;
	}
	VmPageValidateInt(virtAddrPtr, curPTEPtr);
	*curPTEPtr |= VM_REFERENCED_BIT;
        retVal = IS_DONE;
    } else {
	*curPTEPtr |= VM_IN_PROGRESS_BIT;
	retVal = NOT_DONE;
    }

    UNLOCK_MONITOR;
    return(retVal);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FinishPage --
 *	This routine finishes the page-in process.  This includes validating
 *	the page for the currently executing process and releasing the 
 *	lock on the page.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Page-in in progress cleared and lockcount decremented in the
 * 	core map entry.
 *	
 *
 * ----------------------------------------------------------------------------
 */
ENTRY static void
FinishPage(transVirtAddrPtr, ptePtr) 
    register	Vm_VirtAddr	*transVirtAddrPtr;
    register	Vm_PTE		*ptePtr;
{
    LOCK_MONITOR;

    /*
     * Make the page accessible to the user.
     */
    VmPageValidateInt(transVirtAddrPtr, ptePtr);
    coreMap[Vm_GetPageFrame(*ptePtr)].lockCount--;
    *ptePtr &= ~(VM_ZERO_FILL_BIT | VM_IN_PROGRESS_BIT);
    /*
     * Wakeup processes waiting for this pagein to complete.
     */
    Sync_Broadcast(&transVirtAddrPtr->segPtr->condition);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * VmKillSharers --
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

ENTRY void
VmKillSharers(segPtr) 
    register	Vm_Segment	*segPtr;
{
    register	VmProcLink	*procLinkPtr;

    LOCK_MONITOR;

    LIST_FORALL(segPtr->procList, (List_Links *) procLinkPtr) {
	Sig_SendProc(procLinkPtr->procPtr, SIG_KILL, PROC_VM_READ_ERROR); 
    }

    UNLOCK_MONITOR;
}

Boolean	PageLocked();


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_PinUserMem --
 *
 *      Hardwire pages for all user addresses between firstAddr and
 *	lastAddr.
 *
 * Results:
 *     SUCCESS if the page-in was successful and SYS_ARG_NO_ACCESS otherwise.
 *
 * Side effects:
 *     Pages between firstAddr and lastAddr are wired down in memory.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Vm_PinUserMem(mapType, numBytes, addr)
    int		mapType;	/* VM_READONLY_ACCESS | VM_READWRITE_ACCESS */
    int		numBytes;	/* Number of bytes to map. */
    Address	addr;		/* Where to start mapping at. */
{
    Vm_VirtAddr	 		virtAddr;
    Proc_ControlBlock		*procPtr;
    int				firstPage;
    int				lastPage;
    int				accBytes;
    Address			accAddr;
    ReturnStatus		status = SUCCESS;

    if (numBytes == 0) {
	return(SUCCESS);
    } else if (numBytes < 0) {
	return(SYS_INVALID_ARG);
    }
    firstPage = (unsigned int)addr >> vmPageShift;
    lastPage = (unsigned int)(addr + numBytes - 1) >> vmPageShift;
    if (lastPage - firstPage >= VM_MAX_USER_MAP_PAGES) {
	return(SYS_INVALID_ARG);
    }
    /*
     * Make sure that the range of addresses are accessible.
     */
    Vm_MakeAccessible(VM_READWRITE_ACCESS, numBytes, addr, &accBytes, &accAddr);
    if (accAddr == (Address)NIL) {
	return(SYS_INVALID_ARG);
    } else if (accBytes != numBytes) {
	Vm_MakeUnaccessible(addr, numBytes);
	return(SYS_INVALID_ARG);
    }
    /*
     * Determine the segment that the addresses are in so that we can 
     * lock the pages down.
     */
    procPtr = Proc_GetCurrentProc();
    VmVirtAddrParse(procPtr, addr, &virtAddr);
    if (virtAddr.flags & VM_HEAP_PT_IN_USE) {
	/*
	 * The heap segment has been made not expandable by VmVirtAddrParse
	 * so that the address parse would remain valid.  Decrement the
	 * in use count now.  We don't have to leave the count up because
	 * the page tables were locked when we did the make accessible above.
	 */
	VmDecPTUserCount(procPtr->vmPtr->segPtrArray[VM_HEAP]);
    }
    if (mapType != VM_READONLY_ACCESS && virtAddr.segPtr->type == VM_CODE) {
	status = SYS_INVALID_ARG;
    } else {
	/*
	 * If this segment can still be made copy-on-write then disallow it
	 * because once we start hardwiring user pages in memory we can't
	 * deal with copy-on-write.
	 */
	if (!(virtAddr.segPtr->flags & VM_SEG_CANT_COW)) {
	    VmSegCantCOW(virtAddr.segPtr);
	}
	/*
	 * Finally lock down all of the pages.
	 */
	for (; virtAddr.page <= lastPage; virtAddr.page++) {
	    int	val;
	    int	*valAddr;

	    valAddr = (int *) (virtAddr.page << vmPageShift);
	    while (TRUE) {
		val = *valAddr;
		if (PageLocked(&virtAddr)) {
		    break;
		}
	    }
	}
    }

    Vm_MakeUnaccessible(addr, numBytes);

    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PageLocked --
 *
 *      Hardwire pages for all user addresses between firstAddr and
 *	lastAddr.
 *
 * Results:
 *     TRUE if the page was resident and FALSE otherwise.
 *
 * Side effects:
 *     Core map entry lock count and flags may be modified.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
PageLocked(virtAddrPtr)
    Vm_VirtAddr	*virtAddrPtr;
{
    register	VmCore	*corePtr;
    register	Vm_PTE	*ptePtr;
    Boolean		retVal;

    LOCK_MONITOR;

    ptePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
    while (*ptePtr & VM_IN_PROGRESS_BIT) {
	Sync_Wait(&virtAddrPtr->segPtr->condition, FALSE);
    }
    if (!(*ptePtr & VM_PHYS_RES_BIT)) {
	retVal = FALSE;
    } else {
	corePtr = &coreMap[Vm_GetPageFrame(*ptePtr)];
	corePtr->wireCount++;
	corePtr->lockCount++;
	retVal = TRUE;
    }

    UNLOCK_MONITOR;

    return(retVal);
}

void	PageUnlock();


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_UnpinUserMem --
 *
 *      Unlock all pages between firstAddr and lastAddr.
 *	lastAddr.
 *
 * Results:
 *     SUCCESS if the page-in was successful and FAILURE otherwise.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Vm_UnpinUserMem(numBytes, addr)
    int		numBytes;	/* The number of bytes to map. */
    Address 	addr;		/* The address to start mapping at. */
{
    Vm_VirtAddr	 		virtAddr;
    Proc_ControlBlock		*procPtr;
    int				lastPage;
    ReturnStatus		status;

    if (numBytes == 0) {
	return(SUCCESS);
    } else if (numBytes < 0) {
	return(SYS_INVALID_ARG);
    }
    lastPage = (unsigned int)(addr + numBytes - 1) >> vmPageShift;

    /*
     * Make sure that all pages between first and last addr fall into the
     * same segment
     */
    procPtr = Proc_GetCurrentProc();
    VmVirtAddrParse(procPtr, addr, &virtAddr);
    if (lastPage - virtAddr.segPtr->offset >= virtAddr.segPtr->numPages) {
	status = SYS_ARG_NOACCESS;
	goto done;
    }

    /*
     * Now unlock all of the pages.
     */
    for (; virtAddr.page <= lastPage; virtAddr.page++) {
	PageUnlock(&virtAddr);
    }

done:
    if (virtAddr.flags & VM_HEAP_PT_IN_USE) {
	/*
	 * The heap segment has been made not expandable by VmVirtAddrParse
	 * so that the address parse would remain valid.  Decrement the
	 * in use count now.
	 */
	VmDecPTUserCount(procPtr->vmPtr->segPtrArray[VM_HEAP]);
    }

    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * PageUnlock --
 *
 *      Unlock pages for all user addresses between firstAddr and
 *	lastAddr.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Core map entry lock count and flags field may be modified.
 *
 * ----------------------------------------------------------------------------
 */
void
PageUnlock(virtAddrPtr)
    Vm_VirtAddr	*virtAddrPtr;
{
    register	VmCore	*corePtr;
    register	Vm_PTE	*ptePtr;

    LOCK_MONITOR;

    ptePtr = VmGetPTEPtr(virtAddrPtr->segPtr, virtAddrPtr->page);
    while (*ptePtr & VM_IN_PROGRESS_BIT) {
	Sync_Wait(&virtAddrPtr->segPtr->condition, FALSE);
    }

    if (*ptePtr & VM_PHYS_RES_BIT) {
	corePtr = &coreMap[Vm_GetPageFrame(*ptePtr)];
	if (corePtr->wireCount > 0) {
	    corePtr->wireCount--;
	    corePtr->lockCount--;
	}
    }

    UNLOCK_MONITOR;
}


/*----------------------------------------------------------------------------
 *
 * 		Routines for writing out dirty pages		
 *
 * Dirty pages are written to the swap file by the function PageOut.
 * PageOut is called by using the Proc_CallFunc routine which invokes
 * a process on PageOut.  When a page is put onto the dirty list a new
 * incantation of PageOut will be created unless there are already
 * more than vmMaxPageOutProcs already writing out the dirty list.  Thus the
 * dirty list will be cleaned by at most vmMaxPageOutProcs working in parallel.
 *
 * The work done by PageOut is split into work done at non-monitor level and
 * monitor level.  It calls the monitored routine PageOutPutAndGet to get the 
 * next page off of the dirty list.  It then writes the page out to the 
 * file server at non-monitor level.  Next it calls the monitored routine 
 * PageOutPutAndGet to put the page onto the front of the allocate list and
 * get the next dirty page.  Finally when there are no more pages to clean it
 * returns (and dies).
 */

static	void	PageOutPutAndGet();
static	void	PutOnFront();

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
	status = VmPageServerWrite(&corePtr->virtPage, 
				   (unsigned int) (corePtr - coreMap));
	if (status != SUCCESS) {
	    if (vmSwapStreamPtr == (Fs_Stream *)NIL ||
	        (status != RPC_TIMEOUT && status != FS_STALE_HANDLE &&
		 status != RPC_SERVICE_DISABLED)) {
		/*
		 * Non-recoverable error on page write, so kill all users of 
		 * this segment.
		 */
		VmKillSharers(corePtr->virtPage.segPtr);
	    }
	}
    }

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
    register	Vm_PTE	*ptePtr;
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
	    case RPC_SERVICE_DISABLED:
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
	    VmPageInvalidateInt(&corePtr->virtPage,
		VmGetPTEPtr(corePtr->virtPage.segPtr, corePtr->virtPage.page));
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
	ptePtr = VmGetPTEPtr(corePtr->virtPage.segPtr, corePtr->virtPage.page);
	*ptePtr |= VM_ON_SWAP_BIT;
	*ptePtr &= ~VM_MODIFIED_BIT;
	VmMach_ClearModBit(&corePtr->virtPage, Vm_GetPageFrame(*ptePtr));
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
    register	Vm_PTE	*ptePtr;

    if (corePtr->flags & VM_SEG_PAGEOUT_WAIT) {
	Sync_Broadcast(&corePtr->virtPage.segPtr->condition);
    }
    corePtr->flags &= ~(VM_DIRTY_PAGE | VM_PAGE_BEING_CLEANED | 
		        VM_SEG_PAGEOUT_WAIT | VM_DONT_FREE_UNTIL_CLEAN);
    if (corePtr->flags & VM_FREE_PAGE) {
	PutOnFreeList(corePtr);
    } else {
	if (vmFreeWhenClean && corePtr->lockCount == 0) {
	    /*
	     * We are supposed to free pages after we clean them.  Before
	     * we put this page onto the dirty list, we already invalidated
	     * it in hardware, thus forcing it to be faulted on before being
	     * referenced.  If it was faulted on then PreparePage would have
	     * set the reference bit in the PTE.  Thus if the reference bit
	     * isn't set then the page isn't valid and thus it couldn't
	     * possibly have been modified or referenced.  So we free this
	     * page.
	     */
	    ptePtr = VmGetPTEPtr(corePtr->virtPage.segPtr,
				 corePtr->virtPage.page);
	    if (!(*ptePtr & VM_REFERENCED_BIT)) {
		if (!(*ptePtr & VM_PHYS_RES_BIT)) {
		    Sys_Panic(SYS_FATAL, "PutOnFront: Resident bit not set\n");
		}
		corePtr->virtPage.segPtr->resPages--;
		*ptePtr &= ~(VM_PHYS_RES_BIT | VM_PAGE_FRAME_FIELD);
		PutOnFreeList(corePtr);
	    } else {
		PutOnAllocListFront(corePtr);
	    }
	} else {
	    PutOnAllocListFront(corePtr);
	}
    }
    vmStat.numDirtyPages--; 
    Sync_Broadcast(&cleanCondition);
}


/*
 * Variables for the clock daemon.  vmPagesToCheck is the number of page 
 * frames to examine each time that the clock daemon wakes up.  vmClockSleep
 * is the amount of time for the clock daemon before it runs again.
 */
unsigned int	vmClockSleep;		
int		vmPagesToCheck = 100;
static	int	clockHand = 0;

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
 *     	The allocate list is modified.
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

    register	VmCore	*corePtr;
    register	Vm_PTE	*ptePtr;
    int			i;
    Time		curTime;
    Boolean		referenced;
    Boolean		modified;

    LOCK_MONITOR;

    Timer_GetTimeOfDay(&curTime, (int *) NIL, (Boolean *) NIL);

    if (vm_Tracing) {
	VmMach_Trace();

	/*
	 * Decrement the number of traces per iteration of the clock.  If we
	 * are at 0 then run the clock for one interation.
	 */
	vmTracesToGo--;
	if (vmTracesToGo > 0) {
	    callInfoPtr->interval = vmClockSleep;
	    UNLOCK_MONITOR;
	    return;
	}
	vmTracesToGo = vmTracesPerClock;
    }

    /*
     * Examine vmPagesToCheck pages.
     */

    for (i = 0; i < vmPagesToCheck; i++) {
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
	 * or in the middle of a pageout, then we aren't concerned 
	 * with this page.
	 */
	if ((corePtr->flags & (VM_DIRTY_PAGE | VM_FREE_PAGE)) ||
	    corePtr->lockCount > 0) {
	    continue;
	}

	ptePtr = VmGetPTEPtr(corePtr->virtPage.segPtr, corePtr->virtPage.page);

	/*
	 * If the page has been referenced, then put it on the end of the
	 * allocate list.
	 */
	VmMach_GetRefModBits(&corePtr->virtPage, Vm_GetPageFrame(*ptePtr),
			     &referenced, &modified);
	if ((*ptePtr & VM_REFERENCED_BIT) || referenced) {
	    VmListMove((List_Links *) corePtr, LIST_ATREAR(allocPageList));
	    corePtr->lastRef = curTime.seconds;
	    *ptePtr &= ~VM_REFERENCED_BIT;
	    VmMach_ClearRefBit(&corePtr->virtPage, Vm_GetPageFrame(*ptePtr));
	}
    }

    if (!initialized) {
        vmClockSleep = timer_IntOneSecond;
	initialized = TRUE;
    }

    callInfoPtr->interval = vmClockSleep;

    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmCountDirtyPages --
 *
 *	Return the number of dirty pages.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The allocate list is modified.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY int
VmCountDirtyPages()
{
    register	Vm_PTE	*ptePtr;
    register	VmCore	*corePtr;
    register	int	i;
    register	int	numDirtyPages = 0;
    Boolean		referenced;
    Boolean		modified;

    LOCK_MONITOR;

    for (corePtr = &coreMap[vmFirstFreePage], i = vmFirstFreePage;
         i < vmStat.numPhysPages;
	 i++, corePtr++) {
	if ((corePtr->flags & VM_FREE_PAGE) || corePtr->lockCount > 0) {
	    continue;
	}
	if (corePtr->flags & VM_DIRTY_PAGE) {
	    numDirtyPages++;
	    continue;
	}
	ptePtr = VmGetPTEPtr(corePtr->virtPage.segPtr, corePtr->virtPage.page);
	if (*ptePtr & VM_MODIFIED_BIT) {
	    numDirtyPages++;
	    continue;
	}
	VmMach_GetRefModBits(&corePtr->virtPage, Vm_GetPageFrame(*ptePtr),
			     &referenced, &modified);
	if (modified) {
	    numDirtyPages++;
	}
    }
    UNLOCK_MONITOR;
    return(numDirtyPages);
}

/*
 * ----------------------------------------------------------------------------
 *
 * VmFlushSegment --
 *
 *	Flush the given range of pages in the segment to swap space and take
 *	them out of memory.  It is assumed that the processes that own this
 *	segment are frozen.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	All memory in the given range is forced out to swap and freed.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
VmFlushSegment(segPtr, firstPage, lastPage)
    Vm_Segment	*segPtr;
    int		firstPage;
    int		lastPage;
{
    register	Vm_PTE		*ptePtr;
    register	VmCore		*corePtr;
    unsigned int		pfNum;
    Boolean			referenced;
    Boolean			modified;
    Vm_VirtAddr			virtAddr;

    LOCK_MONITOR;

    if (segPtr->ptPtr == (Vm_PTE *)NIL) {
	UNLOCK_MONITOR;
	return;
    }
    virtAddr.segPtr = segPtr;
    virtAddr.page = firstPage;
    for (ptePtr = VmGetPTEPtr(segPtr, firstPage);
         virtAddr.page <= lastPage;
	 virtAddr.page++, VmIncPTEPtr(ptePtr, 1)) {
	if (!(*ptePtr & VM_PHYS_RES_BIT)) {
	    continue;
	}
	pfNum = Vm_GetPageFrame(*ptePtr);
	corePtr = &coreMap[pfNum];
	if (corePtr->lockCount > 0) {
	    continue;
	}
	if (corePtr->flags & VM_DIRTY_PAGE) {
	    corePtr->flags |= VM_DONT_FREE_UNTIL_CLEAN;
	} else {
	    VmMach_GetRefModBits(&virtAddr, Vm_GetPageFrame(*ptePtr),
				 &referenced, &modified);
	    if ((*ptePtr & VM_MODIFIED_BIT) || modified) {
		TakeOffAllocList(corePtr);
		PutOnDirtyList(corePtr);
		corePtr->flags |= VM_DONT_FREE_UNTIL_CLEAN;
	    }
	}
	VmPageFreeInt(pfNum);
	VmPageInvalidateInt(&virtAddr, ptePtr);
    }

    UNLOCK_MONITOR;
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
    return(vm_PageSize);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_MapBlock --
 *
 *      Allocate and validate enough pages at the given address to map
 *	one FS cache block.
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
    Vm_VirtAddr		virtAddr;
    unsigned	int	page;
    int			curFSPages;

    vmStat.fsMap++;
    curFSPages = vmStat.fsMap - vmStat.fsUnmap;
    if (curFSPages >= vmBoundary) {
	vmBoundary += vmPagesPerGroup;
	vmCurPenalty += vmFSPenalty;
    }
    if (curFSPages > vmStat.maxFSPages) {
	vmStat.maxFSPages = curFSPages;
    }

    virtAddr.page = (unsigned int) addr >> vmPageShift;
    virtAddr.offset = 0;
    virtAddr.segPtr = vm_SysSegPtr;
    ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);

    /*
     * Allocate a block.  We know that the page size is not smaller than
     * the block size so that one page will suffice.
     */
    page = DoPageAllocate(&virtAddr, 0);
    if (page == VM_NO_MEM_VAL) {
	/*
	 * Couldn't get any memory.  
	 */
	return(0);
    }
    *ptePtr |= page;
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
    unsigned int *pageNumPtr;	/* One of the pages that was unmapped. */
{
    register	Vm_PTE	*ptePtr;
    Vm_VirtAddr		virtAddr;
    int			curFSPages;

    vmStat.fsUnmap++;
    curFSPages = vmStat.fsMap - vmStat.fsUnmap;

    if (curFSPages < vmBoundary) {
	vmBoundary -= vmPagesPerGroup;
	vmCurPenalty -= vmFSPenalty;
    }
    if (curFSPages < vmStat.minFSPages) {
	vmStat.minFSPages = curFSPages;
    }

    virtAddr.page = (unsigned int) addr >> vmPageShift;
    virtAddr.offset = 0;
    virtAddr.segPtr = vm_SysSegPtr;
    ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);

    if (retOnePage) {
	*pageNumPtr = Vm_GetPageFrame(*ptePtr);
    } else {
	/*
	 * If we aren't supposed to return the page, then free it.
	 */
	VmPageFree(Vm_GetPageFrame(*ptePtr));
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
    vmMaxDirtyPages = vmStat.numFreePages / MAX_DIRTY_PAGE_FRACTION;

    *startAddrPtr = vmBlockCacheBaseAddr;
    /*
     * We aren't going to get any more free pages so limit the maximum number
     * of blocks in the cache to the number of free pages that we have minus
     * the minimum amount of free pages that we keep for user
     * processes to run.
     */
    numPages = ((unsigned int)vmBlockCacheEndAddr - 
		(unsigned int)vmBlockCacheBaseAddr) / vm_PageSize;
    if (numPages > vmStat.numFreePages - vmStat.minVMPages) {
	numPages = vmStat.numFreePages - vmStat.minVMPages;
    }
    *endAddrPtr = vmBlockCacheBaseAddr + numPages * vm_PageSize - 1;
    /*
     * Compute the penalties to put onto FS pages.
     */
    vmPagesPerGroup = vmStat.numFreePages / vmNumPageGroups;
    vmCurPenalty = 0;
    vmBoundary = vmPagesPerGroup;
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
	   numPageOutProcs < vmMaxPageOutProcs) { 
	Proc_CallFunc(PageOut, (ClientData) numPageOutProcs, 0);
	numPageOutProcs++;
    }

    UNLOCK_MONITOR;
}
