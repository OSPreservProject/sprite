/* 
 * devSCSITape.c --
 *
 *      The standard Open, Read, Write, IOControl, and Close operations
 *      are defined here for the SCSI tape.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devSCSITape.h"
#include "devSCSISysgen.h"
#include "devSCSIEmulex.h"
#include "devSCSIExabyte.h"
#include "byte.h"

#include "dbg.h"
int SCSITapeDebug = FALSE;

/*
 * State for each SCSI tape drive.  This used to map from unit numbers
 * back to the controller for the drive.
 */
int scsiTapeIndex = -1;
DevSCSIDevice *scsiTape[SCSI_MAX_TAPES];


/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeInit --
 *
 *	Initialize the device driver state for a SCSI Tape drive.
 *	In order for filesystem unit numbers to correctly match up
 *	with different disks we depend on Dev_SCSIInitDevice to
 *	increment the scsiTapeIndex properly.
 *
 * Results:
 *	SUCCESS.  Because tape drives take up to several seconds to
 *	initialize themselves we always assume one is out there.
 *
 * Side effects:
 *	A DevSCSITape structure is allocated and referneced by the private
 *	data pointer in	the generic DevSCSIDevice structure.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITapeInit(devPtr)
    DevSCSIDevice *devPtr;		/* Device state to complete */
{
    register DevSCSITape *tapePtr;

    /*
     * Don't try to talk to the tape drive at boot time.  It may be doing
     * stuff after the SCSI bus reset like auto load.
     */
    if (scsiTapeIndex >= SCSI_MAX_TAPES) {
	printf("SCSI: Too many tape drives configured\n");
	return(FAILURE);
    }
    devPtr->type = SCSI_TAPE;
    devPtr->errorProc = DevSCSITapeError;
    devPtr->sectorSize = DEV_BYTES_PER_SECTOR;
    tapePtr = (DevSCSITape *) malloc(sizeof(DevSCSITape));
    devPtr->data = (ClientData)tapePtr;
    tapePtr->state = SCSI_TAPE_CLOSED;
    tapePtr->type = SCSI_UNKNOWN;
    scsiTape[scsiTapeIndex] = devPtr;
    printf("SCSI-%d tape %d at slave %d\n",
		devPtr->scsiPtr->number, scsiTapeIndex, devPtr->targetID);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeType --
 *
 *	This determines the type of the tape drive depending on
 *	the amount of sense data returned from the drive and initializes
 *	the tapePtr structure.. 
 *
 * Results:
 *	SUCCESS.  Because tape drives take up to several seconds to
 *	initialize themselves we always assume one is out there.
 *
 * Side effects:
 *	A DevSCSITape structure is allocated and referneced by the private
 *	data pointer in	the generic DevSCSIDevice structure.
 *
 *----------------------------------------------------------------------
 */
int
DevSCSITapeType(senseSize, tapePtr)
    int senseSize;			/* Amount of sense data in bytes */
    DevSCSITape *tapePtr;
{

    printf("SCSITape, %d sense bytes => ", senseSize);
    switch(senseSize) {
	case sizeof(DevQICIISense):
	    DevSysgenInit(tapePtr);
	    printf("Sysgen\n");
	    break;
	case sizeof(DevEmulexTapeSense):
	case sizeof(DevEmulexTapeSense)-1: /* The sense size of the Emulex 
					    * drive is 11 bytes but the
					    * C compiler rounds all structures
					    * to even byte size.
					    */
	    DevEmulexInit(tapePtr);
	    printf("Emulex\n");
	    break;
	case sizeof(DevExabyteSense):
	    DevExabyteInit(tapePtr);
	    printf("Exabyte\n");
	    break;
	default:
	    printf("Unknown sense size\n");
    }
    return(tapePtr->type);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeIO --
 *
 *      Low level routine to read or write an SCSI tape device.  The
 *      interface here is in terms of a particular SCSI tape and the
 *      number of sectors to transfer.  This routine takes care of mapping
 *      its buffer into the special multibus memory area that is set up
 *      for Sun DMA.  Each IO involves one tape block, and all tape blocks
 *	are multiples of the underlying device block size (DEV_BYTES_PER_SECTOR)
 *
 *	This should be combined with DevSCSISectorIO as it is so similar.
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
DevSCSITapeIO(command, devPtr, buffer, countPtr)
    int command;			/* SCSI_READ, SCSI_WRITE, etc. */
    register DevSCSIDevice *devPtr; 	/* State info for the tape */
    char *buffer;			/* Target buffer */
    int *countPtr;			/* Upon entry, the number of sectors to
					 * transfer, or general count for
					 * skipping blocks, etc. Upon return,
					 * the number of sectors transferred. */
{
    ReturnStatus status;
    register DevSCSIController *scsiPtr; /* Controller for the drive */

    /*
     * Synchronize with the interrupt handling routine and with other
     * processes that are trying to initiate I/O with this controller.
     */
    scsiPtr = devPtr->scsiPtr;
    MASTER_LOCK(&scsiPtr->mutex);

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
    scsiPtr->flags &= ~SCSI_IO_COMPLETE;

    if (command == SCSI_READ || command == SCSI_WRITE) {
	/*
	 * Map the buffer into the special area of multibus memory that
	 * the device can DMA into.  Probably have to worry about
	 * the buffer size.
	 */
	buffer = VmMach_DevBufferMap(*countPtr * DEV_BYTES_PER_SECTOR,
				 buffer, scsiPtr->IOBuffer);
    }
    DevSCSITapeSetupCommand(command, devPtr, countPtr);
    status = (*scsiPtr->commandProc)(devPtr->targetID, scsiPtr, *countPtr,
				     buffer, INTERRUPT);
    /*
     * Wait for the command to complete.  The interrupt handler checks
     * for I/O errors, computes the residual, and notifies us.
     */
    if (status == SUCCESS) {
	while((scsiPtr->flags & SCSI_IO_COMPLETE) == 0) {
	    Sync_MasterWait(&scsiPtr->IOComplete, &scsiPtr->mutex, FALSE);
	}
	status = scsiPtr->status;
    }
    if (scsiPtr->residual) {
	printf("Warning: SCSI residual %d, cmd %x\n", scsiPtr->residual,
			    command);
    }
    *countPtr -= scsiPtr->residual;
    if (command == SCSI_READ || command == SCSI_WRITE) {
	*countPtr /= DEV_BYTES_PER_SECTOR;
    }
    scsiPtr->flags &= ~SCSI_CNTRLR_BUSY;
    Sync_MasterBroadcast(&scsiPtr->readyForIO);
    MASTER_UNLOCK(&scsiPtr->mutex);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeSetupCommand --
 *
 *	A variation on DevSCSISetupCommand that creates a control block
 *	designed for tape drives.  SCSI tape drives read from the current
 *	tape position, so there is only a block count, no offset.  There
 *	is a special code that modifies the command in the tape control
 *	block.  The value of the code is a function of the command and the
 *	type of the tape drive (ugh.)  The correct code is determined here.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the tape control block.
 *
 *----------------------------------------------------------------------
 */
void
DevSCSITapeSetupCommand(command, devPtr, countPtr)
    int command;		/* One of SCSI_* commands */
    DevSCSIDevice *devPtr;	/* Device state */
    int *countPtr;		/* In - Transfer count, blocks or bytes!
				 * Out - The proper dma byte count for caller */
{
    register DevSCSITapeControlBlock	*tapeControlBlockPtr;
    register DevSCSITape		*tapePtr;
    int dmaCount = *countPtr;		/* DMA count needed by host interface */
    int count = *countPtr;		/* Count put in the control block */

    devPtr->scsiPtr->devPtr = devPtr;
    tapeControlBlockPtr =
	    (DevSCSITapeControlBlock *)&devPtr->scsiPtr->controlBlock;
    bzero((Address)tapeControlBlockPtr,sizeof(DevSCSITapeControlBlock));
    tapePtr = (DevSCSITape *)devPtr->data;
    if ((int)tapePtr->setupProc != NIL) {
	/*
	 * If the drive type is known we have to customize the control
	 * block with various vendor-specific bits.  This means that
	 * the first commands done can't depend on this.  This is ok
	 * as we do a REQUEST_SENSE first to detect the drive type.
	 */
	(*tapePtr->setupProc)(tapePtr, &command, tapeControlBlockPtr,
		&dmaCount, &count);
    }
    tapeControlBlockPtr->command = command & 0xff;
    tapeControlBlockPtr->unitNumber = devPtr->LUN;
    tapeControlBlockPtr->highCount = (count & 0x1f0000) >> 16;
    tapeControlBlockPtr->midCount =  (count & 0x00ff00) >> 8;
    tapeControlBlockPtr->lowCount =  (count & 0x0000ff);
    *countPtr = dmaCount;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSITapeOpen --
 *
 *	Open a SCSI tape drive as a file.  This merely
 *	checks to see if the drive is on-line before succeeding.
 *
 * Results:
 *	SUCCESS if the tape is on-line.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Dev_SCSITapeOpen(devicePtr, useFlags, token)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData token;		/* Call-back token for input, unused here */
{
    ReturnStatus status;
    DevSCSIDevice *devPtr;
    DevSCSITape *tapePtr;
    int retries = 0;

    /*
     * Unix has a complex encoding of tape densities and no-rewind
     * characteristics in the unit number.
     *	0	first drive, rewind
     *	1	second drive, rewind
     *	2	third drive, rewind
     *	3	forth drive, rewind
     *	4	first drive, no rewind
     *	5	second drive, no rewind
     *	6	third drive, no rewind
     *	7	forth drive, no rewind
     *	unit numbers 8-15 are for the QIC 24 format, not QIC 11.
     *
     * For now we just ignore this and support one kind of tape interface:
     * a raw one where reads return the next tape record, and writes
     * create a record.
     */
    if (devicePtr->unit > SCSI_MAX_TAPES * DEV_TAPES_PER_CNTRLR) {
	return(DEV_INVALID_UNIT);
    }
    devPtr = scsiTape[devicePtr->unit / DEV_TAPES_PER_CNTRLR];
    if (devPtr == (DevSCSIDevice *)0 || devPtr == (DevSCSIDevice *)NIL) {
	return(DEV_NO_DEVICE);
    }
    tapePtr = (DevSCSITape *)devPtr->data;
    if (tapePtr->state & SCSI_TAPE_OPEN) {
	return(FS_FILE_BUSY);
    }
    devPtr->scsiPtr->command = SCSI_OPENING;
    do {
	status = DevSCSITest(devPtr);
	if (status == DEV_OFFLINE) {
	    break;
	}
	status = DevSCSIRequestSense(devPtr->scsiPtr, devPtr);
    } while (status != SUCCESS && ++retries < 3);
    if (status == SUCCESS) {
	/*
	 * Check for EMULEX controller, because it comes up in the
	 * wrong mode (QIC_24) and needs to be reset to use QIC 2 format.
	 */
	if (tapePtr->type == SCSI_EMULEX) {
	    status = DevSCSITapeModeSelect(devPtr, SCSI_MODE_QIC_02);
	}
	if (status == SUCCESS) {
	    tapePtr->state = SCSI_TAPE_OPEN;
	}
   }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSITapeRead --
 *
 *	Read from a raw SCSI Tape.  The offset is ignored because
 *	you can't seek the SCSI tape drive, only rewind it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process will sleep waiting for the I/O to complete.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_SCSITapeRead(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle for raw SCSI tape device */
    int offset;			/* IGNORED for tape. */
    int bufSize;		/* Number of bytes to read,  rounded down
				 * do a multiple of the sector size */
    char *buffer;		/* Buffer for the read */
    int *lenPtr;		/* How many bytes actually read */
{
    ReturnStatus error;	/* Error code */
    int numSectors;	/* The number of sectors to read */
    DevSCSIDevice *devPtr;
    DevSCSITape *tapePtr;

    devPtr = scsiTape[devicePtr->unit / DEV_TAPES_PER_CNTRLR];
    if (devPtr == (DevSCSIDevice *)0 || devPtr == (DevSCSIDevice *)NIL) {
	return(DEV_NO_DEVICE);
    }
    tapePtr = (DevSCSITape *)devPtr->data;
    if (tapePtr->state & SCSI_TAPE_AT_EOF) {
	/*
	 * Force the use of the SKIP_FILES control to get past the end of
	 * the file on tape.
	 */
	*lenPtr = 0;
	return(SUCCESS);
    }
    /*
     * For simplicity the offset is rouned down to the start of the sector
     * and the amount to read is also rounded down to a whole number of
     * sectors.  This should break misaligned reads up so the first and
     * last sectors are read into an extra buffer and copied into the
     * user's buffer.
     */
    bufSize &= ~(DEV_BYTES_PER_SECTOR-1);
    numSectors = bufSize / DEV_BYTES_PER_SECTOR;
    error = DevSCSITapeIO(SCSI_READ, devPtr, buffer, &numSectors);
    /*
     * Special check against funky end-of-file situations.  The Emulex tape
     * doesn't compute a correct residual when it hits the file mark
     * on the tape.
     */
    if (error == DEV_END_OF_TAPE) {
	*lenPtr = 0;
	error = SUCCESS;
    } else {
	*lenPtr = numSectors * DEV_BYTES_PER_SECTOR;
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSITapeWrite --
 *
 *	Write to a raw SCSI tape.
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
Dev_SCSITapeWrite(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle of raw tape device */
    int offset;			/* IGNORED for tape. */
    int bufSize;		/* Number of bytes to write.  Rounded down
				 * to a multiple of the sector size */
    char *buffer;		/* Write buffer */
    int *lenPtr;		/* How much was actually written */
{
    ReturnStatus error;
    int numSectors;
    DevSCSIDevice *devPtr;
    DevSCSITape *tapePtr;

    /*
     * For simplicity the offset is rouned down to the start of the sector
     * and the amount to write is also rounded down to a whole number of
     * sectors.  For misaligned writes we need to first read in the sector
     * and then overwrite part of it.
     */

    devPtr = scsiTape[devicePtr->unit / DEV_TAPES_PER_CNTRLR];
    if (devPtr == (DevSCSIDevice *)0 || devPtr == (DevSCSIDevice *)NIL) {
	return(DEV_NO_DEVICE);
    }
    tapePtr = (DevSCSITape *)devPtr->data;
    bufSize &= ~(DEV_BYTES_PER_SECTOR-1);
    numSectors = bufSize / DEV_BYTES_PER_SECTOR;
    error = DevSCSITapeIO(SCSI_WRITE, devPtr, buffer, &numSectors);
    *lenPtr = numSectors * DEV_BYTES_PER_SECTOR;
    tapePtr->state |= SCSI_TAPE_WRITTEN;

    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSITapeIOControl --
 *
 *	Do a special operation on a raw SCSI Tape.
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
Dev_SCSITapeIOControl(devicePtr, command, inBufSize, inBuffer,
				 outBufSize, outBuffer)
    Fs_Device *devicePtr;
    int command;
    int inBufSize;
    char *inBuffer;
    int outBufSize;
    char *outBuffer;
{
    ReturnStatus status = SUCCESS;
    int count = 0;
    DevSCSIDevice *devPtr;
    DevSCSITape *tapePtr;

   devPtr = scsiTape[devicePtr->unit / DEV_TAPES_PER_CNTRLR];
    if (devPtr == (DevSCSIDevice *)0 || devPtr == (DevSCSIDevice *)NIL) {
	return(DEV_NO_DEVICE);
    }
    tapePtr = (DevSCSITape *)devPtr->data;

    switch(command) {
	case IOC_REPOSITION: {
	    Ioc_RepositionArgs *repoArgsPtr;
	    repoArgsPtr = (Ioc_RepositionArgs *)inBuffer;

	    switch (repoArgsPtr->base) {
		case IOC_BASE_ZERO:
		    if (repoArgsPtr->offset != 0) {
			return(DEV_INVALID_ARG);
		    }
		    goto rewind;
		case IOC_BASE_CURRENT:
		    status = DEV_INVALID_ARG;
		    break;
		case IOC_BASE_EOF:
		    if (repoArgsPtr->offset != 0) {
			status = DEV_INVALID_ARG;
		    } else if ((tapePtr->state & SCSI_TAPE_WRITTEN) == 0) {
			/*
			 * If not atlready at the end of the tape by writing,
			 * space to the end of the current file.
			 */
			int count = 1;
			status = DevSCSITapeIO(SCSI_SPACE_FILES,
				    devPtr, (char *)0, &count);
		    }
		    break;
	    }
	    break;
	}
	case IOC_TAPE_COMMAND: {
	    Dev_TapeCommand *cmdPtr = (Dev_TapeCommand *)inBuffer;
	    if (inBufSize < sizeof(Dev_TapeCommand)) {
		return(DEV_INVALID_ARG);
	    }
	    switch (cmdPtr->command) {
		case IOC_TAPE_WEOF: {
		    count = 1;
		    while (cmdPtr->count) {
			cmdPtr->count--;
			status = DevSCSITapeIO(SCSI_WRITE_EOF, devPtr,
					(char *)0, &count);
		    }
		    break;
		}
		case IOC_TAPE_RETENSION:
		    tapePtr->state |= SCSI_TAPE_RETENSION;
		    /* fall through */
		case IOC_TAPE_OFFLINE:
		case IOC_TAPE_REWIND: {
rewind:		    /*
		     * Have to write an end-of-file mark if the last thing done
		     * was a write...
		     */
		    if (tapePtr->state & SCSI_TAPE_WRITTEN) {
			count = 1;
			status = DevSCSITapeIO(SCSI_WRITE_EOF, devPtr,
					(char *)0, &count);
			tapePtr->state &= ~SCSI_TAPE_WRITTEN;
		    }
		    status = DevSCSITapeIO(SCSI_REWIND, devPtr,
						(char *)0, &count);
		    break;
		}
		case IOC_TAPE_SKIP_BLOCKS: {
		    count = 1;
		    while (cmdPtr->count) {
			cmdPtr->count--;
			status = DevSCSITapeIO(SCSI_SPACE_BLOCKS, devPtr,
					(char *)0, &count);
			if (status == DEV_END_OF_TAPE) {
			    status = SUCCESS;
			}
		    }
		    break;
		case IOC_TAPE_SKIP_FILES:
		    count = 1;
		    while (cmdPtr->count) {
			cmdPtr->count--;
			status = DevSCSITapeIO(SCSI_SPACE_FILES, devPtr,
					(char *)0, &count);
			if (status == DEV_END_OF_TAPE) {
			    status = SUCCESS;
			}
		    }
		    break;
		}
		case IOC_TAPE_BACKUP_BLOCKS:
		case IOC_TAPE_BACKUP_FILES:
		    status = DEV_INVALID_ARG;
		    break;
		case IOC_TAPE_ERASE: {
		    status = DevSCSITapeIO(SCSI_ERASE_TAPE, devPtr,
				    (char *)0, &count);
		    break;
		}
		case IOC_TAPE_NO_OP: {
		    status = DevSCSITapeIO(SCSI_TEST_UNIT_READY, devPtr,
				    (char *)0, &count);
		    break;
		}
	    }
	    break;
	}
	case IOC_TAPE_STATUS: {
	    DevSCSIController *scsiPtr = devPtr->scsiPtr;
	    Dev_TapeStatus *statusPtr = (Dev_TapeStatus *)outBuffer;
	    if (outBufSize < sizeof(Dev_TapeStatus)) {
		return(DEV_INVALID_ARG);
	    }
	    status = DevSCSIRequestSense(devPtr->scsiPtr, devPtr);

	    if (scsiPtr->type == SCSI0) {
		DevSCSI0Regs *regsPtr = (DevSCSI0Regs *)scsiPtr->regsPtr;
		statusPtr->statusReg = regsPtr->control;
	    } else if (scsiPtr->type == SCSI3) {
		DevSCSI3Regs *regsPtr = (DevSCSI3Regs *)scsiPtr->regsPtr;
		statusPtr->statusReg = regsPtr->control;
	    }
	    (*tapePtr->statusProc)(devPtr, statusPtr);
	    statusPtr->residual = scsiPtr->residual;
	    statusPtr->fileNumber = 0;
	    statusPtr->blockNumber = 0;
	    break;
	}
	    /*
	     * No tape specific bits are set this way.
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
	    /*
	     * Could make this to an erase tape...
	     */
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
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSITapeClose --
 *
 *	Close a raw SCSI tape file.  This rewinds the tape.
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
Dev_SCSITapeClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;
    int		useFlags;	/* FS_READ | FS_WRITE */
    int		openCount;	/* Number of times device open. */
    int		writerCount;	/* Number of times device open for writing. */
{
    register ReturnStatus	status;
    DevSCSIDevice		*devPtr;
    DevSCSITape			*tapePtr;
    int 			count = 0;

    devPtr = scsiTape[devicePtr->unit / DEV_TAPES_PER_CNTRLR];
    if (devPtr == (DevSCSIDevice *)0 || devPtr == (DevSCSIDevice *)NIL) {
	return(DEV_NO_DEVICE);
    }
    if (openCount > 0) {
	return(SUCCESS);
    }
    tapePtr = (DevSCSITape *)devPtr->data;
    if (tapePtr->state & SCSI_TAPE_WRITTEN) {
	/*
	 * Make sure an end-of-file mark is at the end of the file on tape.
	 */
	count = 1;
	status = DevSCSITapeIO(SCSI_WRITE_EOF, devPtr,
			(char *)0, &count);
    }
    /*
     * For now, always rewind the tape upon close.
     */
    status = DevSCSITapeIO(SCSI_REWIND, devPtr, (char *)0,
			    &count);
    tapePtr->state = SCSI_TAPE_CLOSED;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeModeSelect --
 *
 *	Do the special mode select command to the tape drive.
 *	The mode select command sets the block size and the density of
 *	the tape.  This differs between the QIC 24 standard and the
 *	QIC 02 format.  We always want the QIC 02 standard, but some
 *	controllers start out in the other mode.
 *
 * Results:
 *	An error code from the command
 *
 * Side effects:
 *	This overwrites the label buffer used to read the disk label.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITapeModeSelect(devPtr, modeCommand)
    DevSCSIDevice *devPtr;
    int modeCommand;	/* SCSI_MODE_QIC_24 or SCSI_MODE_QIC_02 */
{
    ReturnStatus status;
    DevSCSIController *scsiPtr = devPtr->scsiPtr;
    DevSCSITape *tapePtr = (DevSCSITape *)devPtr->data;
    register DevEmulexModeSelParams *modeParamsPtr;
    int count;

    if (modeCommand == SCSI_MODE_QIC_24) {
	printf("Warning: SCSI Mode select won't do QIC 24 format");
	return(DEV_INVALID_ARG);
    } else if (tapePtr->type != SCSI_EMULEX) {
	printf("Warning: SCSI Mode select won't do Sysgen drives");
	return(DEV_INVALID_ARG);
    }
    modeParamsPtr = (DevEmulexModeSelParams *)scsiPtr->labelBuffer;
    bzero((Address)modeParamsPtr, sizeof(DevEmulexModeSelParams));
    modeParamsPtr->header.bufMode = 1;
    modeParamsPtr->header.blockLength = sizeof(DevEmulexModeSelBlock);
    modeParamsPtr->block.density = SCSI_EMULEX_QIC_02;
    /*
     * The rest of the fields in the select params can be left zero.
     */
    count = sizeof(DevEmulexModeSelParams);
    status = DevSCSITapeIO(SCSI_MODE_SELECT, devPtr, (Address)modeParamsPtr,
			    &count);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeError --
 *
 *	Handle error conditions from the tape drive.  This looks at
 *	the sense data returned after a command and determines what
 *	happened based on the drive type.  This is usually called
 *	at interrupt time.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITapeError(devPtr, sensePtr)
    DevSCSIDevice *devPtr;
    DevSCSISense *sensePtr;
{
    register ReturnStatus status = SUCCESS;
    DevSCSITape *tapePtr = (DevSCSITape *)devPtr->data;

    if (tapePtr->type == SCSI_UNKNOWN) {
	printf("DevSCSITapeError: Unknown tape drive type");
	if (sensePtr->error != SCSI_NO_SENSE_DATA) {
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
	    status = DEV_INVALID_ARG;
	}
    } else {
	status = (*tapePtr->errorProc)(devPtr, sensePtr);
    }
    return(status);
}
