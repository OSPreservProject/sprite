/* vmStack.c -
 *
 *     	This file contains routines to allocate and free kernel stacks.  It
 *	is managed by its own monitor.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "vm.h"
#include "vmInt.h"
#include "list.h"
#include "proc.h"
#include "sched.h"
#include "sync.h"
#include "sys.h"
#include "dbg.h"

Address	vmStackBaseAddr;
Address	vmStackEndAddr;

/*
 * Monitor declarations.
 */
Sync_Lock	stackLock;
#define	LOCKPTR	&stackLock

/*
 * Stack list element.
 */
typedef struct {
    List_Links	links;
    Address	startAddr;
} StackList;

/*
 * Array of stack list information.
 */
static	StackList	*stackListElements;

/*
 * There are two stack lists: in use stacks and free stacks.
 */
static	List_Links	activeListHdr;
#define	activeList	(&activeListHdr)
static	List_Links	freeListHdr;
#define	freeList	(&freeListHdr)

static	int	numStackPages;

static StackList *GetFreeStack _ARGS_((void));
static void FreeStack _ARGS_((Address stackBase));


/*
 * ----------------------------------------------------------------------------
 *
 * VmStackInit --
 *
 *      Allocate and initialize the stack stuff.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Stack information is allocated and initialization.
 *
 * ----------------------------------------------------------------------------
 */
void
VmStackInit()
{
    Address	addr;
    int		i;

    Sync_LockInitDynamic(&stackLock, "Vm:stackLock");
    stackListElements =
	    (StackList *)Vm_BootAlloc(vmMaxProcesses * sizeof(StackList));
    List_Init(activeList);
    List_Init(freeList);
    for (i = 0, addr = vmStackBaseAddr;
	 addr < vmStackEndAddr;
	 i++, addr += mach_KernStackSize) {
	 stackListElements[i].startAddr = (Address) addr;
	 List_Insert((List_Links *) &stackListElements[i], 
		     LIST_ATREAR(freeList));
    }
    numStackPages = mach_KernStackSize >> vmPageShift;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_GetKernelStack --
 *
 *      Allocate a stack for a kernel process.
 *
 * Results:
 *      The base of the new stack.
 *
 * Side effects:
 *      The kernels page table is modified to map in the new stack pages.
 *
 * ----------------------------------------------------------------------------
 */
Address
Vm_GetKernelStack(invalidPage)
    int	invalidPage;	/*Which of the stack pages to make invalid. */
{
    register	Vm_PTE		*ptePtr;
    register	StackList	*stackListPtr;
    register	int		i;
    Vm_VirtAddr			virtAddr;

    stackListPtr = GetFreeStack();
    if (stackListPtr == (StackList *) NIL) {
	return((Address)NIL);
    }

    /*
     * Need to allocate stack pages and initialize the page table entry.
     * We allocate page table entries for all pages except the first which
     * is protected.
     */
    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = ((unsigned int) stackListPtr->startAddr) >> vmPageShift;
    virtAddr.offset = 0;
    virtAddr.flags = 0;
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
    for (i = 0, ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);
	 i < numStackPages;
	 i++, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	if (i != invalidPage) {
	    *ptePtr |= VmPageAllocate(&virtAddr, VM_CAN_BLOCK);
	    vmStat.kernStackPages++;
	    VmPageValidate(&virtAddr);
	}
    }
    return(stackListPtr->startAddr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_FreeKernelStack --
 *
 *      Free up a kernel stack which was allocated at the given address.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      List of free stacks modified.
 *
 * ----------------------------------------------------------------------------
 */
void
Vm_FreeKernelStack(stackBase)
    Address	stackBase;	/* Virtual address of the stack that is being
				 * freed. */
{
    Vm_VirtAddr			virtAddr;
    register	int		i;
    register	Vm_PTE		*ptePtr;

    /*
     * Unmap the stack and free its pages.
     */
    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = (unsigned int) (stackBase) >> vmPageShift;
    virtAddr.offset = 0;
    virtAddr.sharedPtr = (Vm_SegProcList *)NIL;
    virtAddr.flags = 0;
    for (i = 0, ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);
	 i < numStackPages; 
	 i++, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	if (*ptePtr & VM_PHYS_RES_BIT) {
	    vmStat.kernStackPages--;
	    VmPageFree(Vm_GetPageFrame(*ptePtr));
	    VmPageInvalidate(&virtAddr);
	}
    }
    FreeStack(stackBase);
}


/*
 *----------------------------------------------------------------------
 *
 * GetFreeStack --
 *
 *	Move a stack from the free list to the active list and return
 *	a pointer to it.
 *
 * Results:
 *	Pointer to free stack, NIL if there aren't any.
 *
 * Side effects:
 *	Lists of free and active kernel stacks are modified..
 *
 *----------------------------------------------------------------------
 */

ENTRY static StackList *
GetFreeStack()
{
    register	StackList	*stackListPtr;

    LOCK_MONITOR;


    /*
     * Get the first free stack.
     */
    if (List_IsEmpty(freeList)) {
	stackListPtr = (StackList *) NIL;
    } else {
	stackListPtr = (StackList *) List_First(freeList);
	List_Move((List_Links *) stackListPtr, LIST_ATREAR(activeList));
    }
    UNLOCK_MONITOR;
    return(stackListPtr);
}
/*
 *----------------------------------------------------------------------
 *
 * FreeStack --
 *
 *	Move the stack from the active list to the free list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lists of free and active kernel stacks are modified..
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
FreeStack(stackBase)
    Address	stackBase;	/* Virtual address of the stack that is being
				 * freed. */
{
    register	StackList	*stackListPtr;

    LOCK_MONITOR;


    /*
     * Put the stack back onto the free list.
     */ 
    if (List_IsEmpty(activeList)) {
	panic("Vm_FreeKernelStack: active list empty\n");
    }
    stackListPtr = (StackList *) List_First(activeList);
    List_Move((List_Links *) stackListPtr, LIST_ATREAR(freeList));
    stackListPtr->startAddr = stackBase;

    UNLOCK_MONITOR;
}

