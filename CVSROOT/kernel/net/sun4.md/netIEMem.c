/* netIEMem.c -
 *
 * Routines to manage the control block memory for the ethernet board.  All
 * of the memory lies in one big block.  This block is divided up into equal
 * sized chunks and each chunk is allocated sequentially.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif

#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <netIEInt.h>
#include <vm.h>
#include <vmMach.h>
#include <stdio.h>

static	Address	memAddr;


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
NetIEMemInit(statePtr)
    NetIEState		*statePtr;
{
    if (!statePtr->running) {
	statePtr->memBase = (int) VmMach_NetMemAlloc(NET_IE_MEM_SIZE);
	printf("Initializing Intel memory at 0x%x.\n",statePtr->memBase);
    }
    memAddr = (Address) statePtr->memBase;
    return;
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
NetIEMemAlloc(statePtr)
    NetIEState		*statePtr;
{
    Address	addr;

    addr = memAddr;
    memAddr += NET_IE_CHUNK_SIZE;
    if (memAddr > (Address) (statePtr->memBase) + NET_IE_MEM_SIZE) {
	return((Address) NIL);
    }

    return(addr);
}
