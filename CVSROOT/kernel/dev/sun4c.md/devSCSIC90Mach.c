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
#include "devAddrs.h"
#include "scsiC90.h"
#include "mach.h"
#include "machMon.h"
#include "dev.h"
#include "devInt.h"
#include "scsiHBA.h"
#include "scsiDevice.h"
#include "sync.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "devSCSIC90.h"
#include "devSCSIC90Int.h"

extern Boolean DevEntryAvailProc();
/*
 * Forward declarations.  
 */

static Boolean          ProbeSCSI _ARGS_ ((int address));

volatile DMARegs	*dmaRegsPtr = (volatile DMARegs *) NIL;
int	dmaControllerActive = 0;

static int scsiInitiatorID = 7;


/*
 *----------------------------------------------------------------------
 *
 * ProbeSCSI --
 *
 *	Test for the existence for the interface.
 *
 * Results:
 *	TRUE if the host adaptor was found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
ProbeSCSI(address)
    int address;			/* Alledged controller address */
{
    ReturnStatus	status;
    volatile CtrlRegs	*regsPtr = (volatile CtrlRegs *) address;
    int			x;

    /*
     * Touch the device's status register.  Should I read something else?
     */
    status = Mach_Probe(sizeof (regsPtr->scsi_ctrl.read.status),
	    (Address) &(regsPtr->scsi_ctrl.read.status), (Address) &x);
    if (status != SUCCESS) {
	if (devSCSIC90Debug > 3) {
	    printf("SCSIC90 not found at address 0x%x\n",address);
	}
        return (FALSE);
    }
    if (devSCSIC90Debug > 3) {
	printf("SCSIC90 found\n");
    }
    return(TRUE);
}

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

    /* Reset scsi controller. */
    regsPtr->scsi_ctrl.write.command = CR_RESET_CHIP;
    MACH_DELAY(200);
    regsPtr->scsi_ctrl.write.command = CR_DMA | CR_NOP;
    MACH_DELAY(200);
    dmaControllerActive = 0;		/* Allow dma reset to happen. */
    Dev_ScsiResetDMA();
    MACH_DELAY(200);

    regsPtr->scsi_ctrl.write.config1 |= C1_REPORT | scsiInitiatorID;
    MACH_DELAY(200);
    regsPtr->scsi_ctrl.write.command = CR_RESET_BUS;
    MACH_DELAY(800);
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
 * Dev_ScsiResetDMA --
 *
 *	Reset the DMA controller.  The SCSI module owns the dma controller,
 *	so it gets to decide when it may be reset or not.  The network
 *	module also calls us to try to reset it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	DMA chip reset.
 *
 *----------------------------------------------------------------------
 */
void
Dev_ScsiResetDMA()
{
    static	int	whichTime = 0;

    if (whichTime > 1) {
	return;
    }

    whichTime++;

    if (dmaControllerActive > 0 && devSCSIC90Debug > 4) {
	printf("Wanted to reset dma controller, but it was active: %d\n",
		dmaControllerActive);
	return;
    }
    if (dmaRegsPtr == (DMARegs *) NIL) { 
	if (Mach_MonSearchProm("dma", "address", (char *)&dmaRegsPtr,
		sizeof dmaRegsPtr) != sizeof dmaRegsPtr) {
            MachDevReg reg;
	    Address phys;

	    Mach_MonSearchProm("dma", "reg", (char *)&reg, sizeof reg);
	    if (romVectorPtr->v_romvec_version < 2
		    && reg.addr >= (Address)SBUS_BASE
		    && reg.bustype == 1) {          /* old style */
		phys = reg.addr;
	    } else {                                /* new style */
		phys = reg.addr + SBUS_BASE +
		       reg.bustype * SBUS_SIZE;
	    }
	    dmaRegsPtr = (DMARegs *) VmMach_MapInDevice(phys, 1);
	}
    }

    /* Reset dma controller. */
    dmaRegsPtr->ctrl = DMA_RESET;
    MACH_DELAY(200);
    /* Reset the dma reset bit. */
    dmaRegsPtr->ctrl = dmaRegsPtr->ctrl & ~(DMA_RESET);
    /* Allow dma interrupts. */
    dmaRegsPtr->ctrl = DMA_INT_EN;
    MACH_DELAY(200);

    if (devSCSIC90Debug > 4) {
	printf("Returning from Reset command.\n");
    }

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
    /*
     * A DMA cannot cross a 16Mbyte boundary using this dma controller.
     * We could remap pages if it does, but since the file system won't
     * do this, we just panic for now.
     */
    if (((unsigned) buffer & 0xff000000) !=
	    (((unsigned) buffer + size - 1) & 0xff000000)) {
	panic("DMA crosses 16Mbyte boundary.\n");
    }
    if (buffer == (Address) NIL) {
	panic("DMA buffer was NIL before dma.\n");
    }
    dmaRegsPtr->addr = (unsigned int) buffer;

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
    /* Enable DMA */
    if (devPtr->dmaState == DMA_RECEIVE) {
	dmaRegsPtr->ctrl = DMA_EN_DMA | DMA_READ | DMA_INT_EN;
    } else {
	dmaRegsPtr->ctrl = DMA_EN_DMA | DMA_INT_EN;
    }
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
    int	ctrlNum;
    Boolean	found;
    Controller *ctrlPtr;
    int	i,j;
    static int numSCSIC90Controllers = 0; /* highest controller we've
					   * probed for */

    /*
     * See if the controller is there. 
     */
    ctrlNum = ctrlLocPtr->controllerID;
    found =  ProbeSCSI(ctrlLocPtr->address);
    if (!found) {
	return DEV_NO_CONTROLLER;
    }
    if (Mach_MonSearchProm("options", "scsi-initiator-id",
	(char *)&scsiInitiatorID, sizeof(int)) != sizeof(int)) {
	scsiInitiatorID = 7;
    }


    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    if (ctrlNum+1 > numSCSIC90Controllers) {
	numSCSIC90Controllers = ctrlNum+1;
    }
    Controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (volatile CtrlRegs *) (ctrlLocPtr->address);
    ctrlPtr->name = ctrlLocPtr->name;
    Sync_SemInitDynamic(&(ctrlPtr->mutex), ctrlPtr->name);
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
	    ctrlPtr->devicePtr[i][j] =
		(i == 7) ? (Device *) 0 : (Device *) NIL;
	}
    }
    Controllers[ctrlNum] = ctrlPtr;
    DevReset(ctrlPtr);

    if (devSCSIC90Debug > 3) {
	printf("devSCSIC90Init: controller 0x%02x initialized.\n", ctrlNum);
    }

    return (ClientData) ctrlPtr;
}
