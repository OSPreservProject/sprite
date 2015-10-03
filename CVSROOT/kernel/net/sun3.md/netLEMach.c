/* 
 * netLEMach.c --
 *
 *	Machine dependent routines for the sun4c LANCE driver. 
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
    Address 	ctrlAddr;	/* Kernel virtual address of controller. */
    char		buffer[32];

    ctrlAddr = interPtr->ctrlAddr;
    /*
     * If the address is physical (not in kernel's virtual address space)
     * then we have to map it in.
     */
    if (interPtr->virtual == FALSE) {
	ctrlAddr = (char *) VmMach_MapInDevice(ctrlAddr, 1);
    }
    /*
     * The onboard control register is at a pre-defined kernel virtual
     * address.  The virtual mapping is set up by the sun PROM monitor
     * and passed to us from the netInterface table.
     */

    statePtr->regPortPtr = (volatile NetLE_Reg *) ctrlAddr;
    {
	/*
	 * Poke the controller by setting the RAP.
	 */
	short value = NET_LE_CSR0_ADDR;
	ReturnStatus status;
	status = Mach_Probe(sizeof(short), (char *) &value, 
			  (char *) (((short *)(statePtr->regPortPtr)) + 1));
	if (status != SUCCESS) {
	    return(status);
	}
    } 
    Mach_SetHandler(interPtr->vector, Net_Intr, (ClientData) interPtr);
    /*
     * Get ethernet address out of the rom.  
     */

    Mach_GetEtherAddress(&statePtr->etherAddress);
    (void) Net_EtherAddrToString(&statePtr->etherAddress, buffer);
    printf("%s-%d Ethernet address %s\n",interPtr->name, interPtr->number,
	    buffer);
    return SUCCESS;
}
