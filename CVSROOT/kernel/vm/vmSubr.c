/* vmSubr.c -
 *
 *     This file contains miscellaneous virtual memory routines.
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
#include "lock.h"
#include "sync.h"
#include "sys.h"
#include "list.h"
#include "sunMon.h"
#include "byte.h"
#include "dbg.h"

/*
 * Declarations of external variables
 */

Vm_Stat		vmStat;
int             vmFirstFreePage;  
Address		vmMemEnd;
Sync_Lock 	vmMonitorLock = {0, 0};


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_Init --
 *
 *     Initialize all virtual memory data structures.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     All virtual memory linked lists and arrays are initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Vm_Init()
{
    /*
     * Zero out the statistics structure.
     */
    Byte_Zero(sizeof(vmStat), (Address) &vmStat);

    /*
     * Allocate the segment table and core map.
     */

    VmSegTableAlloc();
    VmCoreMapAlloc();

    /*
     * Call machine dependent routine to allocate tables.
     */

    VmMachAllocTables();
    
    /*
     * Can no longer use Vm_BootAlloc
     */

    vmNoBootAlloc = TRUE;

    /* 
     * Determine how many physical pages that we have used.
     */

    vmFirstFreePage = 
		((int) vmMemEnd - MACH_KERNEL_START - 1) / VM_PAGE_SIZE + 1;

    /*
     * Initialize the segment table and core map.
     */
    VmSegTableInit();
    VmCoreMapInit();

    /*
     * Now call the hardware dependent routine.
     */
    VmMachInit();
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_ProcInit --
 *
 *     Initialize virtual info for this process.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Virtual memory information for the given process is initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Vm_ProcInit(procPtr)
    Proc_ControlBlock	*procPtr;
{
    int			i;

    if (procPtr->vmPtr == (Vm_ProcInfo *)NIL) {
	procPtr->vmPtr = (Vm_ProcInfo *)Mem_Alloc(sizeof(Vm_ProcInfo));
	procPtr->vmPtr->machPtr = (struct VmMachProcInfo *)Mem_Alloc(sizeof(VmMachProcInfo));
    }
    for (i = 0; i < VM_NUM_SEGMENTS; i++) {
	procPtr->vmPtr->segPtrArray[i] = (Vm_Segment *)NIL;
    }
    procPtr->vmPtr->vmFlags = 0;
    procPtr->vmPtr->machPtr->context = VM_INV_CONTEXT;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_RawAlloc --
 *
 *	Allocate bytes of memory.
 *
 * Results:
 *	Pointer to beginning of memory allocated..
 *
 * Side effects:
 *	Variable that indicates the end of kernel memory is modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY Address
Vm_RawAlloc(numBytes)
{
    Address 		retAddr;
    int 		maxAddr;
    int 		lastPage;
    Vm_PTE		*ptePtr;
    VmVirtAddr		virtAddr;
    register Vm_Segment	*segPtr;

    LOCK_MONITOR;

    /*
     * We return the current end of memory as our new address.
     */

    if (numBytes > 100 * 1024) {
	Sys_Printf("\nvmMemEnd = 0x%x - ", vmMemEnd);
	Sys_Panic(SYS_WARNING, "VmRawAlloc asked for >100K\n");
    }
    retAddr = vmMemEnd;

    /*
     * Bump the end of memory by the number of bytes that we just
     * allocated making sure that it is four byte aligned.
     */

    vmMemEnd += (numBytes + 3) & ~3;

    /*
     * Panic if we just ran off the end of memory.
     */

    if (((int) vmMemEnd) > VM_MEM_END_ADDR) {
	Sys_Printf("vmMemEnd = 0x%x - ", vmMemEnd);
	Sys_Panic(SYS_FATAL, "Vm_RawAlloc: Out of memory.\n");
    }

    segPtr = vmSysSegPtr;
    virtAddr.segPtr = segPtr;
    lastPage = segPtr->numPages + segPtr->offset - 1;
    maxAddr = (lastPage + 1) * VM_PAGE_SIZE - 1;
    ptePtr = VmGetPTEPtr(segPtr, lastPage);

    /*
     * Add new pages to the virtual address space until we have added
     * enough to handle this memory request.  Note that we don't allow
     * VmPageAllocate to block if it encounters lots of dirty pages.
     * Better hope that not all of memory is dirty.
     */

    while ((int) (vmMemEnd) - 1 > maxAddr) {
	int	page;

	maxAddr += VM_PAGE_SIZE;
	lastPage++;
	VmIncPTEPtr(ptePtr, 1);
	ptePtr->protection = VM_KRW_PROT;
	virtAddr.page = lastPage;
	virtAddr.offset = 0;
	page = VmPageAllocateInt(&virtAddr, FALSE);
	if (page == -1) {
	    /*
	     * The normal page allocation mechanism failed so go to the
	     * list of pages that are held in reserve for just such an
	     * occasion.
	     */
	    page = VmGetReservePage(&virtAddr);
	    if (page == -1) {
		Sys_Panic(SYS_FATAL, "VmRawAlloc: No memory available\n");
	    }
	}
	ptePtr->pfNum = VmVirtToPhysPage(page);
	VmPageValidateInt(&virtAddr);
	segPtr->numPages++;
    }

    UNLOCK_MONITOR;

    return(retAddr);
}
