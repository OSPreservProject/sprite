/* 
 * devScsiDisk.c --
 *
 *      SCSI Command formatter for SCSI type 0 (Direct Access Devices.) 
 *	This file implements the BlockDevice interface to SCSI disk.
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

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/RCS/devSCSIDisk.c,v 8.6 89/05/23 09:59:42 mendel Exp Locker: brent $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "scsiDevice.h"
#include "devDiskLabel.h"
#include "devDiskStats.h"
#include "devBlockDevice.h"
#include "stdlib.h"
#include "dev/scsi.h"
#include "boot.h"
#include "dbg.h"

typedef struct DiskMap {
    int	 firstSector;
    int	 sizeInSectors;
} DiskMap;

/*
 * State info for an SCSI Disk.  This gets allocated and filled in by
 * the attach procedure. 
 */
typedef struct ScsiDisk {
    DevBlockDeviceHandle blockHandle; /* Must be FIRST field. */
    ScsiDevice	*devPtr;	      /* SCSI Device we have open. */
    int	        partition;  /* What partition we want. A partition number
			     * of -1 means the whole disk.
			     */
    int sizeInSectors;	    /* The number of sectors on disk */
    DiskMap map[DEV_NUM_DISK_PARTS];	/* The partition map */
    int type;		/* Type of the drive, needed for error checking */
    Sys_DiskStats *diskStatsPtr;	/* Area for disk stats. */	
} ScsiDisk;

typedef struct ScsiDiskCmd {
    ScsiDisk	*diskPtr;	/* Target disk of command. */
    ScsiCmd	scsiCmd;	/* SCSI command to send to disk. */
} ScsiDiskCmd;


#define	SCSI_DISK_SECTOR_SIZE	DEV_BYTES_PER_SECTOR

#define	RequestDone(requestPtr,status,byteCount) \
	((requestPtr)->doneProc)((requestPtr),(status),(byteCount))

static ReturnStatus DiskError();
static Boolean	ScsiDiskIdleCheck();


/*
 *----------------------------------------------------------------------
 *
 * InitDisk --
 *
 *	Initialize the device driver state for a SCSI Disk. 
 *
 * Results:
 *	The ScsiDisk structure. NIL - if there is an error attaching the
 *	disk.
 *
 * Side effects:
 *	The disk's label is read and saved in a ScsiDisk structure.  This
 *	is allocated here.
 *
 *----------------------------------------------------------------------
 */
static ScsiDisk *
InitDisk(devPtr,readLabel)
    ScsiDevice	*devPtr; /* SCSI Handle for device. */
    Boolean	readLabel;   /* TRUE if we should read and fill in label 
			      * fields. */
{
    ScsiDisk	*diskPtr;
    ReturnStatus status;
    diskPtr = (ScsiDisk *) malloc(sizeof(ScsiDisk));
    diskPtr->devPtr = devPtr;

    return(diskPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DiskDoneProc --
 *
 *	Call back routine for request to SCSI Disk. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Request call may be woken up.
 *
 *----------------------------------------------------------------------
 */

static int
DiskDoneProc(scsiCmdPtr, status, statusByte, byteCount, senseLength, 
	     senseDataPtr)
    ScsiCmd	*scsiCmdPtr;	/* Request that finished. */
    ReturnStatus  status;	/* Error of request. */
    unsigned char statusByte;	/* SCSI status byte of request. */
    int		byteCount;	/* Number of bytes transferred. */
    int		senseLength;	/* Length of sense data returned. */
    Address	senseDataPtr;	/* Sense data. */
{
    DevBlockDeviceRequest *requestPtr;
    ScsiDisk	*diskPtr;

    requestPtr = (DevBlockDeviceRequest *) (scsiCmdPtr->clientData);
    diskPtr = ((ScsiDiskCmd *) (requestPtr->ctrlData))->diskPtr;
    status = DiskError(diskPtr, statusByte, senseLength, senseDataPtr);
    RequestDone(requestPtr,status,byteCount);
    return 0;

}

/*
 *----------------------------------------------------------------------
 *
 * SendCmdToDevice --
 *
 *	Translate a Block Device request into and SCSI command and send it
 *	to the disk device.
 *
 * Results:
 *	SUCCESS is the command is sent otherwise a Sprite Error code.
 *
 * Side effects:
 *	Disk may be read or written.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
SendCmdToDevice(diskPtr, requestPtr, firstSector, lengthInSectors)
    ScsiDisk	*diskPtr;
    DevBlockDeviceRequest *requestPtr;
    unsigned int	firstSector;
    unsigned int	lengthInSectors;
{
    int		cmd;
    ScsiDiskCmd	 *diskCmdPtr = (ScsiDiskCmd *) (requestPtr->ctrlData);

    cmd = (requestPtr->operation == FS_READ) ? SCSI_READ : SCSI_WRITE;
    DevScsiGroup0Cmd(diskPtr->devPtr,cmd, firstSector, lengthInSectors,
		      &(diskCmdPtr->scsiCmd));
    diskCmdPtr->scsiCmd.buffer = requestPtr->buffer;
    diskCmdPtr->scsiCmd.bufferLen = lengthInSectors * SCSI_DISK_SECTOR_SIZE;
    diskCmdPtr->scsiCmd.dataToDevice = (cmd == SCSI_WRITE);
    diskCmdPtr->scsiCmd.doneProc = DiskDoneProc;
    diskCmdPtr->scsiCmd.clientData = (ClientData) requestPtr;
    diskCmdPtr->diskPtr = diskPtr;
    (boot_SendSCSICommand)(diskPtr->devPtr,&(diskCmdPtr->scsiCmd));
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DiskError --
 *
 *	Map SCSI errors indicated by the sense data into Sprite ReturnStatus
 *	and error message. This proceedure handles two types of 
 *	sense data Class 0 and class 7.
 *
 * Results:
 *	A sprite error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
DiskError(diskPtr, statusByte, senseLength, senseDataPtr)
    ScsiDisk	 *diskPtr;	/* SCSI disk that's complaining. */
    unsigned char statusByte;	/* The status byte of the command. */
    int		 senseLength;	/* Length of SCSI sense data in bytes. */
    char	 *senseDataPtr;	/* Sense data. */
{
    ReturnStatus status;
    ScsiStatus *statusPtr = (ScsiStatus *) &statusByte;
    ScsiClass0Sense *sensePtr = (ScsiClass0Sense *) senseDataPtr;
    char	*name = diskPtr->devPtr->locationName;
    char	errorString[MAX_SCSI_ERROR_STRING];

    /*
     * Check for status byte to see if the command returned sense
     * data. If no sense data exists then we only have the status
     * byte to look at.
     */
    if (!statusPtr->check) {
	if (SCSI_RESERVED_STATUS(statusByte) || statusPtr->intStatus) {
	    return SUCCESS;
	} 
	if (statusPtr->busy) {
	    return DEV_OFFLINE;
	}
	return SUCCESS;
    }
    if (senseLength == 0) {
	 return DEV_NO_SENSE;
    }
    if (DevScsiMapClass7Sense(senseLength, senseDataPtr,&status, errorString)) {
	return status;
    }

    /*
     * If its not a class 7 error it must be Old style sense data..
     */
    if (sensePtr->error == SCSI_NO_SENSE_DATA) {	    
	status = SUCCESS;
    } else {
	 status = DEV_HARD_ERROR;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * IOControlProc --
 *
 *      Do a special operation on a raw SCSI Disk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static ReturnStatus
IOControlProc(handlePtr, command, inBufSize, inBuffer,
                                 outBufSize, outBuffer)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
    int command;
    int inBufSize;
    char *inBuffer;
    int outBufSize;
    char *outBuffer;
{
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseProc --
 *
 *	Block device release proc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGUSED*/
static ReturnStatus
ReleaseProc(handlePtr)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * BlockIOProc --
 *
 *	Convert a Block device IO request in a SCSI command block and 
 *	submit it to the HBA.
 *
 * Results:
 *	The return code from the I/O operation.
 *
 * Side effects:
 *	The disk write, if operation == FS_WRITE.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
BlockIOProc(handlePtr, requestPtr) 
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
    DevBlockDeviceRequest *requestPtr; /* IO Request to be performed. */
{
    ReturnStatus status;	
    ScsiDisk	*diskPtr = (ScsiDisk *) handlePtr;
    unsigned int	firstSector, lengthInSectors, lastSector;


    /*
     * Insure that the request is within the bounds of the partition.
     */
    firstSector = requestPtr->startAddress/DEV_BYTES_PER_SECTOR;
    lengthInSectors = requestPtr->bufferLen/DEV_BYTES_PER_SECTOR;

    status = SendCmdToDevice(diskPtr, requestPtr, firstSector, lengthInSectors);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevScsiDiskAttach --
 *
 *	Attach a SCSI Disk device to the system.
 *
 * Results:
 *	The DevBlockDeviceHandle of the device.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

DevBlockDeviceHandle *
DevScsiDiskAttach(devicePtr)
    Fs_Device	*devicePtr;	/* The device to attach. */
{
    ScsiDevice	*devPtr;
    ScsiDisk	*diskPtr;

    /*
     * Ask the HBA to set up the path to the device. For the time being
     * we will not sort the disk requests.
     */
    devPtr = DevScsiAttachDevice(devicePtr, DEV_QUEUE_FIFO_INSERT);
    /*
     * Initialize the ScsiDisk structure. We don't need to read the label
     * if the user is opening the device in raw (non partitioned) mode.
     */
    diskPtr = InitDisk(devPtr,0);
      diskPtr->partition =  0;

    diskPtr->blockHandle.blockIOProc = BlockIOProc;
    diskPtr->blockHandle.releaseProc = ReleaseProc;
    diskPtr->blockHandle.IOControlProc = IOControlProc;
    diskPtr->blockHandle.minTransferUnit = SCSI_DISK_SECTOR_SIZE;
    diskPtr->blockHandle.maxTransferSize = devPtr->maxTransferSize;
    return (DevBlockDeviceHandle *) diskPtr;
}


