/* vmSunBoot.c -
 *
 *	This file contains routines that allocate memory for tables at boot 
 *	time.  This contains some hardware dependencies when it initializes
 *	the virtual address space for the kernel.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "vmSunConst.h"
#include "sprite.h"
#include "machine.h"
#include "sys.h"
#include "vm.h"
#include "vmInt.h"

Address	vmMemEnd;
Boolean	vmNoBootAlloc = TRUE;


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_BootInit --
 *
 *	Initialize virtual memory and the variable that determines 
 *	where to start allocating memory at boot time.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     vmMemEnd is set.
 *
 * ----------------------------------------------------------------------------
 */

void
Vm_BootInit()
{
    Vm_PTE	pte;
    int 	i;
    int		virtAddr;

    /*
     * We map all of the kernel memory that we might need (VM_NUM_KERN_PAGES
     * worth) one for one.  We know that the monitor maps the first part of
     * memory one for one but for some reason it doesn't map enough.  We
     * assume that the pmegs have been mapped correctly.
     */

    *(int *) &pte = 0;
    pte.protection = VM_KRW_PROT;
    pte.resident = 1;
    for (i = 0, virtAddr = MACH_KERNEL_START; 
	 i < VM_NUM_KERN_PAGES * VM_CLUSTER_SIZE;
	 i++, virtAddr += VM_PAGE_SIZE_INT) {
        pte.pfNum = i;
        Vm_SetPageMap((Address) virtAddr, pte);
    }

    vmNoBootAlloc = FALSE;

    /*
     * Determine where memory ends.
     */

    vmMemEnd = (Address) &endBss;

    /*
     * Make sure that we start on a four byte boundary.
     */

    vmMemEnd = (Address) (((int) vmMemEnd + 3) & ~3);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Vm_BootAlloc --
 *
 *     Allocate a block of memory of the given size starting at the
 *     current end of kernel memory.
 *
 * Results:
 *     A pointer to the allocated memory.
 *
 * Side effects:
 *     vmMemEnd is incremented.
 *
 * ----------------------------------------------------------------------------
 */

Address
Vm_BootAlloc(numBytes)
{
    Address	addr;

    if (vmNoBootAlloc) {
	Sys_Panic(SYS_FATAL, "Trying to use Vm_BootAlloc either before calling Vm_BootAllocInit\r\nor after calling Vm_Init\r\n");
	addr = 0;
	return(addr);
    }

    addr =  vmMemEnd;

    vmMemEnd += (numBytes + 3) & ~3;

    return(addr);
}
