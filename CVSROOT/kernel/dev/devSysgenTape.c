/* 
 * devSCSISysgen.c --
 *
 *      Procedures that set up command blocks and process sense
 *	data for Sysgen tape drives.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "devInt.h"
#include "devSCSI.h"
#include "devSCSITape.h"
#include "devSCSISysgen.h"

void SysgenSetup();
void SysgenStatus();
ReturnStatus SysgenError();


/*
 *----------------------------------------------------------------------
 *
 * DevSysgenInit --
 *
 *	Initialize the DevSCSITape state for a Sysgen drive.
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
DevSysgenInit(tapePtr)
    DevSCSITape	*tapePtr;	/* Tape drive state */
{
    tapePtr->type = SCSI_SYSGEN;
    tapePtr->blockSize = DEV_SYSGEN_BLOCK_SIZE;
    tapePtr->setupProc = SysgenSetup;
    tapePtr->statusProc = SysgenStatus;
    tapePtr->errorProc = SysgenError;
}

/*
 *----------------------------------------------------------------------
 *
 * SysgenSetup --
 *
 *	This customizes the control block and sets the count and dmaCount
 *	to be correct for Sysgen based tape drives.
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
SysgenSetup(tapePtr, commandPtr, controlBlockPtr, countPtr, dmaCountPtr)
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
	     * Do a tape retension by setting vendor57 bit.
	     */
	    if (tapePtr->state & SCSI_TAPE_RETENSION) {
		tapePtr->state &= ~SCSI_TAPE_RETENSION;
		controlBlockPtr->vendor57 = 1;
	    }
	    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_REQUEST_SENSE:
	    *dmaCountPtr = *countPtr = sizeof(DevQICIISense);
	    break;
	case SCSI_READ:
	case SCSI_WRITE:
	    *countPtr /= tapePtr->blockSize;
	    break;
	case SCSI_WRITE_EOF:
	    *dmaCountPtr = 0;
	    *countPtr = 1;
	    break;
	case SCSI_SPACE:
	case SCSI_SPACE_FILES:
	    *dmaCountPtr = 0;
	    *commandPtr = SCSI_SPACE;
	    controlBlockPtr->code = 1;
	    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_SPACE_BLOCKS:
	    *dmaCountPtr = 0;
	    *commandPtr = SCSI_SPACE;
	    controlBlockPtr->code = 0;
	    break;
	case SCSI_SPACE_EOT:
	    *dmaCountPtr = 0;
	    *commandPtr = SCSI_SPACE;
	    controlBlockPtr->code = 3;
	    tapePtr->state |= SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_ERASE_TAPE:
	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SysgenStatus --
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
SysgenStatus(devPtr, statusPtr)
    DevSCSIDevice *devPtr;
    Dev_TapeStatus *statusPtr;
{
    unsigned char *senseBytes;

    statusPtr->type = DEV_TAPE_SYSGEN;
    /*
     * Return the first two sense bytes from the sysgen controller.
     * This has the standard QIC 02 bits, see the typedef
     * for DevQICIISense.  In the high part of the error word
     * return the standard SCSI error byte that contains the
     * error class and code.
     */
    senseBytes = devPtr->scsiPtr->senseBuffer->sense;
    statusPtr->errorReg = ((senseBytes[1] & 0xFF) << 8) |
			    (senseBytes[0] & 0xFF) |
			   (devPtr->scsiPtr->senseBuffer->error << 24);
}

/*
 *----------------------------------------------------------------------
 *
 * SysgenError --
 *
 *	Handle error conditions from a Sysgen based tape drive.
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
SysgenError(devPtr, sensePtr)
    DevSCSIDevice *devPtr;
    DevSCSISense *sensePtr;
{
    register ReturnStatus status = SUCCESS;
    DevSCSITape *tapePtr = (DevSCSITape *)devPtr->data;
    register DevQICIISense *qicSensePtr = (DevQICIISense *)sensePtr;

    if (qicSensePtr->noCartridge) {
	status = DEV_OFFLINE;
    } else if (qicSensePtr->noDrive) {
	status = DEV_NO_DEVICE;
    } else if (qicSensePtr->dataError || qicSensePtr->retries) {
	status = DEV_HARD_ERROR;
    } else if (qicSensePtr->endOfTape) {
	status = DEV_END_OF_TAPE;
    } else {
	switch (devPtr->scsiPtr->command) {
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
    return(status);
}
