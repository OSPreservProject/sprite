/* 
 * devSCSITape.c --
 *
 *      The standard Open, Read, Write, IOControl, and Close operations
 *      are defined here for the SCSI tape.
 *
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
#include "scsi.h"
#include "scsiDevice.h"
#include "scsiTape.h"
#include "devSCSITape.h"
#include "dev/scsi.h"
#include "stdlib.h"

#include "dbg.h"
int SCSITapeDebug = FALSE;



/*
 *----------------------------------------------------------------------
 *
 * InitTapeDevice --
 *
 *	Initialize the device driver state for a SCSI Tape drive.
 *
 * Results:
 *	SUCCESS.  If the tape driver is successfully initialized. A 
 *	Sprite error code otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
InitTapeDevice(devicePtr, devPtr)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    ScsiDevice *devPtr;		/* Attached SCSI tape. */
{
    ScsiTape	tapeData;	
    ScsiTape	*tapePtr;
    ReturnStatus status;
    int		i;

    /*
    * Determine the type of device from the inquiry return by the
    * attach. Reject device if not of tape type. If the target 
    * didn't respond to the INQUIRY command we assume that it
    * just a stupid tape.
    */ 
    if ((devPtr->inquiryLength > 0) &&
	(((ScsiInquiryData *) (devPtr->inquiryDataPtr))->type != 
						    SCSI_TAPE_TYPE)) {
	return DEV_NO_DEVICE;
    } 
    /*
     * Do a quick ready test on the device.
     */
    status = DevScsiTestReady(devPtr);
    if (status != SUCCESS) {
	return status;
    }
    if (devicePtr->data == (ClientData) NIL) {
	tapePtr = &tapeData;
	bzero((char *) tapePtr, sizeof(ScsiTape));
	tapePtr->devPtr = devPtr;
	tapePtr->state = SCSI_TAPE_CLOSED;
	tapePtr->name = "SCSI Tape";
	tapePtr->blockSize = SCSI_TAPE_DEFAULT_BLOCKSIZE;
	tapePtr->tapeIOProc = DevSCSITapeFixedBlockIO;
	tapePtr->errorProc = DevSCSITapeError;
	tapePtr->specialCmdProc =  DevSCSITapeSpecialCmd;
    } else {
	tapePtr = (ScsiTape *) (devicePtr->data);
    }
    for (i = 0; i < devNumSCSITapeTypes; i++) {
	status = (devSCSITapeAttachProcs[i])(devicePtr,devPtr,tapePtr);
	if (status == SUCCESS) {
	    break;
	}
    }
    /*
     * Allocate and return the ScsiTape structure in the data field of the
     * Fs_Device.
     */
    if ((status == SUCCESS) && devicePtr->data == (ClientData) NIL) { 
	tapePtr = (ScsiTape *) malloc(sizeof(ScsiTape));
	*tapePtr = tapeData;
        devicePtr->data = (ClientData)tapePtr;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * SetupCommand --
 *
 *	A variation on DevScsiGroup0Cmd that creates a control block
 *	designed for tape drives.  SCSI tape drives read from the current
 *	tape position, so there is only a block count, no offset.  There
 *	is a special code that modifies the command in the tape control
 *	block.  The value of the code is a function of the command and the
 *	type of the tape drive (ugh.)  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the various fields in the tape control block.
 *
 *----------------------------------------------------------------------
 */
static void
SetupCommand(devPtr, command, code, len, scsiCmdPtr)
    ScsiDevice	*devPtr;	/* Scsi device for command. */
    int command;		/* One of SCSI_* commands */
    unsigned int code;		/* Value for "code" field of command. The code
				 * field is the low 4 bits of the 2nd byte. */
    unsigned int len;		/* Length of the data in bytes or blocks. */
    ScsiCmd	*scsiCmdPtr;	/* Command block to fill in. */
{
   DevScsiGroup0Cmd(devPtr, command, ((code&0xf) << 16) | (len>>8),
		    (len & 0xff), scsiCmdPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeError --
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
ReturnStatus
DevSCSITapeError(tapePtr, statusByte, senseLength, senseDataPtr)
    ScsiTape	 *tapePtr;	/* SCSI Tape that's complaining. */
    unsigned char statusByte;	/* The status byte of the command. */
    int		 senseLength;	/* Length of SCSI sense data in bytes. */
    char	 *senseDataPtr;	/* Sense data. */
{
    ScsiStatus *statusPtr = (ScsiStatus *) &statusByte;
    ScsiClass0Sense *sensePtr = (ScsiClass0Sense *) senseDataPtr;
    char	*name = tapePtr->devPtr->locationName;
    char	errorString[MAX_SCSI_ERROR_STRING];
    ReturnStatus	status;

    /*
     * Check for status byte to see if the command returned sense
     * data. If no sense data exists then we only have the status
     * byte to look at.
     */
    if (!statusPtr->check) {
	if (SCSI_RESERVED_STATUS(statusByte) || statusPtr->intStatus) {
	    printf("Warning: %s at %s unknown status byte 0x%x\n",
		   tapePtr->name, name, statusByte);
	    return SUCCESS;
	} 
	if (statusPtr->busy) {
	    return DEV_OFFLINE;
	}
	return SUCCESS;
    }
    if (senseLength == 0) {
	 printf("Warning: %s at %s error: no sense data\n", tapePtr->name,name);
	 return DEV_NO_SENSE;
    }
    if (DevScsiMapClass7Sense(senseLength, senseDataPtr,&status, errorString)) {
	ScsiClass7Sense	*s = (ScsiClass7Sense *) senseDataPtr;
	if (errorString[0]) {
	     printf("Warning: %s at %s error: %s\n", tapePtr->name, name, 
		    errorString);
	}
	if (status == SUCCESS) {
	    if (s->fileMark) {
		/*
		 * Hit the file mark after reading good data. Setting this 
		 * bit causes the next read to return zero bytes.
		 */
		tapePtr->state |= SCSI_TAPE_AT_EOF;
	    } else if (s->endOfMedia) {
		status = DEV_END_OF_TAPE;
	    }
	}
	return status;
    }
    /*
     * If its not a class 7 error it must be Old style sense data..
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
	    printf("Warning: %s at %s: Sense error (%d-%d) at <%x> ",
			     tapePtr->name, name, class, code, addr);
	    if (devScsiNumErrors[class] > code) {
		printf("%s", devScsiErrors[class][code]);
	    }
	    printf("\n");
	    status = DEV_INVALID_ARG;
    } 
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeSpecialCmd --
 *
 *	Performance a special tape command on a SCSI Tape drive. This 
 *	routine should work on any SCSI Tape drive adhering to the SCSI
 *	common command set.
 *
 * Results:
 *	The sprite return status.
 *
 * Side effects:
 *	Command dependent.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevSCSITapeSpecialCmd(tapePtr, command, count)
    ScsiTape	*tapePtr;	/* Target drive for command. */
    int		command;	/* Command to be performed. */
    int		count;		/* Argument to command. */
{
    ReturnStatus status;
    ScsiCmd	 scsiTapeCmd;
    unsigned char statusByte;
    int		senseLength;
    char	senseBuffer[SCSI_MAX_SENSE_LEN];
    unsigned int	code;
    int		scsiCmd;

   code = 0;
   switch (command) {
    case IOC_TAPE_SKIP_FILES:
    case IOC_TAPE_SKIP_BLOCKS: 
	scsiCmd = SCSI_SPACE;
	code = (command == IOC_TAPE_SKIP_FILES) ? 1 : 0;
	break;
    case IOC_TAPE_REWIND:
	scsiCmd = SCSI_REWIND;
	count = 0;
	break;
    case IOC_TAPE_WEOF:
	scsiCmd = SCSI_WRITE_EOF;
	break;
    case IOC_TAPE_ERASE:
	scsiCmd = SCSI_ERASE_TAPE;
	count = 0;
	break;
    case IOC_TAPE_NO_OP:
	scsiCmd = SCSI_TEST_UNIT_READY;
	count = 0;
	break;
    case IOC_TAPE_RETENSION:
	return DEV_INVALID_ARG;
    default:
	panic("DevSCSITapeSpecialCmd: Unknown command %d\n", command);
    }
    SetupCommand(tapePtr->devPtr, scsiCmd, code, (unsigned)count, &scsiTapeCmd);
    scsiTapeCmd.buffer = (char *) 0;
    scsiTapeCmd.bufferLen = 0;
    scsiTapeCmd.dataToDevice = FALSE;
    senseLength = SCSI_MAX_SENSE_LEN;
    status = DevScsiSendCmdSync(tapePtr->devPtr, &scsiTapeCmd, &statusByte,
				&code, &senseLength, senseBuffer);

    if (status == SUCCESS) {
	status = (tapePtr->errorProc)(tapePtr,statusByte, senseLength, 
				      senseBuffer);
    }
    return(status);

}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeVariableIO --
 *
 *      Low level routine to read or write an SCSI tape device using a
 *	variable size block format.   Each IO involves some number of
 *	bytes.
 *
 * Results:
 *	The Sprite return status.
 *
 * Side effects:
 *	Tape is written or read.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITapeVariableIO(tapePtr,command, buffer, countPtr)
    register ScsiTape *tapePtr; 	/* State info for the tape */
    int command;			/* SCSI_READ, SCSI_WRITE, etc. */
    char *buffer;			/* Target buffer */
    int *countPtr;			/* In/Out byte count. */
{
    ReturnStatus status;
    ScsiCmd	 scsiTapeCmd;
    unsigned char statusByte;
    int		senseLength;
    char	senseBuffer[SCSI_MAX_SENSE_LEN];


    /* 
     * Setup the command, a code value of zero means variable block.
     */
    SetupCommand(tapePtr->devPtr, command, 0,  (unsigned)*countPtr, 
		&scsiTapeCmd);
    scsiTapeCmd.buffer = buffer;
    scsiTapeCmd.bufferLen = *countPtr;
    scsiTapeCmd.dataToDevice = (command == SCSI_WRITE);
    senseLength = SCSI_MAX_SENSE_LEN;
    status = DevScsiSendCmdSync(tapePtr->devPtr, &scsiTapeCmd, &statusByte,
				countPtr, &senseLength, senseBuffer);

    if (status == SUCCESS) {
	status = (tapePtr->errorProc)(tapePtr,statusByte, senseLength, 
				      senseBuffer);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeFixedBlockIO --
 *
 *      Low level routine to read or write an SCSI tape device using a
 *	fixed size block format.   Each IO involves one or more tape blocks 
 *	and must be  multiples of the underlying device block size.
 *
 * Results:
 *	The Sprite return status.
 *
 * Side effects:
 *	Tape is written or read.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevSCSITapeFixedBlockIO(tapePtr, command, buffer, countPtr)
    register ScsiTape *tapePtr; 	/* State info for the tape */
    int command;			/* SCSI_READ, SCSI_WRITE, etc. */
    char *buffer;			/* Target buffer */
    int *countPtr;			/* In/Out byte count. */
{
    ReturnStatus status;
    ScsiCmd	 scsiTapeCmd;
    unsigned char statusByte;
    int		senseLength;
    char	senseBuffer[SCSI_MAX_SENSE_LEN];
    int		lengthInBlocks;

   /*
     * For simplicity reads and writes that are multiple of the block size
     * are rejected.
     */
    if ((*countPtr % (tapePtr->blockSize)) != 0) {
	*countPtr = 0;
	return DEV_INVALID_ARG;
    }
    lengthInBlocks = *countPtr / tapePtr->blockSize;
    /*
     * Set up the command with a code value of 1 meaning fixed block.
     */
    SetupCommand(tapePtr->devPtr, command, 1, (unsigned)lengthInBlocks,
		 &scsiTapeCmd);
    scsiTapeCmd.buffer = buffer;
    scsiTapeCmd.bufferLen = *countPtr;
    scsiTapeCmd.dataToDevice = (command == SCSI_WRITE);
    senseLength = SCSI_MAX_SENSE_LEN;
    status = DevScsiSendCmdSync(tapePtr->devPtr, &scsiTapeCmd, &statusByte,
				countPtr, &senseLength, senseBuffer);

    if (status == SUCCESS) {
	status = (tapePtr->errorProc)(tapePtr,statusByte, senseLength, 
				      senseBuffer);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeOpen --
 *
 *	Open a SCSI tape drive as a file.  This routine verifies the
 *	drives existance and sets any special mode flags.
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
DevSCSITapeOpen(devicePtr, useFlags, token)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    Fs_NotifyToken token;	/* Call-back token for input, unused here */
{
    ReturnStatus status;
    ScsiDevice *devPtr;
    ScsiTape *tapePtr;

    tapePtr = (ScsiTape *) (devicePtr->data);
    if (tapePtr == (ScsiTape *) NIL) {
	/*
	 * Ask the HBA to set up the path to the device with FIFO ordering
	 * of requests.
	 */
	devPtr = DevScsiAttachDevice(devicePtr, DEV_QUEUE_FIFO_INSERT);
	if (devPtr == (ScsiDevice *) NIL) {
	    return DEV_NO_DEVICE;
	}
    } else { 
	/*
	 * If the tapePtr is already attached to the device it must be
	 * busy.
	 */
	 return(FS_FILE_BUSY);
    }
    status = InitTapeDevice(devicePtr, devPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeRead --
 *
 *	Read from a raw SCSI Tape.  The offset is ignored because
 *	you can't seek the SCSI tape drive, only rewind it.
 *
 * Results:
 *	The return status of the read.
 *
 * Side effects:
 *	The process will sleep waiting for the I/O to complete.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevSCSITapeRead(devicePtr, readPtr, replyPtr)
    Fs_Device *devicePtr;	/* Handle for raw SCSI tape device */
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus error;	
    ScsiTape *tapePtr;
    int	totalTransfer;
    int transferSize;
    int	maxXfer;

    tapePtr = (ScsiTape *)(devicePtr->data);
    if (tapePtr->state & SCSI_TAPE_AT_EOF) {
	/*
	 * Force the use of the SKIP_FILES control to get past the end of
	 * the file on tape.
	 */
	replyPtr->length = 0;
	return(SUCCESS);
    }
    /*
     * Break up the IO into piece the device/HBA can handle.
     */
    error = SUCCESS;
#ifdef GOOD
    maxXfer = tapePtr->devPtr->maxTransferSize;
#else
    maxXfer = 1024;
#endif
    totalTransfer = 0;
    while((readPtr->length > 0) && (error == SUCCESS)) {  
	int	byteCount;
	transferSize = (readPtr->length > maxXfer) ? maxXfer : readPtr->length;
	byteCount = transferSize;
	error = (tapePtr->tapeIOProc)(tapePtr, SCSI_READ,
				  readPtr->buffer + totalTransfer, &byteCount);
	/*
	 * A short read implies we hit end of file or end of tape. 
	 */
	totalTransfer += byteCount;
	readPtr->length -= byteCount;
	if (byteCount < transferSize) {
	    break;
	}
    }
    replyPtr->length = totalTransfer;
    /*
     * Special check against funky end-of-file situations.  The Emulex tape
     * doesn't compute a correct residual when it hits the file mark
     * on the tape.
     */
    if (error == DEV_END_OF_TAPE) {
	replyPtr->length = 0;
	error = SUCCESS;
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeWrite --
 *
 *	Write to a raw SCSI tape.
 *
 * Results:
 *	A Sprite error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevSCSITapeWrite(devicePtr, writePtr, replyPtr)
    Fs_Device *devicePtr;	/* Handle of raw tape device */
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
    ReturnStatus error;	
    ScsiTape *tapePtr;
    int totalTransfer;
    int transferSize;
    int	maxXfer;

    tapePtr = (ScsiTape *)(devicePtr->data);
    /*
     * Break up the IO into piece the device/HBA can handle.
     */
    error = SUCCESS;
    maxXfer = tapePtr->devPtr->maxTransferSize;
    totalTransfer = 0;
    while((writePtr->length > 0) && (error == SUCCESS)) {  
	int	byteCount;
	transferSize = (writePtr->length > maxXfer) ? maxXfer : writePtr->length;
	byteCount = transferSize;
	error = (tapePtr->tapeIOProc)(tapePtr, SCSI_WRITE,
			      writePtr->buffer + totalTransfer, &byteCount);
	/*
	 * A short write implies we hit end of tape. 
	 */
	totalTransfer += byteCount;
	writePtr->length -= transferSize;
	if (byteCount < transferSize) {
	    break;
	}
    }
    writePtr->length = totalTransfer;
    if (error == SUCCESS) {
	tapePtr->state |= SCSI_TAPE_WRITTEN;
    }
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSITapeIOControl --
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
DevSCSITapeIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device *devicePtr;
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    ScsiTape *tapePtr;
    ReturnStatus status = SUCCESS;

    tapePtr = (ScsiTape *)(devicePtr->data);
     if ((ioctlPtr->command & ~0xffff) == IOC_SCSI) {
	 status = DevScsiIOControl(tapePtr->devPtr, ioctlPtr, replyPtr);
	 return status;

     }

    switch(ioctlPtr->command) {
	case IOC_REPOSITION: {
	    Ioc_RepositionArgs *repoArgsPtr;
	    repoArgsPtr = (Ioc_RepositionArgs *)ioctlPtr->inBuffer;

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
			status = (tapePtr->specialCmdProc)(tapePtr, 
						   IOC_TAPE_SKIP_FILES, 1);
		    }
		    break;
	    }
	    break;
	}
	case IOC_TAPE_COMMAND: {
	    Dev_TapeCommand *cmdPtr = (Dev_TapeCommand *)ioctlPtr->inBuffer;
	    if (ioctlPtr->inBufSize < sizeof(Dev_TapeCommand)) {
		return(DEV_INVALID_ARG);
	    }
	    switch (cmdPtr->command) {
		case IOC_TAPE_WEOF: {
		    status = (tapePtr->specialCmdProc)(tapePtr, IOC_TAPE_WEOF,
							cmdPtr->count);
		    break;
		}
		case IOC_TAPE_RETENSION: {
		    status = (tapePtr->specialCmdProc)(tapePtr,
						  IOC_TAPE_RETENSION, 1);
		    break;
		}
		case IOC_TAPE_OFFLINE:
		case IOC_TAPE_REWIND: {
rewind:		    /*
		     * Have to write an end-of-file mark if the last thing done
		     * was a write...
		     */
		    if (tapePtr->state & SCSI_TAPE_WRITTEN) {
			status = (tapePtr->specialCmdProc)(tapePtr,
						   IOC_TAPE_WEOF, 1);
			tapePtr->state &= ~SCSI_TAPE_WRITTEN;
		    }
		    status = (tapePtr->specialCmdProc)(tapePtr,
						  IOC_TAPE_REWIND, 1);
		    break;
		}
		case IOC_TAPE_SKIP_BLOCKS: {
		    status = (tapePtr->specialCmdProc)(tapePtr, 
					IOC_TAPE_SKIP_BLOCKS, cmdPtr->count);
		    if (status == DEV_END_OF_TAPE) {
			status = SUCCESS;
		    }
		    break;
		case IOC_TAPE_SKIP_FILES:
		    status = (tapePtr->specialCmdProc)(tapePtr, 
					IOC_TAPE_SKIP_FILES,  cmdPtr->count);
		    if (status == DEV_END_OF_TAPE) {
			status = SUCCESS;
		    }
		    break;
		}
		case IOC_TAPE_BACKUP_BLOCKS:
		case IOC_TAPE_BACKUP_FILES:
		    status = DEV_INVALID_ARG;
		    break;
		case IOC_TAPE_ERASE: {
		    status = (tapePtr->specialCmdProc)(tapePtr, 
							IOC_TAPE_ERASE,1);
		    break;
		}
		case IOC_TAPE_NO_OP: {
		    status = (tapePtr->specialCmdProc)(tapePtr, 
						      IOC_TAPE_NO_OP,1);
		    break;
		}
	    }
	    break;
	}
	case IOC_TAPE_STATUS: {
		return(DEV_INVALID_ARG);
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
 *	Close a raw SCSI tape file.  This checks the unit number to
 *	determine if the tape should be rewound.  Units 0 and 8
 *	are rewind always, units 1 and 9 are no-rewind..
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
DevSCSITapeClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;
    int		useFlags;	/* FS_READ | FS_WRITE */
    int		openCount;	/* Number of times device open. */
    int		writerCount;	/* Number of times device open for writing. */
{
    ScsiTape *tapePtr;
    ReturnStatus status = SUCCESS;

    tapePtr = (ScsiTape *)(devicePtr->data);
    if (openCount > 0) {
	return(SUCCESS);
    }
    if (tapePtr->state & SCSI_TAPE_WRITTEN) {
	/*
	 * Make sure an end-of-file mark is at the end of the file on tape.
	 */
	status = (tapePtr->specialCmdProc)(tapePtr, IOC_TAPE_WEOF,1);
    }
    /*
     * Use the unit number to indicate rewind or no-rewind.  An
     * ``even'' number (0 and 8) means rewind.
     */
    if ((devicePtr->unit % 2) == 0) {
	status = (tapePtr->specialCmdProc)(tapePtr, IOC_TAPE_REWIND,1);
    }
    (void) DevScsiReleaseDevice(tapePtr->devPtr);
    free((char *)tapePtr);
    devicePtr->data = (ClientData) NIL;
    return(status);
}
