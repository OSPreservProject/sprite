/* netIEMem.c -
 *
 * Routines to manage the memory of the ethernet board. 
 * 
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"
#include "sys.h"
#include "list.h"
#include "vm.h"

/*
 * First available address of buffer RAM.
 */

static	Address	memAddr;	

/*
 *----------------------------------------------------------------------
 *
 * NetIEMemMap --
 *
 * Map the ethernet board registers, memory, and ROM into the kernel's
 * address space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is mapped and netIEstate is updated.
 *
 *----------------------------------------------------------------------
 */

void
NetIEMemMap(slotid)
    unsigned int	slotid;		/* NuBus slot id of ethernet board. */
{
    unsigned int	physAddress;	/* Physical address of board. */

    /*
     * Board starts at 0xfs000000 where s is the 4 bit NuBus slot id and
     * extends to 0xfsffffff
     */
    if (!netIEState.mapped) {
	physAddress = ((0xf0 | (slotid & 0xf)) << 24);
	netIEState.deviceBase = 
		VmMach_MapInDevice(physAddress,NET_IE_SLOT_SPACE_SIZE);
	netIEState.mapped = TRUE;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEMemInit --
 *
 *	Initialize the control block memory structures.  This involves
 *      allocating the memory and initializing the pointer to the
 *	beginning of free memory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Pointer to beginning of free memory list is initialized.
 *
 *----------------------------------------------------------------------
 */

void
NetIEMemInit()
{

    if (!netIEState.running && netIEState.mapped) {
	netIEState.memBase = netIEState.deviceBase;
	Sys_Printf("Initializing Intel memory.\n");
    }
    memAddr =  netIEState.memBase;
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEMemAlloc --
 *
 *	Return a pointer to the next free chunk of memory.  Return NIL if none
 *      left.
 *
 *
 * Results:
 *	Pointer to next free chunk of memory, NIL if none left.
 *
 * Side effects:
 *	Pointer to beginning of free memory is incremented.
 *
 *----------------------------------------------------------------------
 */

Address
NetIEMemAlloc(size)
	int	size;		/* Size of block to allocate */
{
    Address	addr;

    /*
     * Make request size of mutiple of 4 bytes to remove any possible 
     * alignment problems.
     */
    if (size & 0x3) {
	size = (size & ~0x3) + 4;
    }
    if ((int) (memAddr + size) > NET_IE_FREE_MEM_SIZE) { 
	return((Address) NIL);
    }
    addr = memAddr;
    memAddr += size;

    return(addr);
}
