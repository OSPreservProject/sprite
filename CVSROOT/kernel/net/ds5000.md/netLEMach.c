/* 
 * netLEMach.c --
 *
 *	Machine dependent routines for the LANCE driver. 
 *
 * Copyright 1990 Regents of the University of California
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

#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <netInt.h>
#include <netLEInt.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <machMon.h>
#include <dbg.h>
#include <assert.h>


/*
 *----------------------------------------------------------------------
 *
 * NetLEMachInit --
 *
 *	Verify that the interface exists and set up the machine dependent
 *	state.
 *
 * Results:
 *	SUCCESS if the LANCE controller was found and initialized,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	Initializes the netEtherFuncs record, as well as the chip.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetLEMachInit(interPtr, statePtr)
    Net_Interface	*interPtr; 	/* Network interface. */
    NetLEState		*statePtr;	/* State structure. */
{
    char		*slotAddr;
    int 		i;
    List_Links		*itemPtr;
    short 		value = NET_LE_CSR0_ADDR;
    char		*romPtr;
    char		buffer[32];
    ReturnStatus 	status;
    int			slot;
    Mach_SlotInfo	slotInfo;
    static char		*vendor = "DEC";
    static char		*module = "PMAD-AA";

    slot = (int) interPtr->ctrlAddr;
    slotAddr = (char *) MACH_IO_SLOT_ADDR(slot);


    statePtr->regPortPtr = (volatile NetLE_Reg *) (slotAddr + 
	NET_LE_MACH_RDP_OFFSET);
    /*
     * Check that the device is exists and is a LANCE interface. 
     */
    status = Mach_GetSlotInfo(slotAddr + NET_LE_MACH_DIAG_ROM_OFFSET, 
		    &slotInfo);
    if (status != SUCCESS) {
	return status;
    }
    if (strcmp(slotInfo.vendor, vendor) || strcmp(slotInfo.module, module)) {
	return FAILURE;
    }
    /*
     * Read out the Ethernet address. 
     */
    romPtr = (char *) (slotAddr + NET_LE_MACH_ESAR_OFFSET + 2);
    for (i = 0; i < 6; i++, romPtr += 4) {
	((char *) &statePtr->etherAddress)[i] = *romPtr;
    }

    Mach_SetIOHandler(slot, Net_Intr, (ClientData) interPtr);
    statePtr->bufAddr = ((char *) slotAddr) + NET_LE_MACH_BUFFER_OFFSET;
    statePtr->bufAllocPtr = statePtr->bufAddr + 0x4000;
    statePtr->bufSize = NET_LE_MACH_BUFFER_SIZE - 0x4000;
    (void) Net_EtherAddrToString(&statePtr->etherAddress, buffer);
    interPtr->name = module;
    printf("Ethernet in slot %d, address %s (%s %s %s %s)\n",
	slot,buffer, interPtr->name, vendor, slotInfo.revision, slotInfo.type);
    return (SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * NetLEMemAlloc --
 *
 *	Allocates memory in the network buffer.
 *
 * Results:
 *	Address of the allocated region.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
NetLEMemAlloc(statePtr, size)
    NetLEState		*statePtr;	/* State of the interface. */
    int			size;		/* Amount to allocate. */
{
    Address		start;
    if ((int) statePtr->bufAllocPtr + size > statePtr->bufSize) {
	panic("NetLEBufAlloc: out of memory\n");
    }
    start = statePtr->bufAllocPtr;
    statePtr->bufAllocPtr += size;
    return start;
}

