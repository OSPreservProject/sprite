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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <stdio.h>
#include <fs.h>
#include <dev.h>
#include <devInt.h>
#include <sys/scsi.h>
#include <scsiDevice.h>
#include <devDiskLabel.h>
#include <devDiskStats.h>
#include <devBlockDevice.h>
#include <stdlib.h>
#include <bstring.h>
#include <dev/scsi.h>
#include <dbg.h>
#include <fsdm.h>

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
    DevDiskStats *diskStatsPtr;	/* Area for disk stats. */	
    int retries;		/* Number of times current command has been
				 * retried. */
} ScsiDisk;

typedef struct ScsiDiskCmd {
    ScsiDisk	*diskPtr;	/* Target disk of command. */
    ScsiCmd	scsiCmd;	/* SCSI command to send to disk. */
} ScsiDiskCmd;


#define	SCSI_DISK_SECTOR_SIZE	DEV_BYTES_PER_SECTOR

#define	RequestDone(requestPtr,status,byteCount) \
	((requestPtr)->doneProc)((requestPtr),(status),(byteCount))

static ReturnStatus DiskError _ARGS_((ScsiDevice *devPtr,
			    ScsiCmd *scsiCmdPtr));
static Boolean	ScsiDiskIdleCheck _ARGS_((ClientData clientData,
			    DevDiskStats *diskStatsPtr));
static int DiskDoneProc _ARGS_((struct ScsiCmd *scsiCmdPtr, 
			    ReturnStatus status, int statusByte, 
			    int byteCount, int senseLength, 
			    Address senseDataPtr));


/*
 *----------------------------------------------------------------------
 *
 * FillInLabel --
 *
 *	Read the label of the disk and record the partitioning info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Define the disk partitions that determine which part of the
 *	disk each different disk device uses.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
FillInLabel(devPtr,diskPtr)
    ScsiDevice		 *devPtr; /* SCSI Handle for device. */
    ScsiDisk		 *diskPtr;  /* Disk state stucture to read label. */
{
    register ReturnStatus	status;
    ScsiCmd			labelReadCmd;
    Sun_DiskLabel		*sunLabelPtr;
    Dec_DiskLabel		*decLabelPtr;
    Fsdm_DiskHeader		*diskHdrPtr;
    char			labelBuffer[SCSI_DISK_SECTOR_SIZE];
    int				byteCount;
    int				part;
    Boolean			printLabel = FALSE;

#ifdef DEBUG
    printLabel = TRUE;
#endif

    /*
     * The label of a SCSI disk normally resides in the first sector. Format
     * and send a SCSI READ command to fetch the sector.
     */
    DevScsiGroup0Cmd(devPtr, SCSI_READ, 0, 1,&labelReadCmd);
    labelReadCmd.buffer = labelBuffer;
    labelReadCmd.dataToDevice = FALSE;
    labelReadCmd.bufferLen = SCSI_DISK_SECTOR_SIZE;
    diskPtr->retries = 0;
    status = DevScsiSendCmdSync(devPtr,&labelReadCmd, &byteCount);
    if ((status == SUCCESS) && (byteCount < sizeof(Sun_DiskLabel))) {
	status = DEV_EARLY_CMD_COMPLETION;
    }
    if (status != SUCCESS) {
	return(status);
    }
    sunLabelPtr = (Sun_DiskLabel *) labelBuffer;
    if (sunLabelPtr->magic == SUN_DISK_MAGIC) {
	/*
	 * XXX - Should really check if label is valid.
	 */
	if (printLabel) {
	    printf("%s: %s\n", devPtr->locationName, sunLabelPtr->asciiLabel);
	}

	diskPtr->sizeInSectors = sunLabelPtr->numSectors * 
			    sunLabelPtr->numHeads * sunLabelPtr->numCylinders;
    
	if (printLabel) {
	    printf(" Partitions ");
	}
	for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
	    diskPtr->map[part].firstSector = sunLabelPtr->map[part].cylinder *
					     sunLabelPtr->numHeads * 
					     sunLabelPtr->numSectors;
	    diskPtr->map[part].sizeInSectors =
					sunLabelPtr->map[part].numBlocks;
	    if (printLabel) {
		printf(" (%d,%d)", diskPtr->map[part].firstSector,
				       diskPtr->map[part].sizeInSectors);
	    }
	}
	if (printLabel) {
	    printf("\n");
	}

	return(SUCCESS);
    }
    /*
     * The disk isn't in SUN format so try Sprite format.
     */
    diskHdrPtr = (Fsdm_DiskHeader *)labelBuffer;
    if (diskHdrPtr->magic == FSDM_DISK_MAGIC) {
	/*
	 * XXX - Should really check if label is valid.
	 */
	if (printLabel) {
	    printf("%s: %s\n", devPtr->locationName, diskHdrPtr->asciiLabel);
	}

	diskPtr->sizeInSectors = diskHdrPtr->numSectors * 
				 diskHdrPtr->numHeads *
			         diskHdrPtr->numCylinders;

	if (printLabel) {
	    printf(" Partitions ");
	}
	for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
	    diskPtr->map[part].firstSector = 
				diskHdrPtr->map[part].firstCylinder *
				diskHdrPtr->numHeads * diskHdrPtr->numSectors;
	    diskPtr->map[part].sizeInSectors =
				diskHdrPtr->map[part].numCylinders *
				diskHdrPtr->numHeads * diskHdrPtr->numSectors;
	    if (printLabel) {
		printf(" (%d,%d)", diskPtr->map[part].firstSector,
				   diskPtr->map[part].sizeInSectors);
	    }
	}
	if (printLabel) {
	    printf("\n");
	}
	return(SUCCESS);
    }
    /*
     * The disk isn't in SUN or Sprite format so try Dec format.
     * We have to read the right sector first.
     */
    DevScsiGroup0Cmd(devPtr, SCSI_READ, DEC_LABEL_SECTOR, 1,&labelReadCmd);
    labelReadCmd.buffer = labelBuffer;
    labelReadCmd.dataToDevice = FALSE;
    labelReadCmd.bufferLen = SCSI_DISK_SECTOR_SIZE;
    diskPtr->retries = 0;
    status = DevScsiSendCmdSync(devPtr,&labelReadCmd, &byteCount);
    if ((status == SUCCESS) && (byteCount < sizeof(Dec_DiskLabel))) {
	status = DEV_EARLY_CMD_COMPLETION;
    }
    if (status != SUCCESS) {
	return(status);
    }
    decLabelPtr = (Dec_DiskLabel *) labelBuffer;
    if (decLabelPtr->magic == DEC_LABEL_MAGIC) {
	/*
	 * XXX - Should really check if label is valid.
	 */
	if (decLabelPtr->spriteMagic != FSDM_DISK_MAGIC) {
	    printf("Disk needs Sprite-modified Dec label\n");
	}
	if (decLabelPtr->version != DEC_LABEL_VERSION) {
	    printf("Disk label version mismatch: %x vs %x\n",
		    decLabelPtr->version, DEC_LABEL_VERSION);
	}
	if (printLabel) {
	    printf("%s: %s\n", devPtr->locationName, decLabelPtr->asciiLabel);
	}

	diskPtr->sizeInSectors = decLabelPtr->numSectors * 
			    decLabelPtr->numHeads * decLabelPtr->numCylinders;
    
	if (printLabel) {
	    printf(" Partitions ");
	}
	for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
	    diskPtr->map[part].firstSector =
		    decLabelPtr->map[part].offsetBytes / DEV_BYTES_PER_SECTOR;
	    diskPtr->map[part].sizeInSectors =
		    decLabelPtr->map[part].numBytes / DEV_BYTES_PER_SECTOR;
	    if (printLabel) {
		printf(" (%d,%d)", diskPtr->map[part].firstSector,
				       diskPtr->map[part].sizeInSectors);
	    }
	}
	if (printLabel) {
	    printf("\n");
	}

	return(SUCCESS);
    }
    return(FAILURE);
}

/*
 *----------------------------------------------------------------------
 *
 * ScsiDiskIdleCheck --
 *
 *	Routine for the Disk Stats module to use to determine the idleness
 *	for a disk.
 *
 * Results:
 *	TRUE if the disk pointed to by clientData is idle, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Boolean
ScsiDiskIdleCheck(clientData, diskStatsPtr) 
    ClientData		clientData;		/* Unused for SCSI disks. */
    DevDiskStats	*diskStatsPtr;
{
    Boolean		retVal;

    MASTER_LOCK(&(diskStatsPtr->mutex));
    retVal = !(diskStatsPtr->busy);
    MASTER_UNLOCK(&(diskStatsPtr->mutex));

    return retVal;

}

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
    ScsiDisk	disk, *diskPtr;
    ReturnStatus status;
    int		retry = 3;
    bzero((char *) &disk, sizeof(ScsiDisk));
    /*
     * Check that the disk is on-line.  
     * We do this check twice because it appears that dec rz55 
     * drives always indicate that they are ready after powerup,
     * even if their not.
     */
    status = DevScsiTestReady(devPtr);
    status = DevScsiTestReady(devPtr);
    if (status != SUCCESS) {
	if (status != DEV_OFFLINE) {
	    return((ScsiDisk *) NIL);
	}
	/*
	 * Do this loop a few times because Quantum drives appear to respond
	 * to the first start request before they are actually on-line.
	 */
	while (retry > 0) {
	    /*
	     * Try and start the unit.
	     */
	    status = DevScsiStartStopUnit(devPtr, TRUE);
	    if (status != SUCCESS) {
		return((ScsiDisk *) NIL);
	    }
	    /*
	     * Make sure the unit is ready.
	     */
	    status = DevScsiTestReady(devPtr);
	    if (status == SUCCESS) {
		break;
	    }
	    retry--;
	}
    }
    disk.devPtr = devPtr;
    if (readLabel) {
	status = FillInLabel(devPtr,&disk);
	if (status != SUCCESS) {
	    return((ScsiDisk *) NIL);
	}
    } 

    /*
     * Return a malloced copy of the structure we filled in.
     */
    diskPtr = (ScsiDisk *) malloc(sizeof(ScsiDisk));
    *diskPtr = disk;
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
/*ARGSUSED*/
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

    MASTER_LOCK(&(diskPtr->diskStatsPtr->mutex));
    diskPtr->diskStatsPtr->busy--;
    if (requestPtr->operation == FS_READ) {
	diskPtr->diskStatsPtr->diskStats.diskReads += 
				byteCount/DEV_BYTES_PER_SECTOR;
    } else {
	diskPtr->diskStatsPtr->diskStats.diskWrites += 
				byteCount/DEV_BYTES_PER_SECTOR;
    }
    MASTER_UNLOCK(&(diskPtr->diskStatsPtr->mutex));

    /*
     * We need to copy the sense data out of the buffer given to us by
     * the HBA into the buffer in ScsiCmd.  Someday we should get rid
     * of all sense buffers except those in the ScsiCmd.  JHH
     */

     bcopy((char *) senseDataPtr, scsiCmdPtr->senseBuffer, senseLength);
     scsiCmdPtr->senseLen = senseLength;
     scsiCmdPtr->statusByte = statusByte;

    /*
     * If request suffered an HBA error or got no error we notify the
     * caller that the request is done.
     */
    if ((status != SUCCESS) || (statusByte == 0)) {
	RequestDone(requestPtr,status,byteCount);
	return 0;
    }
    /*
     * Otherwise we have a SCSI command that returned an error. 
     */
    status = DiskError(diskPtr->devPtr, scsiCmdPtr);
    /*
     * If the device was reset then retry the command.  This isn't quite
     * correct, but works in the majority of cases because most of the
     * time the scsi bus is reset by the device driver.  In reality the
     * bus could be reset because someone replaced one drive with another.
     * If you really want to handle that situation then you have to pass
     * the status up to the higher level code and let it decide what to do.
     * Of course that means you have to handle it in many different places,
     * whereas this solution only requires modification to this routine.
     * Also note that it is possible to go into an infinite loop if the
     * device always returns unit attention.
     * JHH 6/7/91
     */
    if (status == DEV_RESET) {
	if ((diskPtr->retries > 0) && (diskPtr->retries % 10 == 0)) {
	    printf("WARNING: device %s always returns unit attention?\n",
		diskPtr->devPtr->locationName);
	    diskPtr->retries = 0;
	}
	scsiCmdPtr->senseLen = sizeof(scsiCmdPtr->senseBuffer);
	DevScsiSendCmd(diskPtr->devPtr, scsiCmdPtr);
	diskPtr->retries++;
    } else {
	RequestDone(requestPtr,status,byteCount);
    }
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
    ReturnStatus status;

    if (sizeof(ScsiDiskCmd) > sizeof((requestPtr->ctrlData))) {
	panic("ScsiDISK: command block bigger than controller data\n");
	return FAILURE;
    }
    if (firstSector <= 0x1fffff) {
	cmd = (requestPtr->operation == FS_READ) ? SCSI_READ : SCSI_WRITE;
	status = DevScsiGroup0Cmd(diskPtr->devPtr,cmd, firstSector, 
		    lengthInSectors, &(diskCmdPtr->scsiCmd));
	if (status != SUCCESS) {
	    return FAILURE;
	}
    } else {
	ScsiReadExtCmd	*cmdPtr;
	/*
	 * The offset is too big for the standard 6-byte SCSI read and
	 * write commands.  We have to use the extended version.  Perhaps
	 * we should always use the extended version?
	 */
	if (lengthInSectors > 0xffff) {
	    printf("SendCmdToDevice: too many sectors (%d > %d)\n",
		lengthInSectors, 0xffff);
	    return FAILURE;
	}
	bzero((char *) &(diskCmdPtr->scsiCmd), sizeof(ScsiCmd));
	diskCmdPtr->scsiCmd.commandBlockLen = sizeof(ScsiReadExtCmd);
	cmdPtr = (ScsiReadExtCmd *) (diskCmdPtr->scsiCmd.commandBlock);
	cmdPtr->command = (requestPtr->operation == FS_READ) ? 
	    SCSI_READ_EXT : SCSI_WRITE_EXT;
	cmdPtr->unitNumber = diskPtr->devPtr->LUN;
	cmdPtr->highAddr = ((firstSector >> 24) & 0xff);
	cmdPtr->highMidAddr = ((firstSector >> 16) & 0xff);
	cmdPtr->lowMidAddr = ((firstSector >> 8) & 0xff);
	cmdPtr->lowAddr = ((firstSector) & 0xff);
	cmdPtr->highCount = ((lengthInSectors >> 8) & 0xff);
	cmdPtr->lowCount = ((lengthInSectors) & 0xff);
    }
    diskCmdPtr->scsiCmd.buffer = requestPtr->buffer;
    diskCmdPtr->scsiCmd.bufferLen = lengthInSectors * SCSI_DISK_SECTOR_SIZE;
    diskCmdPtr->scsiCmd.dataToDevice = (requestPtr->operation == FS_WRITE);
    diskCmdPtr->scsiCmd.doneProc = DiskDoneProc;
    diskCmdPtr->scsiCmd.clientData = (ClientData) requestPtr;
    diskCmdPtr->scsiCmd.senseLen = sizeof(diskCmdPtr->scsiCmd.senseBuffer);
    diskCmdPtr->diskPtr = diskPtr;

    MASTER_LOCK(&(diskPtr->diskStatsPtr->mutex));
    diskPtr->diskStatsPtr->busy++;
    MASTER_UNLOCK(&(diskPtr->diskStatsPtr->mutex));

    diskPtr->retries = 0;
    DevScsiSendCmd(diskPtr->devPtr,&(diskCmdPtr->scsiCmd));
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
DiskError(devPtr, scsiCmdPtr)
    ScsiDevice	 *devPtr;	/* SCSI device that's complaining. */
    ScsiCmd	*scsiCmdPtr;	/* SCSI command that had the problem. */
{
    unsigned char statusByte = scsiCmdPtr->statusByte;
    ReturnStatus status;
    ScsiStatus *statusPtr = (ScsiStatus *) &statusByte;
    ScsiClass0Sense *sensePtr = (ScsiClass0Sense *) scsiCmdPtr->senseBuffer;
    int	senseLength = scsiCmdPtr->senseLen;
    char	*name = devPtr->locationName;
    char	errorString[MAX_SCSI_ERROR_STRING];

    /*
     * Check for status byte to see if the command returned sense
     * data. If no sense data exists then we only have the status
     * byte to look at.
     */
    if (!statusPtr->check) {
	if (SCSI_RESERVED_STATUS(statusByte) || statusPtr->intStatus) {
	    printf("Warning: SCSI Disk at %s unknown status byte 0x%x\n",
		   name, statusByte);
	    return SUCCESS;
	} 
	if (statusPtr->busy) {
	    return DEV_OFFLINE;
	}
	return SUCCESS;
    }
    if (senseLength == 0) {
	 printf("Warning: SCSI Disk %s error: no sense data\n", name);
	 return DEV_NO_SENSE;
    }
    if (DevScsiMapClass7Sense(senseLength, scsiCmdPtr->senseBuffer,
	    &status, errorString)) {
	if (errorString[0]) {
	     printf("Warning: SCSI Disk %s error: %s\n", name, errorString);
	}
	return status;
    }

    /*
     * If its not a class 7 error it must be Old style sense data..
     */
    if (sensePtr->error == SCSI_NO_SENSE_DATA) {	    
	status = SUCCESS;
    } else {
	int class = (sensePtr->error & 0x70) >> 4;
	int code = sensePtr->error & 0xF;
	int addr;
	addr = (sensePtr->highAddr << 16) |
		(sensePtr->midAddr << 8) |
		sensePtr->lowAddr;
	printf("Warning: SCSI disk at %s sense error (%d-%d) at <%x> ",
			name, class, code, addr);
	if (devScsiNumErrors[class] > code) {
		printf("%s", devScsiErrors[class][code]);
	 }
	 printf("\n");
	 status = DEV_HARD_ERROR;
    }
    return status;
}
/*
 * This code is for the raid people to test out HBA/disks pairs 
 * by bypassing most of Sprite.
 * Mendel 9/12/89
 */

#include <dev/hbatest.h>
/*ARGSUSED*/
static int
DiskHBATestDoneProc(scsiCmdPtr, status, statusByte, byteCount, senseLength, 
	     senseDataPtr)
    ScsiCmd	*scsiCmdPtr;	/* Request that finished. */
    ReturnStatus  status;	/* Error of request. */
    unsigned char statusByte;	/* SCSI status byte of request. */
    int		byteCount;	/* Number of bytes transferred. */
    int		senseLength;	/* Length of sense data returned. */
    Address	senseDataPtr;	/* Sense data. */
{
    ReturnStatus *errorStatusPtr = (ReturnStatus *) (scsiCmdPtr->clientData);
    ScsiStatus *statusPtr = (ScsiStatus *) &statusByte;
    char	errorString[MAX_SCSI_ERROR_STRING];
    ScsiClass0Sense *sensePtr = (ScsiClass0Sense *) senseDataPtr;
    /*
     * Check for status byte to see if the command returned sense
     * data. If no sense data exists then we only have the status
     * byte to look at.
     */
    if ((status == SUCCESS) && !statusPtr->check) {
	return 0;
    }
    if (senseLength == 0) {
	 printf("Warning: SCSI Disk error: no sense data\n");
	 *errorStatusPtr = DEV_NO_SENSE;
	 return DEV_NO_SENSE;
    }
    if (DevScsiMapClass7Sense(senseLength, senseDataPtr,&status, errorString)) {
	if (errorString[0]) {
	     printf("Warning: SCSI Disk  error: %s\n", errorString);
	}
        *errorStatusPtr = status;
	return status;
    }

    /*
     * If its not a class 7 error it must be Old style sense data..
     */
    if (sensePtr->error == SCSI_NO_SENSE_DATA) {	    
	status = SUCCESS;
    } else {
	int class = (sensePtr->error & 0x70) >> 4;
	int code = sensePtr->error & 0xF;
	int addr;
	addr = (sensePtr->highAddr << 16) |
		(sensePtr->midAddr << 8) |
		sensePtr->lowAddr;
	printf("Warning: SCSI disk sense error (%d-%d) at <%x> ",
			class, code, addr);
	if (devScsiNumErrors[class] > code) {
		printf("%s", devScsiErrors[class][code]);
	 }
	 printf("\n");
	 status = DEV_HARD_ERROR;
    }
    *errorStatusPtr = status;
    return 0;
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
IOControlProc(handlePtr, ioctlPtr, replyPtr)
    DevBlockDeviceHandle	*handlePtr; /* Handle pointer of device. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
     ScsiDisk	*diskPtr = (ScsiDisk *) handlePtr;
     ReturnStatus	status = FAILURE;

     if ((ioctlPtr->command & ~0xffff) == IOC_SCSI) {
	 status = DevScsiIOControl(diskPtr->devPtr, ioctlPtr, replyPtr);
	 return status;

     }
     switch (ioctlPtr->command) {
	case	IOC_REPOSITION:
	    /*
	     * Reposition is ok
	     */
	    return(SUCCESS);
	    /*
	     * No disk specific bits are set this way.
	     */
	case	IOC_GET_FLAGS:
	case	IOC_SET_FLAGS:
	case	IOC_SET_BITS:
	case	IOC_CLEAR_BITS:
	    return(SUCCESS);

	case	IOC_GET_OWNER:
	case	IOC_SET_OWNER:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_TRUNCATE:
	    return(GEN_INVALID_ARG);

	case	IOC_LOCK:
	case	IOC_UNLOCK:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_NUM_READABLE:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_MAP:
	    return(GEN_NOT_IMPLEMENTED);

    /*
     * This code is for the raid people to test out HBA/disks pairs 
     * by bypassing most of Sprite.
     * Mendel 9/12/89
     */
	case   IOC_HBA_DISK_IO_TEST: {
	    register int count, i;
	    register DevHBADiskTest  *cmds;
	    ScsiCmd	*scsiCmds;
	    ReturnStatus       errorStatus;
	    static  char *dmaMem, *dmem;
	    char *mem;
	    static int referCount = 0;

	    if ((ioctlPtr->inBufSize % sizeof(DevHBADiskTest)) || 
		!ioctlPtr->inBufSize) {
		return(GEN_INVALID_ARG);
	    }
	    count = ioctlPtr->inBufSize / sizeof(DevHBADiskTest);
	    cmds = (DevHBADiskTest  * ) ioctlPtr->inBuffer;
	    mem = malloc(sizeof(ScsiCmd)*count);
	    if (referCount == 0) { 
		    dmem = malloc(128*1024);
#if defined(sun4) || defined(sun3)
		    dmaMem = VmMach_DMAAlloc(128*1024, dmem);
		    if (dmaMem == (Address) NIL) {
			panic("IOControlProc: unable to allocate dma memory.");
		    }
#else
		    dmaMem = dmem;
#endif
	    } 
	    referCount++;
	    scsiCmds = (ScsiCmd *) (mem);
	    errorStatus = 0;
	    for (i = 0; i < count; i++) {
		DevScsiGroup0Cmd(diskPtr->devPtr, 
			    cmds[i].writeOperation ? SCSI_WRITE : SCSI_READ,
			    cmds[i].firstSector, cmds[i].lengthInSectors,
			    &(scsiCmds[i]));
		scsiCmds[i].buffer = dmaMem;
		scsiCmds[i].bufferLen = cmds[i].lengthInSectors * 
							SCSI_DISK_SECTOR_SIZE;
	        scsiCmds[i].dataToDevice = cmds[i].writeOperation;
		scsiCmds[i].clientData = (ClientData) &errorStatus;
	        scsiCmds[i].doneProc = DiskHBATestDoneProc;
	        scsiCmds[i].senseLen = sizeof(scsiCmds[i].senseBuffer);
		if (i < count - 1) {
		    diskPtr->retries = 0;
		    DevScsiSendCmd(diskPtr->devPtr, &(scsiCmds[i]));
		} else {
		    int	byteCount;
		    diskPtr->retries = 0;
		    status = DevScsiSendCmdSync(diskPtr->devPtr,
			    &(scsiCmds[i]),&byteCount);
		}
	   }
	   referCount--;
	   if (referCount == 0) {
#if defined(sun4) || defined(sun3)
		   VmMach_DMAFree(128*1024, dmaMem);
#endif
		   free(dmem);
	   }
	   free(mem);
	   if (status) {
	       return status;
	    }
	   return errorStatus;

	}
	case   IOC_HBA_DISK_UNIT_TEST: {
	    register int count, i;
	    register ScsiCmd	*scsiCmds;
	    ReturnStatus       errorStatus;

	    if (ioctlPtr->inBufSize != sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    count = *(int *) ioctlPtr->inBuffer;
	    if ((count < 0) || (count > MAX_HBA_UNIT_TESTS)) {
		return(GEN_INVALID_ARG);
	    }
	    scsiCmds = (ScsiCmd *) malloc(sizeof(ScsiCmd)*count);
	    errorStatus = 0;
	    for (i = 0; i < count; i++) {
		DevScsiGroup0Cmd(diskPtr->devPtr, SCSI_TEST_UNIT_READY,
			    0, 0, scsiCmds + i);
		scsiCmds[i].buffer = (char *) NIL;
		scsiCmds[i].bufferLen = 0;
	        scsiCmds[i].dataToDevice = 0;
		scsiCmds[i].clientData = (ClientData) &errorStatus;
	        scsiCmds[i].doneProc = DiskHBATestDoneProc;
	        scsiCmds[i].senseLen = sizeof(scsiCmds[i].senseBuffer);
		if (i < count - 1) {
		    diskPtr->retries = 0;
		    DevScsiSendCmd(diskPtr->devPtr, &(scsiCmds[i]));
		} else {
		    int	byteCount;
		    diskPtr->retries = 0;
		    status = DevScsiSendCmdSync(diskPtr->devPtr,
			    &(scsiCmds[i]), &byteCount);
		}
	   }
	   free((char *) scsiCmds);
	   if (status) {
	       return status;
	    }
	   return errorStatus;

	}
	default:
	    return(GEN_INVALID_ARG);
    }
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
    ReturnStatus status;	
    ScsiDisk	*diskPtr = (ScsiDisk *) handlePtr;

    status = DevScsiReleaseDevice(diskPtr->devPtr);
    DevDiskUnregister(diskPtr->diskStatsPtr);
    free((char *) diskPtr);

    return status;
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
    unsigned int	firstSector, lengthInSectors, sizeInSectors;


    if (!((requestPtr->operation == FS_READ) ||
          (requestPtr->operation == FS_WRITE))) {
	panic("Unknown operation %d in ScsiDisk blockIOProc.\n", 
		requestPtr->operation);
	return DEV_INVALID_ARG;
    }
    /*
     * Insure that the request is within the bounds of the partition.
     */
    firstSector = requestPtr->startAddress/DEV_BYTES_PER_SECTOR;
    lengthInSectors = requestPtr->bufferLen/DEV_BYTES_PER_SECTOR;
    sizeInSectors = (diskPtr->partition == WHOLE_DISK_PARTITION) ?
				diskPtr->sizeInSectors   :
		            diskPtr->map[diskPtr->partition].sizeInSectors;
    if (firstSector >= sizeInSectors) {
	/*
	 * The offset is past the end of the partition.
	 */
	printf("ScsiDisk request: firstSector(%d) >= size (%d)\n",
						 firstSector, sizeInSectors);
	RequestDone(requestPtr,SUCCESS,0);
	return SUCCESS;
    } 
    if (((firstSector + lengthInSectors - 1) >= sizeInSectors)) {
	/*
	 * The transfer is at the end of the partition.  Reduce the
	 * sector count so there is no overrun.
	 */
	lengthInSectors = sizeInSectors - firstSector;
    } 
    if (diskPtr->partition != WHOLE_DISK_PARTITION) {
	/*
	 * Relocate the disk address to be relative to this partition.
	 */
	firstSector += diskPtr->map[diskPtr->partition].firstSector;
    }

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
    if (devPtr == (ScsiDevice *) NIL) {
	return (DevBlockDeviceHandle *) NIL;
    }
    /*
     * Determine the type of device from the inquiry return by the
     * attach. Reject device if not of disk type. If the target 
     * didn't respond to the INQUIRY command we assume that it
     * just a stupid disk.
     */
    if ((devPtr->inquiryLength > 0) &&
	(((ScsiInquiryData *) (devPtr->inquiryDataPtr))->type != 
							SCSI_DISK_TYPE)) {
	(void) DevScsiReleaseDevice(devPtr);
	return (DevBlockDeviceHandle *) NIL;
    }
    devPtr->errorProc = DiskError;
    /*
     * Initialize the ScsiDisk structure. We don't need to read the label
     * if the user is opening the device in raw (non partitioned) mode.
     */
    diskPtr = InitDisk(devPtr,DISK_IS_PARTITIONED(devicePtr));
    if (diskPtr == (ScsiDisk *) NIL) {
	return (DevBlockDeviceHandle *) NIL;
    }
    /*
     * Register this disk with the Disk stat routines.
     */
    {
	Fs_Device rawDevice;

	rawDevice = *devicePtr;
	rawDevice.unit = rawDevice.unit & ~0xf;
	diskPtr->diskStatsPtr = DevRegisterDisk(&rawDevice,
					      devPtr->locationName,
					      ScsiDiskIdleCheck, 
					      (ClientData) diskPtr);
    }
    diskPtr->partition = DISK_IS_PARTITIONED(devicePtr) ? 
					DISK_PARTITION(devicePtr) :
					WHOLE_DISK_PARTITION;
    diskPtr->blockHandle.blockIOProc = BlockIOProc;
    diskPtr->blockHandle.releaseProc = ReleaseProc;
    diskPtr->blockHandle.IOControlProc = IOControlProc;
    diskPtr->blockHandle.minTransferUnit = SCSI_DISK_SECTOR_SIZE;
    diskPtr->blockHandle.maxTransferSize = devPtr->maxTransferSize;
    return (DevBlockDeviceHandle *) diskPtr;
}


