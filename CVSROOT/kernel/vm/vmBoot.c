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

#include <sprite.h>
#include <sys.h>
#include <vm.h>
#include <vmInt.h>
#include <bstring.h>

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

    /* 
     * Don't bother initializing vmStat.minFSPages.  During booting it
     * will get set to 0 or 1, which isn't interesting.  So we will
     * put something in the bootcmds script to set minFSPages to the
     * current cache size after the system has finished booting.
     */
    bzero((Address) &vmStat, sizeof(vmStat));

    vmNoBootAlloc = FALSE;
    vmMemEnd = (Address) &end;
    /*
     * Make sure that we start on a four byte boundary.
     */
#ifdef sun4		/* temporary test - this will not last */
    vmMemEnd = (Address) (((int) vmMemEnd + 7) & ~7);	/* double-word bound. */
#else
    vmMemEnd = (Address) (((int) vmMemEnd + 3) & ~3);
#endif /* sun4 */

    VmMach_BootInit(&vm_PageSize, &vmPageShift, &vmPageTableInc,
		    &vmKernMemSize, &vmStat.numPhysPages, &vmMaxMachSegs,
		    &vmMaxProcesses);
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
int numBytes;
{
    Address	addr;

    if (vmNoBootAlloc) {
	panic("Trying to use Vm_BootAlloc either before calling Vm_BootAllocInit\r\nor after calling Vm_Init\r\n");
	addr = 0;
	return(addr);
    }
    addr =  vmMemEnd;
#ifdef sun4	/* temporary test - this will not last */
    vmMemEnd += (numBytes + 7) & ~7;	/* double-word boundary */
#else
    vmMemEnd += (numBytes + 3) & ~3;
#endif /* sun4 */
    return(addr);
}
