/* vmBoot.c -
 *
 *	This file contains routines that allocate memory for tables at boot 
 *	time.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "vmMach.h"
#include "vm.h"
#include "vmInt.h"
#include "byte.h"

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
 *	None.
 *
 * Side effects:
 *     	vmMemEnd is set and several variables are set by the machine dependent
 *	boot routine.
 *
 * ----------------------------------------------------------------------------
 */
void
Vm_BootInit()
{
    extern unsigned int end;

    Byte_Zero(sizeof(vmStat), (Address) &vmStat);
    vmNoBootAlloc = FALSE;
    vmMemEnd = (Address) &end;
    /*
     * Make sure that we start on a four byte boundary.
     */
    vmMemEnd = (Address) (((int) vmMemEnd + 3) & ~3);

    VmMach_BootInit(&vm_PageSize, &vmPageShift, &vmPageTableInc,
		    &vmKernMemSize, &vmStat.numPhysPages);
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
