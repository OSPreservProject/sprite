/* 
 * devSCSIEmulex.c --
 *
 *      Procedures that set up command blocks and process sense
 *	data for Emulex tape drives.
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
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devSCSITape.h"
#include "devSCSIEmulex.h"

void EmulexSetup();
void EmulexStatus();
ReturnStatus EmulexError();


/*
 *----------------------------------------------------------------------
 *
 * DevEmulexInit --
 *
 *	Initialize the DevSCSITape state for a Emulex drive.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the type and call-back procedures.
 *
 *----------------------------------------------------------------------
 */
void
DevEmulexInit(tapePtr)
    DevSCSITape	*tapePtr;	/* Tape drive state */
{
    tapePtr->type = SCSI_EMULEX;
    tapePtr->blockSize = DEV_EMULEX_BLOCK_SIZE;
    tapePtr->setupProc = EmulexSetup;
    tapePtr->statusProc = EmulexStatus;
    tapePtr->errorProc = EmulexError;
}

/*
 *----------------------------------------------------------------------
 *
 * EmulexSetup --
 *
 *	This customizes the control block and sets the count and dmaCount
 *	to be correct for Emulex based tape drives.
 *
 * Results:
 *	Various reserved bits may be set in the control block.
 *	count is set for the count field in the command block.
 *	dmaCount is set for the dma transfer count.
 *
 * Side effects:
 *	The tapePtr->state may be modified regarding EOF and RETENSION.
 *
 *----------------------------------------------------------------------
 */
void
EmulexSetup(tapePtr, commandPtr, controlBlockPtr, countPtr, dmaCountPtr)
    DevSCSITape	*tapePtr;	/* Tape drive state */
    int *commandPtr;		/* In/Out tape command */
    DevSCSITapeControlBlock *controlBlockPtr;	/* CMD Block to set up */
    int *countPtr;		/* In - Transfer count, blocks or bytes!
				 * Out - The proper byte count for CMD block */
    int *dmaCountPtr;		/* In - Transfer count, blocks or bytes!
				 * Out - The proper DMA byte count for caller */
{
    switch (*commandPtr) {
	case SCSI_TEST_UNIT_READY:
	    break;
	case SCSI_REWIND:
	    /*
	     * Can do tape retension by using SCSI_START_STOP
	     * and setting count to 3 (wild but true)
	     */
	    if (tapePtr->state & SCSI_TAPE_RETENSION) {
		tapePtr->state &= ~SCSI_TAPE_RETENSION;
		*commandPtr = SCSI_START_STOP;
		*dmaCountPtr = 0;
		*countPtr = 3;
	    }
	    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_REQUEST_SENSE:
	    *dmaCountPtr = *countPtr = sizeof(DevEmulexTapeSense);
	    break;
	case SCSI_MODE_SELECT:
	    break;
	case SCSI_READ:
	case SCSI_WRITE:
	    controlBlockPtr->code = 1;
	    *countPtr /= tapePtr->blockSize;
	    break;
	case SCSI_WRITE_EOF:
	    *countPtr = 1;
	    *dmaCountPtr = 0;
	    break;
	case SCSI_SPACE:
	case SCSI_SPACE_FILES:
	    *dmaCountPtr = 0;
	    controlBlockPtr->code = 1;
	    *commandPtr = SCSI_SPACE;
	    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_SPACE_BLOCKS:
	    *dmaCountPtr = 0;
	    controlBlockPtr->code = 0;
	    *commandPtr = SCSI_SPACE;
	    break;
	case SCSI_SPACE_EOT:
	    *dmaCountPtr = 0;
	    controlBlockPtr->code = 3;
	    *commandPtr = SCSI_SPACE;
	    tapePtr->state |= SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_ERASE_TAPE:
	    controlBlockPtr->code = 1;
	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * EmulexStatus --
 *
 *	Support for the IOC_TAPE_STATUS I/O control.  This generates
 *	a status error word from sense data.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
EmulexStatus(devPtr, statusPtr)
    DevSCSIDevice *devPtr;
    Dev_TapeStatus *statusPtr;
{
    /*
     * Return byte2 from the class 7 extended sense plus
     * the standard SCSI error byte.  Byte2 of Extended Sense
     * has a fileMark bit, endOfMedia bit, badBlockLen bit, a reserved bit,
     * and a 4 bit sense key.
     */
    DevEmulexTapeSense *emulexSensePtr =
	    (DevEmulexTapeSense *)devPtr->scsiPtr->senseBuffer;
    unsigned char *senseBytes = (unsigned char *)devPtr->scsiPtr->senseBuffer;

    statusPtr->type = DEV_TAPE_EMULEX;
    statusPtr->errorReg = senseBytes[2] | (emulexSensePtr->error << 24);

}

/*
 *----------------------------------------------------------------------
 *
 * EmulexError --
 *
 *	Handle error conditions from a Emulex based tape drive.
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
EmulexError(devPtr, sensePtr)
    DevSCSIDevice *devPtr;
    DevSCSISense *sensePtr;
{
    register ReturnStatus status = SUCCESS;
    DevSCSITape *tapePtr = (DevSCSITape *)devPtr->data;
    register DevEmulexTapeSense *emulexSensePtr =
	    (DevEmulexTapeSense *)sensePtr;

/*
 * One way to do this is look at the extended sense "key", however
 * this isn't fully understood yet.  Instead, the Emulex has its own
 * special bits, plus it returns a regular SCSI error code.
 */
#ifdef notdef
    switch (emulexSensePtr->extSense.key) {
	case SCSI_NO_SENSE:
	    break;
	case SCSI_RECOVERABLE:
	    /*
	     * The drive recovered from an error.
	     */
	    printf("Warning: SCSI-%d drive %d, recoverable error\n",
			devPtr->scsiPtr->number, devPtr->slaveID);
	    break;
	case SCSI_NOT_READY:
	    status = DEV_OFFLINE;
	    break;
	case SCSI_ILLEGAL_REQUEST:
	    /*
	     * Probably a programming error.
	     */
	    printf("Warning: SCSI-%d drive %d, illegal request %d\n",
			devPtr->scsiPtr->number, devPtr->slaveID,
			command);
	    status = DEV_INVALID_ARG;
	    break;
	case SCSI_MEDIA_ERROR:
	case SCSI_HARDWARE_ERROR:
	    printf("Warning: SCSI-%d drive %d, hard class7 error %d\n",
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
	    printf("Warning: SCSI-%d drive %d, \"blank check\"\n",
		devPtr->scsiPtr->number, devPtr->slaveID);
	    printf("\tInfo bytes 0x%x 0x%x 0x%x 0x%x\n",
		extSensePtr->info1 & 0xff,
		extSensePtr->info2 & 0xff,
		extSensePtr->info3 & 0xff,
		extSensePtr->info4 & 0xff);
	    break;
	case SCSI_MEDIA_CHANGE:
	case SCSI_VENDOR:
	case SCSI_POWER_UP_FAILURE:
	case SCSI_ABORT_KEY:
	case SCSI_EQUAL:
	case SCSI_OVERFLOW:
	    printf("Warning: SCSI-%d drive %d, unsupported class7 error %d\n",
		devPtr->scsiPtr->number, devPtr->slaveID,
		extSensePtr->key);
	    status = DEV_HARD_ERROR;
	    break;
    }
#endif notdef
    switch (emulexSensePtr->error) {
	case SCSI_NOT_READY:
	    status = DEV_OFFLINE;
	    break;
	case SCSI_NOT_LOADED:
	    status = DEV_NO_MEDIA;
	    break;
	case SCSI_INSUF_CAPACITY:
	    printf("Warning: Emulex: Insufficient tape capacity");
	    /* fall thru */
	case SCSI_END_OF_MEDIA:
	    status = DEV_END_OF_TAPE;
	    break;
	case SCSI_HARD_DATA_ERROR:
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_WRITE_PROTECT: {
	    register int command = devPtr->scsiPtr->command;
	    if (command == SCSI_WRITE ||
		command == SCSI_ERASE_TAPE ||
		command == SCSI_WRITE_EOF) {
		status = FS_NO_ACCESS;
	    }
	    break;
	}
	case SCSI_CORRECTABLE_ERROR:
	    printf("Warning: SCSI-%d drive %d, correctable error",
		    devPtr->scsiPtr->number, devPtr->targetID);
	    break;
	case SCSI_FILE_MARK:
	    if (devPtr->scsiPtr->command == SCSI_READ) {
		/*
		 * Hit the file mark after reading good data.
		 * Setting this bit causes the next read to
		 * return zero bytes.
		 */
		tapePtr->state |= SCSI_TAPE_AT_EOF;
	    }
	    break;
	case SCSI_INVALID_COMMAND:
	    printf("Warning: SCSI-%d drive %d, invalid command 0x%x",
		    devPtr->scsiPtr->number, devPtr->targetID,
		    devPtr->scsiPtr->command);
	    break;

	case SCSI_UNIT_ATTENTION:
	    /*
	     * The drive has been reset since the last command.
	     * This status will be handled by the retry in
	     * the tape open routine.
	     */
	    status = DEV_NO_MEDIA;
	    break;
    }
    return(status);
}
