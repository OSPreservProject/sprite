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

#include <sprite.h>
#include <vm.h>
#include <vmInt.h>
#include <lock.h>
#include <list.h>
#include <proc.h>
#include <sched.h>
#include <sync.h>
#include <sys.h>
#include <vmHack.h>
#ifdef VM_CHECK_BSTRING_ACCESS
#include <dbg.h>
#include <stdlib.h>
#endif


Sync_Condition	mappingCondition;

int	vmNumMappedPages = 16;
int	vmMapBasePage;
int	vmMapEndPage;
Address vmMapBaseAddr;
Address vmMapEndAddr;


#ifdef VM_CHECK_BSTRING_ACCESS
/* 
 * Temporary: keep a list of which processes have called
 * Vm_MakeAccessible.  bcopy et al will check the list to verify that
 * it's okay to access the user address space.
 */

typedef struct {
    List_Links		links;
    Proc_ControlBlock	*procPtr;
    Address		startAddr; /* user address */
    int			numBytes; /* as returned by Vm_MakeAccessible */
    int			refCount;
} VmAccessInfo;

List_Links	vmAccessListHdr;
List_Links	*vmAccessList = &vmAccessListHdr;
static Sync_Lock	vmAccessListLock;

Boolean		vmDoAccessChecks = FALSE;

/* Forward references: */

static void RegisterAccess _ARGS_ ((Proc_ControlBlock *procPtr,
				    Address startAddr, int numBytes));
static void RemoveAccess _ARGS_ ((Proc_ControlBlock *procPtr,
				  Address startAddr, int numBytes));
static VmAccessInfo *
FindAccessElement _ARGS_ ((Proc_ControlBlock *procPtr, Address startAddr,
			   int numBytes));

#endif /* VM_CHECK_BSTRING_ACCESS */


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
		virtAddr.flags = 0;
		virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
		*ptePtr |= VM_PHYS_RES_BIT | pfNum;
		VmMach_PageValidate(&virtAddr, *ptePtr);
#ifdef spur
		/*
		 * Until we figure out how to handle virtual synonyms on 
		 * SPUR, we always make map address noncachable. 
		 */
		VmMach_MakeNonCachable(&virtAddr, *ptePtr);
#endif
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
    virtAddr.sharedPtr = (Vm_SegProcList *) NIL;
    /*
     * Clean the old page from the cache.
     */
    VmMach_FlushPage(&virtAddr, TRUE);
    ptePtr = VmGetPTEPtr(segPtr, virtAddr.page);
    *ptePtr &= ~VM_PAGE_FRAME_FIELD;
    *ptePtr |= pfNum;
    VmMach_PageValidate(&virtAddr, *ptePtr);
#ifdef spur
    /*
     * Until we figure out how to handle virtual synonyms on 
     * SPUR, we always make map address noncachable. 
     */
    VmMach_MakeNonCachable(&virtAddr, *ptePtr);
#endif

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
    virtAddr.flags = 0;
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;

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
 *	(Historical note: this function was originally used to access 
 *	pages without worrying about page faults.  This scheme proved 
 *	too slow, so there was a switch to CopyIn/CopyOut.  Now this 
 *	function is useful to ensure the validity of pte pointers.  
 *	See the sychronization comments in vmInt.h)
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
	goto done;
    }

    procPtr->vmPtr->numMakeAcc++;
    *retBytesPtr = numBytes;

#ifdef	sequent
    /*
     * Relocate the returned address to allow kernel to address it
     * directly.  mach_KernelVirtAddrUser should be defined for all
     * machines (with value == 0 on most).
     */
    *retAddrPtr = startAddr + mach_KernelVirtAddrUser;
#else
    *retAddrPtr = startAddr;
#endif  /* sequent */

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
	goto done;
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
	goto done;
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
	goto done;
    }
    /* 
     * If we couldn't make all of the requested pages accessible then return 
     * the number of bytes that we actually made accessible.
     */
    if (virtAddr.page <= lastPage) {
	*retBytesPtr = (virtAddr.page << vmPageShift) - (int) startAddr;
    }

 done:
#ifdef VM_CHECK_BSTRING_ACCESS
    if (vmDoAccessChecks && !dbg_BeingDebugged && *retBytesPtr != 0) {
	RegisterAccess(procPtr, startAddr, *retBytesPtr);
    }
#else 
    ;
#endif
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

#ifdef	sequent
    /*
     * Un-relocate the address back to user-relative.
     */
    addr -= mach_KernelVirtAddrUser;
#endif	/* sequent */

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

#ifdef VM_CHECK_BSTRING_ACCESS
    if (vmDoAccessChecks && !dbg_BeingDebugged && numBytes != 0) {
	RemoveAccess(procPtr, addr, numBytes);
    }
#endif
}

#ifdef VM_CHECK_BSTRING_ACCESS

/*
 *----------------------------------------------------------------------
 *
 * RegisterAccess --
 *
 *	Record the fact that the given process has acquired access to 
 *	the given range of addresses.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds an element to the access linked list if the range isn't 
 *	already registered.  Bumps the reference count for the range 
 *	if it has.
 *
 *----------------------------------------------------------------------
 */

static void
RegisterAccess(procPtr, startAddr, numBytes)
    Proc_ControlBlock	*procPtr;
    Address		startAddr; /* user address */
    int			numBytes; /* as returned by Vm_MakeAccessible */
{
    VmAccessInfo *accessPtr;

    Sync_GetLock(&vmAccessListLock);

    accessPtr = FindAccessElement(procPtr, startAddr, numBytes);
    if (accessPtr != (VmAccessInfo *)NIL) {
	accessPtr->refCount++;
    } else {
	accessPtr = (VmAccessInfo *)malloc(sizeof(VmAccessInfo));
	accessPtr->procPtr = procPtr;
	accessPtr->startAddr = startAddr;
	accessPtr->numBytes = numBytes;
	accessPtr->refCount = 1;
	List_InitElement((List_Links *)accessPtr);
	List_Insert((List_Links *)accessPtr,
		    LIST_ATREAR(vmAccessList));
    }

    Sync_Unlock(&vmAccessListLock);
}


/*
 *----------------------------------------------------------------------
 *
 * RemoveAccess --
 *
 *	Forget that the given process has access to the given range of 
 *	addresses.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Decrements the reference count for the given range.  Removes 
 *	the range from the list if the count goes to 0.  We assume
 *	that the caller will surrender access to the entire range that
 *	was acquired, rather than surrendering only part of the range.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveAccess(procPtr, startAddr, numBytes)
    Proc_ControlBlock	*procPtr;
    Address		startAddr; /* user address */
    int			numBytes; /* as returned by Vm_MakeAccessible */
{
    VmAccessInfo *accessPtr;

    Sync_GetLock(&vmAccessListLock);

    accessPtr = FindAccessElement(procPtr, startAddr, numBytes);
    if (accessPtr == (VmAccessInfo *)NIL) {
	vmDoAccessChecks = FALSE;
	panic("Vm_MakeUnAccessible: address range not registered");
    }

    accessPtr->refCount--;
    if (accessPtr->refCount <= 0) {
	List_Remove((List_Links *)accessPtr);
	free((char *)accessPtr);
    }

    Sync_Unlock(&vmAccessListLock);
}


/*
 *----------------------------------------------------------------------
 *
 * FindAccessElement --
 *
 *	Find the element in the access list corresponding to the given 
 *	arguments.  The caller should be holding the lock for the 
 *	access list.
 *
 * Results:
 *	Returns a pointer to the element if found, NIL if not found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static VmAccessInfo *
FindAccessElement(procPtr, startAddr, numBytes)
    Proc_ControlBlock *procPtr;
    Address	startAddr;
    int		numBytes;
{
    VmAccessInfo *accessPtr;

    LIST_FORALL(vmAccessList, (List_Links *)accessPtr) {
	if (accessPtr->procPtr == procPtr
		&& accessPtr->startAddr == startAddr
		&& accessPtr->numBytes == numBytes) {
	    return accessPtr;
	}
    }

    return (VmAccessInfo *)NIL;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_IsAccessible --
 *
 *	Verify that Vm_MakeAccessible has been called for the given 
 *	range of addresses.
 *
 * Results:
 *	Returns if okay, panics if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Vm_CheckAccessible(startAddr, numBytes)
    Address	startAddr;
    int		numBytes;
{
    VmAccessInfo *accessPtr;
    Proc_ControlBlock	*procPtr = Proc_GetCurrentProc();
    Boolean okay = FALSE;

    if (!vmDoAccessChecks || dbg_BeingDebugged) {
	return;
    }

    /* 
     * All accesses to kernel memory are okay.  Assume that the 
     * requested region doesn't wrap around the end of memory.
     */
    if (startAddr > mach_LastUserAddr
	    || startAddr + numBytes <= mach_FirstUserAddr) {
	return;
    }

    Sync_GetLock(&vmAccessListLock);

    LIST_FORALL(vmAccessList, (List_Links *)accessPtr) {
	if (accessPtr->procPtr != procPtr) {
	    continue;
	}
	/* 
	 * Check the start and end of the given range against the 
	 * range in the list element.  If the given range can't fit in 
	 * the list element, go on to the next element.
	 */
	if (accessPtr->startAddr <= startAddr
		&& (accessPtr->startAddr + accessPtr->numBytes
		    >= startAddr + numBytes)) {
	    okay = TRUE;
	    break;
	}
    }

    Sync_Unlock(&vmAccessListLock);

    if (!okay) {
	vmDoAccessChecks = FALSE;
	panic("Vm_IsAccessible: accessing user memory improperly");
    }
}



/*
 *----------------------------------------------------------------------
 *
 * VmMapInit --
 *
 *	Initialize the access list, lock, etc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The access list and lock are initialized.
 *
 *----------------------------------------------------------------------
 */

void
VmMapInit()
{
    Sync_LockInitDynamic(&vmAccessListLock, "Vm:accessListLock");
    List_Init(vmAccessList);
}

#endif /* VM_CHECK_BSTRING_ACCESS */
