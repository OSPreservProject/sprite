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
#include "machine.h"
#include "mem.h"
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
Sync_Lock	stackLock = {0, 0};
#define	LOCKPTR	&stackLock

/*
 * Stack list element.
 */
typedef struct {
    List_Links	links;
    Address	startAddr;
} StackList;

/*
 * Array ofstack list information.
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
 *      Allocate and initialize a stack for a kernel process.  The stack is
 *	returned so that it looks like it is in the middle of a context switch.
 *
 * Results:
 *      The base of the new stack.
 *
 * Side effects:
 *      The kernels page table is modified to map in the new stack pages.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY int
Vm_GetKernelStack(progCounter, startFunc)
    int		progCounter;	/* Program counter of where to start 
				   execution of new process.  */
    void	(*startFunc)();	/* The function to call when the process
				   first starts executing.  This function will
				   initialize things and then start the
				   process running at progCounter. */
{
    register	Vm_PTE		*ptePtr;
    register	StackList	*stackListPtr;
    register	int		i;
    Vm_VirtAddr			virtAddr;

    LOCK_MONITOR;

    /*
     * Get the first free stack.
     */
    if (List_IsEmpty(freeList)) {
	UNLOCK_MONITOR;
	return(-1);
    }
    stackListPtr = (StackList *) List_First(freeList);
    List_Move((List_Links *) stackListPtr, LIST_ATREAR(activeList));

    /*
     * Need to allocate stack pages and initialize the page table entry.
     * We allocate page table entries for all pages except the first which
     * is protected.
     */
    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = 
	    (((unsigned int) stackListPtr->startAddr) >> vmPageShift) + 1;
    virtAddr.offset = 0;
    for (i = 1, ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);
	 i < numStackPages;
	 i++, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	*ptePtr |= VmPageAllocate(&virtAddr, TRUE);
	vmStat.kernStackPages++;
	VmPageValidate(&virtAddr);
    }

    /*
     * Call hardware dependent routine to initialize the stack.
     */
    Mach_InitStack((int) stackListPtr->startAddr, startFunc,
		    (Address) progCounter);

    UNLOCK_MONITOR;

    return((int) stackListPtr->startAddr);
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
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Vm_FreeKernelStack(stackBase)
    int		stackBase;	/* Virtual address of the stack that is being
				   freed. */
{
    Vm_VirtAddr			virtAddr;
    register	int		i;
    register	Vm_PTE		*ptePtr;
    register	StackList	*stackListPtr;

    LOCK_MONITOR;

    /*
     * Unmap the stack and free its pages.
     */
    virtAddr.segPtr = vm_SysSegPtr;
    virtAddr.page = ((unsigned int) (stackBase) >> vmPageShift) + 1;
    virtAddr.offset = 0;
    for (i = 1, ptePtr = VmGetPTEPtr(vm_SysSegPtr, virtAddr.page);
	 i < numStackPages; 
	 i++, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	vmStat.kernStackPages--;
	VmPageFree(VmGetPageFrame(*ptePtr));
	VmPageInvalidate(&virtAddr);
    }

    /*
     * Put the stack back onto the free list.
     */
    if (List_IsEmpty(activeList)) {
	Sys_Panic(SYS_FATAL, "Vm_FreeKernelStack: active list empty\n");
    }
    stackListPtr = (StackList *) List_First(activeList);
    List_Move((List_Links *) stackListPtr, LIST_ATREAR(freeList));
    stackListPtr->startAddr = (Address) stackBase;

    UNLOCK_MONITOR;
}
