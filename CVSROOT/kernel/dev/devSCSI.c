/* 
 * devSCSI.c --
 *
 *	SCSI = Small Computer System Interface.
 *	Device driver for the SCSI disk and tape interface.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
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
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "devSCSI.h"
#include "devMultibus.h"
#include "devDiskLabel.h"
#include "devSCSIDisk.h"
#include "devSCSITape.h"
#include "devSCSIWorm.h"
#include "dbg.h"
#include "vm.h"
#include "sys.h"
#include "sync.h"
#include "proc.h"	/* for Mach_SetJump */
#include "fs.h"
#include "mem.h"
#include "sched.h"

/*
 * State for each SCSI controller.
 */
static DevSCSIController *scsi[SCSI_MAX_CONTROLLERS];

/*
 * SetJump stuff needed when probing for the existence of a device.
 */
Mach_SetJumpState scsiSetJumpState;

/*
 * The error codes for class 0-6 sense data are class specific.
 * The follow arrays of strings are used to print error messages.
 */
char *scsiClass0Errors[] = {
    "No sense data",
    "No index signal",
    "No seek complete",
    "Write fault",
    "Drive notready",
    "Drive not selected",
    "No Track 00",
    "Multiple drives selected",
    "No address acknowledged",
    "Media not loaded",
    "Insufficient capacity",
};
char *scsiClass1Errors[] = {
    "ID CRC error",
    "Unrecoverable data error",
    "ID address mark not found",
    "Data address mark not found",
    "Record not found",
    "Seek error",
    "DMA timeout error",
    "Write protected",
    "Correctable data check",
    "Bad block found",
    "Interleave error",
    "Data transfer incomplete",
    "Unformatted or bad format on drive",
    "Self test failed",
    "Defective track (media errors)",
};
char *scsiClass2Errors[] = {
    "Invalid command",
    "Illegal block address",
    "Aborted",
    "Volume overflow",
};
int scsiNumErrors[] = {
    sizeof(scsiClass0Errors) / sizeof (char *),
    sizeof(scsiClass1Errors) / sizeof (char *),
    sizeof(scsiClass2Errors) / sizeof (char *),
    0, 0, 0, 0, 0,
};
char **scsiErrors[] = {
    scsiClass0Errors,
    scsiClass1Errors,
    scsiClass2Errors,
};

int devSCSIDebug = FALSE;

/*
 * Forward declarations.
 */

ReturnStatus	DevSCSITest();
void		DevSCSISetupCommand();


/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIInitController --
 *
 *	Initialize an SCSI controller.  This probes for the existence
 *	of an SCSI controller (of various types).  If found it initializes
 *	the main data structure for the controller and allocates some
 *	associated buffers.
 *
 * Results:
 *	Returns TRUE if the controller is alive.
 *
 * Side effects:
 *	Allocate buffer space associated with the controller.
 *	Do a hardware reset of the controller.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SCSIInitController(cntrlrPtr)
    DevConfigController *cntrlrPtr;	/* Config info for the controller */
{
    DevSCSIController *scsiPtr;		/* SCSI specific state */

    /*
     * Allocate space for SCSI specific state and
     * initialize the controller itself.
     */
    scsiPtr = (DevSCSIController *)malloc(sizeof(DevSCSIController));
    if (cntrlrPtr->space == DEV_OBIO) {
	if (!DevSCSI3ProbeOnBoard(cntrlrPtr->address, scsiPtr)) {
	    free((char *)scsiPtr);
	    return(FALSE);
	}
    } else if (!DevSCSI0Probe(cntrlrPtr->address, scsiPtr) &&
	       !DevSCSI3ProbeVME(cntrlrPtr->address, scsiPtr,
				   cntrlrPtr->vectorNumber)) {
	free((char *)scsiPtr);
	return(FALSE);
    }
    scsi[cntrlrPtr->controllerID] = scsiPtr;
    scsiPtr->number = cntrlrPtr->controllerID;
    scsiPtr->regsPtr = (Address)cntrlrPtr->address;

    (*scsiPtr->resetProc)(scsiPtr);

    /*
     * Allocate the mapped multibus memory to buffers:  one small buffer
     * for sense data, a one sector buffer for the label, and one buffer
     * for reading and writing filesystem blocks.  A physical page is
     * obtained for the sense data and the label.  The general buffer gets
     * mapped just before a read or write.  It has to be twice as large as
     * the maximum transfer size so that an unaligned block can be mapped
     * into it.
     */
    scsiPtr->senseBuffer =
	    (DevSCSISense *)VmMach_DevBufferAlloc(&devIOBuffer,
					       sizeof(DevSCSISense));
    VmMach_GetDevicePage((Address)scsiPtr->senseBuffer);

    scsiPtr->labelBuffer = VmMach_DevBufferAlloc(&devIOBuffer,
					     DEV_BYTES_PER_SECTOR);
    VmMach_GetDevicePage((Address)scsiPtr->labelBuffer);

    scsiPtr->IOBuffer = VmMach_DevBufferAlloc(&devIOBuffer,
            2 * max(FS_BLOCK_SIZE,
		    MAX_WORM_SECTORS_IO * DEV_BYTES_PER_WORM_SECTOR));
    
    /*
     * Initialize synchronization variables and set the controllers
     * state to alive and not busy.
     */
    scsiPtr->flags = SCSI_CNTRLR_ALIVE;
    SYNC_SEM_INIT_DYNAMIC(&scsiPtr->mutex,"scsiPtr->mutex");   
    scsiPtr->IOComplete.waiting = 0;
    scsiPtr->readyForIO.waiting = 0;
    scsiPtr->configPtr = cntrlrPtr;

    scsiPtr->numRecoverableErrors = 0;
    scsiPtr->numHardErrors = 0;
    scsiPtr->numUnitAttns = 0;

    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIIdleCheck --
 *
 *	Check to see if the controller is idle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increments the idle check count and possibly the idle count in
 *	the controller entry.
 *
 *----------------------------------------------------------------------
 */
void
Dev_SCSIIdleCheck(cntrlrPtr)
    DevConfigController *cntrlrPtr;	/* Config info for the controller */
{
    DevSCSIController *scsiPtr;

    scsiPtr = scsi[cntrlrPtr->controllerID];
    if (scsiPtr != (DevSCSIController *)NIL) {
	cntrlrPtr->numSamples++;
	if (!(scsiPtr->flags & SCSI_CNTRLR_BUSY)) {
	    cntrlrPtr->idleCount++;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIInitDevice --
 *
 *	Initialize a device hanging off an SCSI controller.
 *	This keeps track of how many times it is called with
 *	for disks and tapes so that it can properly correlate
 *	filesystem unit numbers to particular devices.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Disks:  The label sector is read and the partitioning of
 *	the disk is set up.  The partitions correspond to device
 *	files of the same type but with different unit number.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SCSIInitDevice(devConfPtr)
    DevConfigDevice *devConfPtr;	/* Config info about the device */
{
    ReturnStatus status;
    DevSCSIController *scsiPtr;	/* SCSI specific controller state */
    DevSCSIDevice *devPtr;	/* Device specific state */

    /*
     * Increment disk/tape/worm index before checking for the controller so the
     * unit numbers match up right.  ie. each controller accounts
     * for DEV_NUM_DISK_PARTS unit numbers, and the unit number is
     * used to index the devDisk array (div DEV_NUM_DISK_PARTS).
     * For example, if a host has two controllers, we don't want the unit
     * numbers of devices on the second to change if the first controller
     * isn't powered up.
     */
    switch(devConfPtr->flags & SCSI_TYPE_MASK) {
	case SCSI_DISK:
	    scsiDiskIndex++;
	    break;
	case SCSI_TAPE:
	    scsiTapeIndex++;
	    break;
	case SCSI_WORM: 
	    scsiWormIndex++;
	    break;
	default:
	    printf("Dev_SCSIInitDevice, unknown SCSI device type <%x>\n",
		devConfPtr->flags);
	    return(FAILURE);
    }

    scsiPtr = scsi[devConfPtr->controllerID];
    if (scsiPtr == (DevSCSIController *)NIL ||
	scsiPtr == (DevSCSIController *)0) {
	return(FALSE);
    }

    devPtr = (DevSCSIDevice *)malloc(sizeof(DevSCSIDevice));
    devPtr->scsiPtr = scsiPtr;
    devPtr->targetID = devConfPtr->slaveID;
    devPtr->LUN = devConfPtr->flags & SCSI_LUN_MASK;
    switch(devConfPtr->flags & SCSI_TYPE_MASK) {
	case SCSI_DISK:
	    status = DevSCSIDiskInit(devPtr);
	    break;
	case SCSI_TAPE:
	    status = DevSCSITapeInit(devPtr);
	    break;
	case SCSI_WORM: 
	    status = DevSCSIWormInit(devPtr);
	    break;
    }
    if (status != SUCCESS) {
	free((Address)devPtr);
	return(FALSE);
    }
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITest --
 *
 *	Test an SCSI device to see if it is ready.
 *
 * Results:
 *	SUCCESS if the device is ok, DEV_OFFLINE otherwise.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITest(devPtr)
    DevSCSIDevice *devPtr;
{
    register ReturnStatus status;
    register DevSCSIController *scsiPtr;

    /*
     * Synchronize with the interrupt handling routine and with other
     * processes that are trying to initiate I/O with this controller.
     * FIX HERE TO ENQUEUE REQUESTS - GOES WITH CONNECT/DIS-CONNECT
     */
    scsiPtr = devPtr->scsiPtr;
    MASTER_LOCK(&scsiPtr->mutex);
    while (scsiPtr->flags & SCSI_CNTRLR_BUSY) {
	Sync_MasterWait(&scsiPtr->readyForIO, &scsiPtr->mutex, FALSE);
    }
    scsiPtr->flags |= SCSI_CNTRLR_BUSY;

	DevSCSISetupCommand(SCSI_TEST_UNIT_READY, devPtr, 0, 0);

	status = (*devPtr->scsiPtr->commandProc)(devPtr->targetID,
		devPtr->scsiPtr, 0, (Address)0, WAIT);
	if (status == DEV_TIMEOUT) {
	    status = DEV_OFFLINE;
	}

    scsiPtr->flags &= ~SCSI_CNTRLR_BUSY;
    Sync_MasterBroadcast(&scsiPtr->readyForIO);
    MASTER_UNLOCK(&scsiPtr->mutex);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSISetupCommand --
 *
 *      Setup a control block for a command.  The control block can then
 *      be passed to DevSCSICommand.  The control block specifies the a
 *      sub-unit to the controller, the command, and the device address of
 *      the transfer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the control block.
 *
 *----------------------------------------------------------------------
 */
void
DevSCSISetupCommand(command, devPtr, blockNumber, numSectors)
    char command;	/* One of six standard SCSI commands */
    DevSCSIDevice *devPtr;	/* Device state */
    int blockNumber;	/* The starting block number for the transfer */
    int numSectors;	/* Number of sectors (or bytes!) to transfer */
{
    register DevSCSIControlBlock *controlBlockPtr;

    devPtr->scsiPtr->devPtr = devPtr;
    controlBlockPtr = &devPtr->scsiPtr->controlBlock;
    bzero((Address)controlBlockPtr,sizeof(DevSCSIControlBlock));
    controlBlockPtr->command = command;
    controlBlockPtr->unitNumber = devPtr->LUN;
    controlBlockPtr->highAddr = (blockNumber & 0x1f0000) >> 16;
    controlBlockPtr->midAddr =  (blockNumber & 0x00ff00) >> 8;
    controlBlockPtr->lowAddr =  (blockNumber & 0x0000ff);
    controlBlockPtr->blockCount =  numSectors;
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIRequestSense --
 *
 *	Do a request-sense command to obtain the sense data that an
 *	SCSI device returns after some error conditions.  Unfortunately,
 *	the format of the sense data varies with different controllers.
 *	The "sysgen" drive on 2/120's has a format described by
 *	the DevSCSITapeSense type, while the shoebox drives use the
 *	more standard error "class 7" format.
 *
 * Results:
 *	SUCCESS if the sense data is benign.
 *
 * Side effects:
 *	Does an SCSI_REQUEST_SENSE command.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSIRequestSense(scsiPtr, devPtr)
    DevSCSIController *scsiPtr;	/* Controller state */
    DevSCSIDevice *devPtr;	/* Device state needed for error checking */
{
    ReturnStatus status = SUCCESS;
    register DevSCSISense *sensePtr = scsiPtr->senseBuffer;
    int command;

    if (scsiPtr->flags & SCSI_GETTING_STATUS) {
	printf("Warning: DevSCSIRequestSense recursed");
    } else {
	/*
	 * The regular SetupCommand procedure is used, although the
	 * "numSectors" parameter needs to be a byte count indicating
	 * the size of the sense data buffer.
	 */
	bzero((Address)sensePtr, sizeof(DevSCSISense));
	scsiPtr->flags |= SCSI_GETTING_STATUS;
	command = scsiPtr->command;
	DevSCSISetupCommand(SCSI_REQUEST_SENSE, devPtr, 0,sizeof(DevSCSISense));
	status = (*scsiPtr->commandProc)(devPtr->targetID, scsiPtr,
			    sizeof(DevSCSISense), (Address)sensePtr, WAIT);
	scsiPtr->command = command;
	scsiPtr->flags &= ~SCSI_GETTING_STATUS;
	if (devPtr->type == SCSI_TAPE &&
	    ((DevSCSITape *)devPtr->data)->type == SCSI_UNKNOWN) {
	    /*
	     * Heuristically determine the drive type by examining the
	     * amount of data returned.
	     */
	    DevSCSITapeType(sizeof(DevSCSISense) - scsiPtr->residual,
			((DevSCSITape *)devPtr->data));
	}
	status = (*devPtr->errorProc)(devPtr, sensePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIIntr --
 *
 *	Handle interrupts from the SCSI controller.  This has to poll
 *	through the possible SCSI controllers to find the one generating
 *	the interrupt.  The usual action is to wake up whoever is waiting
 *	for I/O to complete.  This may also start up another transaction
 *	with the controller if there are things in its queue.
 *
 * Results:
 *	TRUE if an SCSI controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
Boolean
Dev_SCSIIntr()
{
    int index;
    register DevSCSIController *scsiPtr;
    register int serviced;

    for (index = 0; index < SCSI_MAX_CONTROLLERS ; index++) {
	scsiPtr = scsi[index];
	if (scsiPtr != (DevSCSIController *)NIL) {
	    serviced = (*scsiPtr->intrProc)(scsiPtr);
	    if (serviced) {
		return(TRUE);
	    }
	}
    }
    return(FALSE);
}
