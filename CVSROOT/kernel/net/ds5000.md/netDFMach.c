/* 
 * netDFMach.c --
 *
 *	Machine dependent routines for the DEC FDDI controller.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif not lint

#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <netInt.h>
#include <vm.h>
#include <vmMach.h>
#include <mach.h>
#include <machMon.h>
#include <dbg.h>
#include <assert.h>

#include <netDFInt.h>

/*
 *----------------------------------------------------------------------
 *
 * NetDFMachInit --
 *
 *	Verify that the interface exists and set up the machine dependent
 *	state.
 *
 * Results:
 *	SUCCESS if the FDDI controller was found and initialized,
 *	FAILURE otherwise.
 *
 * Side effects:
 *	Initializes the NetDFState record, as well as the chip.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
NetDFMachInit(interPtr, statePtr)
    Net_Interface	*interPtr; 	/* Network interface. */
    NetDFState		*statePtr;	/* State structure. */
{
    char		*slotAddr;
    ReturnStatus 	status;
    int			slot;
    Mach_SlotInfo	slotInfo;
    static char		*vendor = "DEC";
    static char		*module = "PMAF-AA";

    slot = (int) interPtr->ctrlAddr;
    slotAddr = (char *) MACH_IO_SLOT_ADDR(slot);

    statePtr->slotAddr = (volatile char *)MACH_IO_SLOT_ADDR(slot);
    /*
     * Locate the registers
     */
    statePtr->regReset = (volatile Net_DFReg *) 
	(slotAddr + NET_DF_MACH_RESET_OFFSET);
    statePtr->regCtrlA = (volatile Net_DFReg *) 
	(slotAddr + NET_DF_MACH_CTRLA_OFFSET);
    statePtr->regCtrlB = (volatile Net_DFReg *) 
	(slotAddr + NET_DF_MACH_CTRLB_OFFSET);
    statePtr->regEvent = (volatile Net_DFReg *) 
	(slotAddr + NET_DF_MACH_INT_EVENT_OFFSET);
    statePtr->regMask = (volatile Net_DFReg *) 
	(slotAddr + NET_DF_MACH_INT_MASK_OFFSET);
    statePtr->regStatus = (volatile Net_DFReg *) 
	(slotAddr + NET_DF_MACH_STATUS_OFFSET);
    /*
     * Locate the command ring
     */
    statePtr->comRingPtr = (volatile NetDFCommandDesc *)
	(slotAddr + NET_DF_MACH_COMMAND_RING_OFFSET);
    statePtr->comNextPtr = statePtr->comRingPtr;
    statePtr->comLastPtr = statePtr->comRingPtr +
	(NET_DF_NUM_COMMAND_DESC - 1);

    statePtr->comBufPtr = (volatile NetDFCommandBuf *)
	(slotAddr + NET_DF_MACH_COMMAND_BUF_OFFSET);

    /*
     * Locate the error log
     */
    statePtr->errLogPtr = (volatile unsigned char *)
	(slotAddr + NET_DF_MACH_ERROR_LOG_OFFSET);
    /*
     * Locate the UNSOLICITED ring
     */
    statePtr->unsolFirstDescPtr = (NetDFUnsolDesc *)
	(slotAddr + NET_DF_MACH_UNSOL_RING_OFFSET);
    statePtr->unsolNextDescPtr = statePtr->unsolFirstDescPtr;
    statePtr->unsolLastDescPtr = statePtr->unsolFirstDescPtr +
	(NET_DF_MACH_NUM_UNSOL_DESC - 1);

    /*
     * Locate the buffers for the RMC XMT ring
     */
    statePtr->rmcXmtFirstBufPtr = (NetDFRmcXmtBuf *)
	(slotAddr + NET_DF_MACH_RMC_XMT_BUF_OFFSET);

    /*
     * Check that the device exists and is an FDDIcontroller 700 interface.
     */
    status = Mach_GetSlotInfo(slotAddr + NET_DF_MACH_OPTION_ROM_OFFSET, 
			      &slotInfo);

    if (status != SUCCESS) {
	return status;
    }

    if (strcmp(slotInfo.vendor, vendor) || strcmp(slotInfo.module, module)) {
	return FAILURE;
    } 

    interPtr->name = module;
    Mach_SetIOHandler(slot, Net_Intr, (ClientData) interPtr);
    /*
     * Get the FDDI address from the results of the INIT command
     */
    return (SUCCESS); 
}

