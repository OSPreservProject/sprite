/* 
 * devSCSITape.c --
 *
 *      The standard Open, Read, Write, IOControl, and Close operations
 *      are defined here for the SCSI tape.
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
#include "dev/tape.h"
#include "byte.h"

#include "dbg.h"
int SCSITapeDebug = FALSE;

#define SECTORS_PER_FRAGMENT	(FS_FRAGMENT_SIZE / DEV_BYTES_PER_SECTOR)

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
	if (status == DEV_TIMEOUT) {
	    break;
	}
	status = DevSCSIRequestSense(devPtr->scsiPtr, devPtr);
    } while (status != SUCCESS && ++retries < 3);
    if (status == SUCCESS) {
	/*
	 * Check for EMULUX controller, because it comes up in the
	 * wrong mode (QIC_24) and needs to be reset to use QIC 2 format.
	 */
	if (tapePtr->type == SCSI_EMULUX) {
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
     * Special check against funky end-of-file situations.  The Emulux tape
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
    Dev_TapeCommand *cmdPtr = (Dev_TapeCommand *)inBuffer;

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
		    while (cmdPtr->count--) {
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
		    while (cmdPtr->count--) {
			status = DevSCSITapeIO(SCSI_SPACE_BLOCKS, devPtr,
					(char *)0, &count);
			if (status == DEV_END_OF_TAPE) {
			    status = SUCCESS;
			}
		    }
		    break;
		case IOC_TAPE_SKIP_FILES:
		    count = 1;
		    while (cmdPtr->count--) {
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
	    Dev_TapeStatus *statusPtr = (Dev_TapeStatus *)outBuffer;
	    if (outBufSize < sizeof(Dev_TapeStatus)) {
		return(DEV_INVALID_ARG);
	    }
	    status = DevSCSIRequestSense(devPtr->scsiPtr, devPtr);
	    
	    statusPtr->statusReg = devPtr->scsiPtr->regsPtr->control;
	    if (tapePtr->type == SCSI_SYSGEN) {
		/*
		 * Return the first two sense bytes from the sysgen controller.
		 * This has the standard QIC 02 bits, see the typedef
		 * for DevQICIISense.  In the high part of the error word
		 * return the standard SCSI error byte that contains the
		 * error class and code.
		 */
		unsigned char *senseBytes;
		statusPtr->type = DEV_TAPE_SYSGEN;
		senseBytes = devPtr->scsiPtr->senseBuffer->sense;
		statusPtr->errorReg = ((senseBytes[1] & 0xFF) << 8) |
				    (senseBytes[0] & 0xFF) |
				   (devPtr->scsiPtr->senseBuffer->error << 24);
	    } else {
		/*
		 * Return one byte from the class 7 extended sense, plus
		 * the standard SCSI error byte.
		 */
		DevEmuluxSense *emuluxSensePtr =
			(DevEmuluxSense *)devPtr->scsiPtr->senseBuffer;
		char *senseBytes = (char *)devPtr->scsiPtr->senseBuffer;

		statusPtr->type = DEV_TAPE_EMULUX;
		statusPtr->errorReg = (senseBytes[2] & 0xFF) |
				   (emuluxSensePtr->error << 24);
	    }
	    statusPtr->residual = devPtr->scsiPtr->residual;
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

    if (openCount > 0) {
	return(SUCCESS);
    }

    devPtr = scsiTape[devicePtr->unit / DEV_TAPES_PER_CNTRLR];
    if (devPtr == (DevSCSIDevice *)0 || devPtr == (DevSCSIDevice *)NIL) {
	return(DEV_NO_DEVICE);
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
    register DevEmuluxModeSelParams *modeParamsPtr;
    int count;

    if (modeCommand == SCSI_MODE_QIC_24) {
	Sys_Panic(SYS_WARNING, "SCSI Mode select won't do QIC 24 format");
	return(DEV_INVALID_ARG);
    } else if (tapePtr->type != SCSI_EMULUX) {
	Sys_Panic(SYS_WARNING, "SCSI Mode select won't do Sysgen drives");
	return(DEV_INVALID_ARG);
    }
    modeParamsPtr = (DevEmuluxModeSelParams *)scsiPtr->labelBuffer;
    Byte_Zero(sizeof(DevEmuluxModeSelParams), (Address)modeParamsPtr);
    modeParamsPtr->header.bufMode = 1;
    modeParamsPtr->header.blockLength = sizeof(DevEmuluxModeSelBlock);
    modeParamsPtr->block.density = SCSI_EMULUX_QIC_02;
    /*
     * The rest of the fields in the select params can be left zero.
     */
    count = sizeof(DevEmuluxModeSelParams);
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
    int command = devPtr->scsiPtr->command;

    switch (tapePtr->type) {
	case SCSI_UNKNOWN: {
	    Sys_Panic(SYS_WARNING, "Unknown tape drive type");
	    if (sensePtr->error != SCSI_NO_SENSE_DATA) {
		register int class = (sensePtr->error & 0x70) >> 4;
		register int code = sensePtr->error & 0xF;
		register int addr;
		addr = (sensePtr->highAddr << 16) |
			(sensePtr->midAddr << 8) |
			sensePtr->lowAddr;
		Sys_Printf("SCSI-%d: Sense error (%d-%d) at <%x> ",
				 devPtr->scsiPtr->number, class, code, addr);
		if (scsiNumErrors[class] > code) {
		    Sys_Printf("%s", scsiErrors[class][code]);
		}
		Sys_Printf("\n");
		status = DEV_INVALID_ARG;
	    }
	    break;
	}
	case SCSI_SYSGEN: {
	    register DevQICIISense *qicSensePtr;
	    qicSensePtr = (DevQICIISense *)sensePtr->sense;
	    /*
	     * Check for special sense data returned by sysgen tape drives.
	     */
	    if (qicSensePtr->noCartridge) {
		status = DEV_OFFLINE;
	    } else if (qicSensePtr->noDrive) {
		status = DEV_NO_DEVICE;
	    } else if (qicSensePtr->dataError || qicSensePtr->retries) {
		status = DEV_HARD_ERROR;
	    } else if (qicSensePtr->endOfTape) {
		status = DEV_END_OF_TAPE;
	    } else {
		switch (command) {
		    case SCSI_TEST_UNIT_READY:
		    case SCSI_OPENING:
		    case SCSI_SPACE:
			break;
		    case SCSI_READ:
			if (qicSensePtr->fileMark) {
			    /*
			     * Hit the file mark after reading good data.
			     * Setting this bit causes the next read to
			     * return zero bytes.
			     */
			    tapePtr->state |= SCSI_TAPE_AT_EOF;
			}
			break;
		    case SCSI_WRITE:
		    case SCSI_WRITE_EOF:
		    case SCSI_ERASE_TAPE:
			if (qicSensePtr->writeProtect) {
			    status = FS_NO_ACCESS;
			}
			break;
		}
	    }
	    break;
	}
	case SCSI_EMULUX: {
	    register DevEmuluxSense *emuluxSensePtr;
	    register DevSCSIExtendedSense *extSensePtr;
	    emuluxSensePtr = (DevEmuluxSense *)sensePtr;
	    extSensePtr = (DevSCSIExtendedSense *)sensePtr;
/*
 * One way to do this is look at the extended sense "key", however
 * this isn't fully understood yet.  Instead, the Emulux has its own
 * special bits, plus it returns a regular SCSI error code.
 */
#ifdef notdef
	    switch (extSensePtr->key) {
		case SCSI_NO_SENSE:
		    break;
		case SCSI_RECOVERABLE:
		    /*
		     * The drive recovered from an error.
		     */
		    Sys_Panic(SYS_WARNING,
				"SCSI-%d drive %d, recoverable error\n",
				devPtr->scsiPtr->number, devPtr->slaveID);
		    break;
		case SCSI_NOT_READY:
		    status = DEV_OFFLINE;
		    break;
		case SCSI_ILLEGAL_REQUEST:
		    /*
		     * Probably a programming error.
		     */
		    Sys_Panic(SYS_WARNING,
				"SCSI-%d drive %d, illegal request %d\n",
				devPtr->scsiPtr->number, devPtr->slaveID,
				command);
		    status = DEV_INVALID_ARG;
		    break;
		case SCSI_MEDIA_ERROR:
		case SCSI_HARDWARE_ERROR:
		    Sys_Panic(SYS_WARNING,
				"SCSI-%d drive %d, hard class7 error %d\n",
				devPtr->scsiPtr->number, devPtr->slaveID,
				extSensePtr->key);
		    status = DEV_HARD_ERROR;
		    break;
		case SCSI_WRITE_PROTECT:
		    if (command == SCSI_WRITE ||
			command == SCSI_WRITE_EOF ||
			command == SCSI_ERASE_TAPE) {
			status = FS_NO_ACCESS;
		    }
		    break;
		case SCSI_DIAGNOSTIC:
		    Sys_Panic(SYS_WARNING,
			"SCSI-%d drive %d, \"blank check\"\n",
			devPtr->scsiPtr->number, devPtr->slaveID);
		    Sys_Printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
			extSensePtr->info1 & 0xff,
			extSensePtr->info2 & 0xff,
			extSensePtr->info3 & 0xff,
			extSensePtr->info4 & 0xff);
		    break;
		case SCSI_MEDIA_CHANGE:
		case SCSI_VENDOR:
		case SCSI_POWER_UP_FAILURE:
		case SCSI_ABORT:
		case SCSI_EQUAL:
		case SCSI_OVERFLOW:
		    Sys_Panic(SYS_WARNING,
			"SCSI-%d drive %d, unsupported class7 error %d\n",
			devPtr->scsiPtr->number, devPtr->slaveID,
			extSensePtr->key);
		    status = DEV_HARD_ERROR;
		    break;
	    }
#endif notdef
	    switch (emuluxSensePtr->error) {
		case SCSI_NOT_READY:
		    status = DEV_OFFLINE;
		    break;
		case SCSI_NOT_LOADED:
		    status = DEV_NO_MEDIA;
		    break;
		case SCSI_INSUF_CAPACITY:
		    Sys_Panic(SYS_WARNING,
				"Emulux: Insufficient tape capacity");
		    /* fall thru */
		case SCSI_END_OF_MEDIA:
		    status = DEV_END_OF_TAPE;
		    break;
		case SCSI_HARD_DATA_ERROR:
		    status = DEV_HARD_ERROR;
		    break;
		case SCSI_WRITE_PROTECT:
		    if (command == SCSI_WRITE ||
			command == SCSI_ERASE_TAPE ||
			command == SCSI_WRITE_EOF) {
			status = FS_NO_ACCESS;
		    }
		    break;
		case SCSI_CORRECTABLE_ERROR:
		    Sys_Panic(SYS_WARNING,
			    "SCSI-%d drive %d, correctable error",
			    devPtr->scsiPtr->number, devPtr->slaveID);
		    break;
		case SCSI_FILE_MARK:
		    if (command == SCSI_READ) {
			/*
			 * Hit the file mark after reading good data.
			 * Setting this bit causes the next read to
			 * return zero bytes.
			 */
			tapePtr->state |= SCSI_TAPE_AT_EOF;
		    }
		    break;
		case SCSI_INVALID_COMMAND:
		    Sys_Panic(SYS_WARNING,
			    "SCSI-%d drive %d, invalid command 0x%x",
			    devPtr->scsiPtr->number, devPtr->slaveID,
			    command);
		    break;

		case SCSI_UNIT_ATTENTION:
		    /*
		     * The drive has been reset sinse the last command.
		     * This status will be handled by the retry in
		     * the tape open routine.
		     */
		    status = DEV_NO_MEDIA;
		    break;
	    }
	    break;
	}
    }
    return(status);
}
