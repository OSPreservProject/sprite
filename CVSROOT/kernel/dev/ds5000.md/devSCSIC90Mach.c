/* 
 * devSCSIC90Mach.c --
 *
 *	Routines specific to the SCSI NCR 53C9X Host Adaptor which
 *	depend on the machine architecture.
 *
 * Copyright 1991 Regents of the University of California
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
#endif /* not lint */

#include "sprite.h"
#include "scsiC90.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "sync.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bstring.h"
#include "devSCSIC90.h"
#include "devSCSIC90Int.h"
#include "dbg.h"

extern Boolean DevEntryAvailProc();


/*
 *----------------------------------------------------------------------
 *
 * DevReset --
 *
 *	Reset a SCSI bus controlled by the SCSI-3 Sun Host Adaptor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the controller and SCSI bus.
 *
 *----------------------------------------------------------------------
 */
void
DevReset(ctrlPtr)
    Controller *ctrlPtr;
{
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    Device *devPtr;
    int i,j;

    if (devSCSIC90Debug > 3) {
	printf("Reset\n");
    }
    /* Reset scsi controller. */
    regsPtr->scsi_ctrl.write.command = CR_RESET_CHIP;
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_NOP;
    /*
     * Don't interrupt when the SCSI bus is reset. Set our bus ID to 7.
     */
    regsPtr->scsi_ctrl.write.config1 |= C1_REPORT | 0x7;
    regsPtr->scsi_ctrl.write.command = CR_RESET_BUS;
    for (i=0; i<8; i++) {
	for (j=0; j<8; j++) {
	    devPtr = ctrlPtr->devicePtr[i][j];
	    if ((devPtr != (Device *)NIL) && (devPtr != (Device *)0)) {
		devPtr->synchPeriod = 0;
		devPtr->synchOffset = 0;
	    }
	}
    }
    /*
     * We initialize configuration, clock conv, synch offset, etc, in
     * SendCommand.
     * Parity is disabled by hardware reset or software.
     */

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * DevStartDMA --
 *
 *	Issue the sequence of commands to the controller to start DMA.
 *	This can be called by Dev_SCSIC90Intr in response to a DATA_{IN,OUT}
 *	phase message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	DMA is enabled.  No registers other than the control register are
 *	to be accessed until DMA is disabled again.
 *
 *----------------------------------------------------------------------
 */
void
DevStartDMA(ctrlPtr)
    Controller *ctrlPtr;
{
    volatile CtrlRegs	*regsPtr;
    int			size;
    Device              *devPtr = ctrlPtr->devPtr;
    Address             buffer;

    size = devPtr->activeBufLen;
    buffer = devPtr->activeBufPtr;

    if (devSCSIC90Debug > 4) {
	printf("StartDMA called for %s, dma %s, size = %d.\n", ctrlPtr->name,
	    (devPtr->dmaState == DMA_RECEIVE) ? "receive" :
		((devPtr->dmaState == DMA_SEND) ? "send" :
						  "not-active!"), size);
    }
    if (devPtr->dmaState == DMA_INACTIVE) {
	printf("StartDMA: Returning, since DMA state isn't active.\n");
	return;
    }
    regsPtr = ctrlPtr->regsPtr;
    if (buffer == (Address) NIL) {
	panic("DMA buffer was NIL before dma.\n");
    }
    if (devPtr->dmaState == DMA_RECEIVE) {
	*ctrlPtr->dmaRegPtr = 0;
    } else {
	bcopy((char *) buffer, ctrlPtr->buffer, size);
	*ctrlPtr->dmaRegPtr = (unsigned int) DMA_WRITE;
    }
    /*
     * Put transfer size in counter.  If this is 16k (max size), this puts
     * a 0 in the counter, which is the correct thing to do.
     */
    /* High byte of size. */
    regsPtr->scsi_ctrl.write.xCntHi = (unsigned char) ((size & 0xff00) >> 8);
    /* Low byte of size. */
    regsPtr->scsi_ctrl.write.xCntLo = (unsigned char) (size & 0x00ff);
    /* Load count into counter by writing a DMA NOP command on C90 only */
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_NOP;
    /* Start scsi command. */
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_XFER_INFO;

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSIC90Init --
 *
 *	Check for the existant of the Sun SCSIC90 HBA controller. If it
 *	exists allocate data stuctures for it.
 *
 * Results:
 *	TRUE if the controller exists, FALSE otherwise.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *----------------------------------------------------------------------
 */
ClientData
DevSCSIC90Init(ctrlLocPtr)
    DevConfigController	*ctrlLocPtr;	/* Controller location. */
{
    int			ctrlNum;
    Boolean		found;
    Controller 		*ctrlPtr;
    int			i,j;
    Mach_SlotInfo	slotInfo;
    char		*slotAddr;
    static char		*vendor = "DEC";
    static char		*module = "PMAZ-AA";
    ReturnStatus	status;
    static int numSCSIC90Controllers = 0; /* highest controller we've
					   * probed for */

    slotAddr = (char *) MACH_IO_SLOT_ADDR(ctrlLocPtr->slot);

    status = Mach_GetSlotInfo(slotAddr + ROM_OFFSET, &slotInfo);
    if (status != SUCCESS) {
	return DEV_NO_CONTROLLER;
    }
    if (strcmp(slotInfo.vendor, vendor) || strcmp(slotInfo.module, module)) {
	return DEV_NO_CONTROLLER;
    }
    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    ctrlNum = ctrlLocPtr->controllerID;
    if (ctrlNum >= MAX_SCSIC90_CTRLS) {
	printf("DevSCSIC90Init: too many controllers\n");
	return DEV_NO_CONTROLLER;
    }
    if (ctrlNum+1 > numSCSIC90Controllers) {
	numSCSIC90Controllers = ctrlNum+1;
    }
    Controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (volatile CtrlRegs *) (slotAddr + REG_OFFSET);
    ctrlPtr->dmaRegPtr = (volatile DMARegister *) (slotAddr + DMA_OFFSET);
    ctrlPtr->buffer = slotAddr + BUFFER_OFFSET;
    ctrlPtr->name = ctrlLocPtr->name;
    ctrlPtr->slot = ctrlLocPtr->slot;
    Sync_SemInitDynamic(&(ctrlPtr->mutex), ctrlPtr->name);
    printf("SCSI controller \"%s\" in slot %d (%s %s %s %s)\n",
	ctrlPtr->name, ctrlPtr->slot, slotInfo.module, slotInfo.vendor, 
	slotInfo.revision, slotInfo.type);
    /* 
     * Initialized the name, device queue header, and the master lock.
     * The controller comes up with no devices active and no devices
     * attached.  Reserved the devices associated with the 
     * targetID of the controller (7).
     */
    ctrlPtr->devPtr = (Device *)NIL;
    ctrlPtr->interruptDevPtr = (Device *)NIL;
    ctrlPtr->devQueuesMask = 0;
    ctrlPtr->devQueues = Dev_CtrlQueuesCreate(&(ctrlPtr->mutex),
	    DevEntryAvailProc);
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    ctrlPtr->devicePtr[i][j] = (i == 7) ? (Device *) 0 : (Device *) NIL;
	}
    }
    Controllers[ctrlNum] = ctrlPtr;
    Mach_SetIOHandler(ctrlPtr->slot, (void (*)()) DevSCSIC90Intr, 
	(ClientData) ctrlPtr);
    DevReset(ctrlPtr);

    if (devSCSIC90Debug > 3) {
	printf("devSCSIC90Init: controller 0x%02x initialized.\n", ctrlNum);
    }

    return (ClientData) ctrlPtr;
}


