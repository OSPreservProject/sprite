/* 
 * devSCSIDisk.c --
 *
 *      The standard Open, Read, Write, IOControl, and Close operations
 *      are defined here for the SCSI disk.  This ``raw'' access to the
 *      disk is used during formatting and disk recovery.
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
#include "fs.h"
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devSCSIDisk.h"
#include "devDiskLabel.h"

#include "dbg.h"
static int SCSIdebug = FALSE;

/*
 * State for each SCSI disk.  The state for all SCSI disks are kept
 * together so that the driver can easily find the disk and partition
 * that correspond to a filesystem unit number.
 */

DevSCSIDevice *scsiDisk[SCSI_MAX_DISKS];
int scsiDiskIndex = -1;


/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDiskInit --
 *
 *	Initialize the device driver state for a SCSI Disk.
 *	In order for filesystem unit numbers to correctly match up
 *	with different disks we depend on Dev_SCSIInitDevice to
 *	increment the scsiDiskIndex properly.
 *
 * Results:
 *	SUCCESS if the disk is on-line.
 *
 * Side effects:
 *	The disk's label is read and saved in a DevSCSIDisk structure.  This
 *	is allocated here and referneced by the private data pointer in
 *	the generic DevSCSIDevice structure.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSIDiskInit(devPtr)
    DevSCSIDevice *devPtr;		/* Device state to complete */
{
    register DevSCSIDisk *diskPtr;
    ReturnStatus status;
    /*
     * Check that the disk is on-line.  This means we won't find a disk
     * if its powered down upon boot.
     */
    devPtr->type = SCSI_DISK;
    devPtr->errorProc = DevSCSIDiskError;
    devPtr->sectorSize = DEV_BYTES_PER_SECTOR;
    diskPtr = (DevSCSIDisk *) malloc(sizeof(DevSCSIDisk));
    devPtr->data = (ClientData)diskPtr;
    diskPtr->type = SCSI_GENERIC_DISK;
    status = DevSCSITest(devPtr);
    if (status != SUCCESS) {
	free((char *)diskPtr);
	return(status);
    }
    /*
     * Set up a slot in the disk list. The slot number has to correspond
     * to a file system device unit number, so we depend on Dev_SCSIInitDevice
     * to increment scsiDiskIndex each time it is called, and on the
     * layout of the DevConfig device table.  (This should be improved).
     */
    if (scsiDiskIndex >= SCSI_MAX_DISKS) {
	printf("SCSIDiskInit: Too many disks configured\n");
	free((char *)diskPtr);
	return(FAILURE);
    }
    status = DevSCSIDoLabel(devPtr);
    if (status != SUCCESS) {
	free((char *)diskPtr);
	return(status);
    }
    scsiDisk[scsiDiskIndex] = devPtr;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDoLabel --
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
ReturnStatus
DevSCSIDoLabel(devPtr)
    DevSCSIDevice *devPtr;
{
    register DevSCSIController *scsiPtr = devPtr->scsiPtr;
    DevSCSIDisk *diskPtr;
    register ReturnStatus status;
    Sun_DiskLabel *diskLabelPtr;
    int part;

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

	DevSCSISetupCommand(SCSI_READ, devPtr, 0, 1);
    
	status = (*scsiPtr->commandProc)(devPtr->targetID, scsiPtr,
			DEV_BYTES_PER_SECTOR, scsiPtr->labelBuffer, WAIT);

    scsiPtr->flags &= ~SCSI_CNTRLR_BUSY;
    Sync_MasterBroadcast(&scsiPtr->readyForIO);
    MASTER_UNLOCK(&scsiPtr->mutex);

    if (status != SUCCESS) {
	printf("SCSI-%d: couldn't read the disk%d-%d label\n",
			     scsiPtr->number, devPtr->targetID, devPtr->LUN);
	return(status);
    }
    diskLabelPtr = (Sun_DiskLabel *)scsiPtr->labelBuffer;
    printf("SCSI-%d disk%d-%d: %s\n", scsiPtr->number, devPtr->targetID,
			devPtr->LUN, diskLabelPtr->asciiLabel);

    diskPtr = (DevSCSIDisk *)devPtr->data;
    diskPtr->numCylinders = diskLabelPtr->numCylinders;
    diskPtr->numHeads = diskLabelPtr->numHeads;
    diskPtr->numSectors = diskLabelPtr->numSectors;

    printf(" Partitions ");
    for (part = 0; part < DEV_NUM_DISK_PARTS; part++) {
	diskPtr->map[part].firstCylinder =
		diskLabelPtr->map[part].cylinder;
	diskPtr->map[part].numCylinders =
		diskLabelPtr->map[part].numBlocks /
		(diskLabelPtr->numHeads * diskLabelPtr->numSectors) ;
	printf(" (%d,%d)", diskPtr->map[part].firstCylinder,
				   diskPtr->map[part].numCylinders);
    }
    printf("\n");
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDiskIO --
 *
 *      Read or Write (to/from) a raw SCSI disk file. The deviceUnit
 *      number is mapped to a particular partition on a particular disk.
 *      The starting coordinate, firstSector,  is relocated to be relative
 *      to the corresponding disk partition.  The transfer is checked
 *      against the partition size to make sure that the I/O doesn't cross
 *      a disk partition.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The number of sectors to transfer gets trimmed down if it would
 *	cross into the next partition.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSIDiskIO(command, deviceUnit, buffer, firstSector, numSectorsPtr)
    int command;			/* SCSI_READ or SCSI_WRITE */
    int deviceUnit;			/* Unit from the filesystem that
					 * indicates a disk and partition */
    char *buffer;			/* Target buffer */
    int firstSector;			/* First sector to transfer. Should be
					 * relative to the start of the disk
					 * partition corresponding to the unit*/
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors actually transferred. */
{
    ReturnStatus status;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    int part;		/* Partition of disk that corresponds to unit number */
    DevSCSIDevice *devPtr;		/* Generic SCSI device state */
    register DevSCSIDisk *diskPtr;	/* State of the disk */
    int totalSectors;		/* The total number of sectors to transfer */
    int numSectors;		/* The number of sectors to transfer at
				 * one time, up to a blocks worth. */
    int lastSector;	/* Last sector of the partition */
    int totalRead;	/* The total number of sectors actually transferred */

    disk = deviceUnit / DEV_NUM_DISK_PARTS;
    part = deviceUnit % DEV_NUM_DISK_PARTS;
    devPtr = scsiDisk[disk];
    diskPtr = (DevSCSIDisk *)devPtr->data;

    /*
     * Do bounds checking to keep the I/O within the partition.
     */
    lastSector = diskPtr->map[part].numCylinders *
		 (diskPtr->numHeads * diskPtr->numSectors) - 1;
    totalSectors = *numSectorsPtr;

    if (firstSector > lastSector) {
	/*
	 * The offset is past the end of the partition.
	 */
	*numSectorsPtr = 0;
	return(SUCCESS);
    } else if ((firstSector + totalSectors - 1) > lastSector) {
	/*
	 * The transfer is at the end of the partition.  Reduce the
	 * sector count so there is no overrun.
	 */
	totalSectors = lastSector - firstSector + 1;
    }
    /*
     * Relocate the disk address to be relative to this partition.
     */
    firstSector += diskPtr->map[part].firstCylinder *
		    (diskPtr->numHeads * diskPtr->numSectors);
    /*
     * Chop up the IO into blocksize pieces.
     */
    totalRead = 0;
    do {
	if (totalSectors > SECTORS_PER_BLOCK) {
	    numSectors = SECTORS_PER_BLOCK;
	} else {
	    numSectors = totalSectors;
	}
	status = DevSCSISectorIO(command, devPtr, firstSector, &numSectors, buffer);
	if (status == SUCCESS) {
	    firstSector += numSectors;
	    totalSectors -= numSectors;
	    buffer += numSectors * DEV_BYTES_PER_SECTOR;
	    totalRead += numSectors;
	}
    } while (status == SUCCESS && totalSectors > 0);
    *numSectorsPtr = totalRead;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSISectorIO --
 *
 *      Lower level routine to read or write an SCSI device.  The
 *      interface here is in terms of a particular SCSI disk and the
 *      number of sectors to transfer.  This routine takes care of mapping
 *      its buffer into the special multibus memory area that is set up
 *      for Sun DMA.  It retries in the event of errors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSISectorIO(command, devPtr, firstSector, numSectorsPtr, buffer)
    int command;			/* SCSI_READ or SCSI_WRITE */
    DevSCSIDevice *devPtr;		/* Which disk to do I/O with */
    int firstSector;			/* The sector at which the transfer
					 * begins. */
    int *numSectorsPtr;			/* Upon entry, the number of sectors to
					 * transfer.  Upon return, the number
					 * of sectors transferred. */
    char *buffer;			/* Target buffer */
{
    ReturnStatus status;
    register DevSCSIController *scsiPtr; /* Controller for the disk */
    int i;

    /*
     * Synchronize with the interrupt handling routine and with other
     * processes that are trying to initiate I/O with this controller.
     * FIX HERE TO ENQUEUE REQUESTS - GOES WITH CONNECT/DIS-CONNECT
     */
    scsiPtr = devPtr->scsiPtr;
    MASTER_LOCK(&scsiPtr->mutex);
    if (command == SCSI_READ) {
	scsiPtr->configPtr->diskReads++;
    } else {
	scsiPtr->configPtr->diskWrites++;
    }
    /*
     * Here we are using a condition variable and the scheduler to
     * synchronize access to the controller.  An alternative would be
     * to have a command queue associated with the controller.  We can't
     * rely on the mutex variable because that is relinquished later
     * when the process using the controller waits for the I/O to complete.
     */
    while (scsiPtr->flags & SCSI_CNTRLR_BUSY) {
	Sync_MasterWait(&scsiPtr->readyForIO, &scsiPtr->mutex, FALSE);
    }
    scsiPtr->flags |= SCSI_CNTRLR_BUSY;

    /*
     * Map the buffer into the special area of multibus memory that
     * the device can DMA into.
     */
    buffer = VmMach_DevBufferMap(*numSectorsPtr * devPtr->sectorSize,
			     buffer, scsiPtr->IOBuffer);
    /*
     * Retry the operation if we hit a hard error. This is because
     * many hard errors seem to work when retried.  Retry recovered
     * errors too, though that may not be necessary.  (Alternatively,
     * convert the error status to success on a recovered error.)
     */
    i = -1;
    do {
	i++;

	scsiPtr->flags &= ~SCSI_IO_COMPLETE;
	DevSCSISetupCommand(command, devPtr, firstSector, *numSectorsPtr);
	status = (*scsiPtr->commandProc)(devPtr->targetID, scsiPtr,
				 *numSectorsPtr * devPtr->sectorSize,
				 buffer, INTERRUPT);
	/*
	 * Wait for the command to complete.  The interrupt handler checks
	 * for I/O errors, computes the residual, and notifies us.
	 */
	if (status == SUCCESS) {
	    while((scsiPtr->flags & SCSI_IO_COMPLETE) == 0) {
		Sync_MasterWait(&scsiPtr->IOComplete, &scsiPtr->mutex,
				      FALSE);
	    }
	    status = scsiPtr->status;
	}
    } while ((status == DEV_HARD_ERROR || status == DEV_RETRY_ERROR ||
	      status == DEV_DMA_FAULT) && i < SCSI_NUM_HARD_ERROR_RETRIES);
    if (i >= SCSI_NUM_HARD_ERROR_RETRIES) {
	if (devSCSIDebug > 2) {
	    panic("SCSI: Too many retries after error.\n");
	} else {
	    printf("Warning: SCSI: Too many retries after error.\n");
	}
    }
    *numSectorsPtr -= (scsiPtr->residual / devPtr->sectorSize);
    scsiPtr->flags &= ~SCSI_CNTRLR_BUSY;
    Sync_MasterBroadcast(&scsiPtr->readyForIO);
    MASTER_UNLOCK(&scsiPtr->mutex);
    /*
     * Voluntarily give up the CPU in case anyone else wants to use the
     * disk.  The combination of the readyForIO and ioComplete 
     */
    Sched_ContextSwitch(PROC_READY);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskOpen --
 *
 *	Open a partition of an SCSI disk as a file.  This merely
 *	checks to see if the disk is on-line before succeeding.
 *
 * Results:
 *	SUCCESS if the disk is on-line.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Dev_SCSIDiskOpen(devicePtr, useFlags, token)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData token;		/* Call-back token for input, unused here */
{
    ReturnStatus error;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    DevSCSIDevice *diskPtr;	/* State of the disk */

    disk = devicePtr->unit / DEV_NUM_DISK_PARTS;
    if (disk >= SCSI_MAX_DISKS) {
	return(DEV_INVALID_UNIT);
    }
    diskPtr = scsiDisk[disk];
    if ((diskPtr == (DevSCSIDevice *)NIL) || (diskPtr == (DevSCSIDevice *)0)) {
	return(DEV_INVALID_UNIT);
    }

    error = DevSCSITest(diskPtr);
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskRead --
 *
 *	Read from a raw SCSI Disk.  This understands about sectors and
 *	may break the read up to account for user offsets into the disk
 *	file that don't correspond to the start of a sector.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process will sleep waiting for the I/O to complete.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_SCSIDiskRead(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle for raw SCSI disk device */
    int offset;			/* Indicates starting point for read.  This
				 * is rounded down to the start of a sector */
    int bufSize;		/* Number of bytes to read,  rounded down
				 * do a multiple of the sector size */
    char *buffer;		/* Buffer for the read */
    int *lenPtr;		/* How many bytes actually read */
{
    ReturnStatus error;	/* Error code */
    int firstSector;	/* The first sector to read */
    int numSectors;	/* The number of sectors to read */

    /*
     * For simplicity the offset is rouned down to the start of the sector
     * and the amount to read is also rounded down to a whole number of
     * sectors.  This should break misaligned reads up so the first and
     * last sectors are read into an extra buffer and copied into the
     * user's buffer.
     */
    if (SCSIdebug) {
	DBG_CALL;
    }
    offset &= ~(DEV_BYTES_PER_SECTOR-1);
    bufSize &= ~(DEV_BYTES_PER_SECTOR-1);
    firstSector = offset / DEV_BYTES_PER_SECTOR;
    numSectors = bufSize / DEV_BYTES_PER_SECTOR;
    error = DevSCSIDiskIO(SCSI_READ, devicePtr->unit, buffer,
			firstSector, &numSectors);
    *lenPtr = numSectors * DEV_BYTES_PER_SECTOR;
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskWrite --
 *
 *	Write to a raw SCSI disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_SCSIDiskWrite(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle of raw disk device */
    int offset;			/* Indicates the starting point of the write.
				 * Rounded down to the start of a sector */
    int bufSize;		/* Number of bytes to write.  Rounded down
				 * to a multiple of the sector size */
    char *buffer;		/* Write buffer */
    int *lenPtr;		/* How much was actually written */
{
    ReturnStatus error;
    int firstSector;
    int numSectors;

    /*
     * For simplicity the offset is rouned down to the start of the sector
     * and the amount to write is also rounded down to a whole number of
     * sectors.  For misaligned writes we need to first read in the sector
     * and then overwrite part of it.
     */
    if (SCSIdebug) {
	DBG_CALL;
    }
    offset &= ~(DEV_BYTES_PER_SECTOR-1);
    bufSize &= ~(DEV_BYTES_PER_SECTOR-1);
    firstSector = offset / DEV_BYTES_PER_SECTOR;
    numSectors = bufSize / DEV_BYTES_PER_SECTOR;
    error = DevSCSIDiskIO(SCSI_WRITE, devicePtr->unit, buffer,
			firstSector, &numSectors);
    *lenPtr = numSectors * DEV_BYTES_PER_SECTOR;

    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskIOControl --
 *
 *	Do a special operation on a raw SCSI Disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_SCSIDiskIOControl(devicePtr, command, inBufSize, inBuffer,
				 outBufSize, outBuffer)
    Fs_Device *devicePtr;
    int command;
    int inBufSize;
    char *inBuffer;
    int outBufSize;
    char *outBuffer;
{
#ifdef not_used_yet
    ReturnStatus error;
    int disk;		/* Disk number of disk that has the partition that
			 * corresponds to the unit number */
    int part;		/* Partition of disk that corresponds to unit number */
    DevSCSIDisk *diskPtr;	/* State of the disk */
#endif
    switch (command) {
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
	    
	default:
	    return(GEN_INVALID_ARG);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskClose --
 *
 *	Close a raw SCSI disk file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SCSIDiskClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;
    int 	useFlags;
    int		openCount;
    int		writerCount;
{
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDiskError --
 *
 *	Handle errors indicated by the sense data returned from the disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSIDiskError(devPtr, sensePtr)
    DevSCSIDevice *devPtr;
    DevSCSISense *sensePtr;
{
    register ReturnStatus status;
    DevSCSIDisk *diskPtr = (DevSCSIDisk *)devPtr->data;
    int command = devPtr->scsiPtr->command;

    if (sensePtr->error == 0x70) {
	/*
	 * Class 7 Extended sense data.  Set disk type so we
	 * switch out to extended sense handling below.
	 */
	diskPtr->type = SCSI_CLASS7_DISK;
    }
    switch (diskPtr->type) {
	case SCSI_GENERIC_DISK: {
	    /*
	     * Old style sense data.
	     */
	    if (sensePtr->error == SCSI_NO_SENSE_DATA) {	    
		status = SUCCESS;
	    } else {
		register int class = (sensePtr->error & 0x70) >> 4;
		register int code = sensePtr->error & 0xF;
		register int addr;
		addr = (sensePtr->highAddr << 16) |
			(sensePtr->midAddr << 8) |
			sensePtr->lowAddr;
		printf("SCSI-%d: Sense error (%d-%d) at <%x> ",
				 devPtr->scsiPtr->number, class, code, addr);
		if (scsiNumErrors[class] > code) {
		    printf("%s", scsiErrors[class][code]);
		}
		printf("\n");
		status = DEV_HARD_ERROR;
	    }
	    break;
	}
	/*
	 * Standard error handling for class7 extended sense data.
	 */
	case SCSI_CLASS7_DISK: {
	    register DevSCSIExtendedSense *extSensePtr;
	    extern Boolean devSCSIDebug;
	    DevSCSIController *scsiPtr;

	    extSensePtr = (DevSCSIExtendedSense *)sensePtr;
	    scsiPtr = devPtr->scsiPtr;
	    
	    switch (extSensePtr->key) {
		case SCSI_NO_SENSE:
		    status = SUCCESS;
		    break;
		case SCSI_RECOVERABLE:
		    /*
		     * The drive recovered from an error.
		     */
		    if (devSCSIDebug > 2) {
			printf("Warning: SCSI-%d drive %d LUN %d, recoverable error\n",
				  scsiPtr->number, devPtr->targetID,
				  devPtr->LUN);
			printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
				   extSensePtr->info1 & 0xff,
				   extSensePtr->info2 & 0xff,
				   extSensePtr->info3 & 0xff,
				   extSensePtr->info4 & 0xff);
		    }
		    scsiPtr->numRecoverableErrors++;
		    status = DEV_RETRY_ERROR;
		    break;
		case SCSI_NOT_READY_KEY:
		    status = DEV_OFFLINE;
		    break;
		case SCSI_ILLEGAL_REQUEST:
		    /*
		     * Probably a programming error.
		     */
		    printf("SCSI-%d drive %d LUN %d, illegal request %d\n",
				scsiPtr->number, devPtr->targetID,
				devPtr->LUN, command);
		    status = DEV_INVALID_ARG;
		    break;
		case SCSI_MEDIA_ERROR:
		case SCSI_HARDWARE_ERROR:
		    if (devSCSIDebug > 2) {
			panic("SCSI-%d drive %d LUN %d, hard class7 key %x\n",
			      scsiPtr->number, devPtr->targetID,
			      devPtr->LUN, extSensePtr->key);
		    } else {
			printf("SCSI-%d drive %d LUN %d, hard class7 key %x\n",
			      scsiPtr->number, devPtr->targetID, devPtr->LUN,
			      extSensePtr->key);
		    }
		    printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
			extSensePtr->info1 & 0xff,
			extSensePtr->info2 & 0xff,
			extSensePtr->info3 & 0xff,
			extSensePtr->info4 & 0xff);
		    scsiPtr->numHardErrors++;
		    status = DEV_HARD_ERROR;
		    break;
		case SCSI_UNIT_ATTN_KEY:
		    /*
		     * This is an error that occurs after the drive is reset.
		     * It can probably be ignored.
		     */
		    scsiPtr->numUnitAttns++;
		    status = SUCCESS;
		    break;
		case SCSI_WRITE_PROTECT_KEY:
		    printf("SCSI-%d drive %d LUN %d is write protected\n",
			scsiPtr->number, devPtr->targetID, devPtr->LUN);
		    status = FS_NO_ACCESS;
		    break;
		case SCSI_BLANK_CHECK:
		    /*
		     * Shouldn't encounter blank media on a disk.
		     */
		    printf("SCSI-%d drive %d LUN %d, \"blank check\"\n",
			scsiPtr->number, devPtr->targetID, devPtr->LUN);
		    printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
			extSensePtr->info1 & 0xff,
			extSensePtr->info2 & 0xff,
			extSensePtr->info3 & 0xff,
			extSensePtr->info4 & 0xff);
		    status = DEV_HARD_ERROR;
		    break;
		case SCSI_VENDOR:
		case SCSI_POWER_UP_FAILURE:
		case SCSI_ABORT_KEY:
		case SCSI_EQUAL:
		case SCSI_OVERFLOW:
		    printf("Warning: SCSI-%d drive %d-%d, unsupported class7 error %d\n",
			scsiPtr->number, devPtr->targetID, devPtr->LUN,
			extSensePtr->key);
		    status = DEV_HARD_ERROR;
		    break;
		default:
		    printf("Warning: SCSI-%d drive %d-%d, can't handle error %d\n",
			scsiPtr->number, devPtr->targetID, devPtr->LUN,
			extSensePtr->key);
		    status = DEV_HARD_ERROR;
		    break;
	    }
	    break;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskBlockIOInit --
 *
 *	Initialization routine for the Block I/O interface to the SCSI disk.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Saves a pointer to the geometry information for the ClientData
 *	field of the Fs_Device object passed in.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_SCSIDiskBlockIOInit(devicePtr)
    Fs_Device *devicePtr;	/* Use the unit number to specify partition */
{
    return(SUCCESS);		/* NOT CALLED */
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSIDiskBlockIO --
 *
 *	Do block I/O on the SCSI disk.  This uses the disk geometry
 *	information to map from filesystem fragments indexes to filesystem
 *	block indexes, and finally to disk sectors.
 *
 * Results:
 *	The return code from the I/O operation.
 *
 * Side effects:
 *	The disk write, if readWrite == FS_WRITE.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Dev_SCSIDiskBlockIO(readWrite, devicePtr, fragNumber, numFrags, buffer)
    int readWrite;		/* FS_READ or FS_WRITE */
    Fs_Device *devicePtr;	/* Use the unit number to specify partition */
    int fragNumber;		/* Index of first fragment to transfer*/
    int numFrags;		/* Number of fragments to transfer */
    Address buffer;		/* I/O buffer */
{
    ReturnStatus status;	/* General return code */
    int firstSector;		/* Starting sector of transfer */
    int numSectors;		/* Number of sectors to transfer */
    int scsiCommand;		/* SCSI_READ or SCSI_WRITE */

    scsiCommand = (readWrite == FS_READ) ? SCSI_READ : SCSI_WRITE;
    if ((fragNumber % FS_FRAGMENTS_PER_BLOCK) != 0) {
	/*
	 * The I/O doesn't start on a block boundary.  Transfer the
	 * first few extra fragments to get things going on a block boundary.
	 */
	register int extraFrags;

	extraFrags = FS_FRAGMENTS_PER_BLOCK -
		    (fragNumber % FS_FRAGMENTS_PER_BLOCK);
	if (extraFrags > numFrags) {
	    extraFrags = numFrags;
	}
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	numSectors = extraFrags * SECTORS_PER_FRAGMENT;
	status = DevSCSIDiskIO(scsiCommand, devicePtr->unit, buffer,
					    firstSector, &numSectors);
	extraFrags = numSectors / SECTORS_PER_FRAGMENT;
	fragNumber += extraFrags;
	buffer += extraFrags * FS_FRAGMENT_SIZE;
	numFrags -= extraFrags;
	if (status != SUCCESS) {
	    return(status);
	}
    }
    while (numFrags >= FS_FRAGMENTS_PER_BLOCK) {
	/*
	 * Transfer whole blocks.
	 */
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	numSectors = SECTORS_PER_FRAGMENT * FS_FRAGMENTS_PER_BLOCK;
	status = DevSCSIDiskIO(scsiCommand, devicePtr->unit, buffer,
					    firstSector, &numSectors);
	fragNumber += FS_FRAGMENTS_PER_BLOCK;
	buffer += FS_BLOCK_SIZE;
	numFrags -= FS_FRAGMENTS_PER_BLOCK;
	if (status != SUCCESS) {
	    return(status);
	}
    }
    if (numFrags > 0) {
	/*
	 * Transfer the left over fragments.
	 */
	firstSector = Fs_BlocksToSectors(fragNumber, devicePtr->data);
	numSectors = numFrags * SECTORS_PER_FRAGMENT;
	status = DevSCSIDiskIO(scsiCommand, devicePtr->unit, buffer,
					    firstSector, &numSectors);
    }
    return(status);
}

