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
#include "vmSunConst.h"
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
 * There are an array of stack list elements, one per stack.
 */
static	StackList	stackListElements[VM_MAX_PROCESSES];

/*
 * There are two stack lists: in use stacks and free stacks.
 */
List_Links	activeListHdr;
#define	activeList	(&activeListHdr)
List_Links	freeListHdr;
#define	freeList	(&freeListHdr)


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
    VmVirtAddr			virtAddr;
    static	Boolean		init = FALSE;

    LOCK_MONITOR;

    if (!init) {
	unsigned	int	addr;

	init = TRUE;
	List_Init(activeList);
	List_Init(freeList);
	for (i = 0, addr = VM_STACK_BASE_ADDR; 
	     addr < VM_STACK_END_ADDR + VM_PAGE_SIZE;
	     i++, addr += MACH_NUM_STACK_PAGES * VM_PAGE_SIZE) {
	     stackListElements[i].startAddr = (Address) addr;
	     List_Insert((List_Links *) &stackListElements[i], 
			 LIST_ATREAR(freeList));
	}
    }

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
    virtAddr.segPtr = vmSysSegPtr;
    virtAddr.page = 
	    (((unsigned int) stackListPtr->startAddr) >> VM_PAGE_SHIFT) + 1;
    virtAddr.offset = 0;
    for (i = 1, ptePtr = VmGetPTEPtr(vmSysSegPtr, virtAddr.page);
	 i < MACH_NUM_STACK_PAGES;
	 i++, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	ptePtr->protection = VM_KRW_PROT;
	ptePtr->pfNum = VmVirtToPhysPage(VmPageAllocate(&virtAddr, TRUE));
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
    VmVirtAddr			virtAddr;
    register	int		i;
    register	Vm_PTE		*ptePtr;
    register	StackList	*stackListPtr;

    LOCK_MONITOR;

    /*
     * Unmap the stack and free its pages.
     */
    virtAddr.segPtr = vmSysSegPtr;
    virtAddr.page = ((unsigned int) (stackBase) >> VM_PAGE_SHIFT) + 1;
    virtAddr.offset = 0;
    for (i = 1, ptePtr = VmGetPTEPtr(vmSysSegPtr, virtAddr.page);
	 i < MACH_NUM_STACK_PAGES; 
	 i++, VmIncPTEPtr(ptePtr, 1), virtAddr.page++) {
	vmStat.kernStackPages--;
	VmPageFree((int) VmPhysToVirtPage(ptePtr->pfNum));
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
