/* 
 * devSCSIExabyte.c --
 *
 *      Procedures that set up command blocks and process sense
 *	data for Exabyte tape drives.
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
#include "devSCSIExabyte.h"

void ExabyteSetup();
void ExabyteStatus();
ReturnStatus ExabyteError();


/*
 *----------------------------------------------------------------------
 *
 * DevExabyteInit --
 *
 *	Initialize the DevSCSITape state for a Exabyte drive.
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
DevExabyteInit(tapePtr)
    DevSCSITape	*tapePtr;	/* Tape drive state */
{
    tapePtr->type = SCSI_EXABYTE;
    tapePtr->blockSize = DEV_EXABYTE_BLOCK_SIZE;
    tapePtr->setupProc = ExabyteSetup;
    tapePtr->statusProc = ExabyteStatus;
    tapePtr->errorProc = ExabyteError;
}

/*
 *----------------------------------------------------------------------
 *
 * ExabyteSetup --
 *
 *	This customizes the control block and sets the count and dmaCount
 *	to be correct for Exabyte based tape drives.
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
ExabyteSetup(tapePtr, commandPtr, controlBlockPtr, countPtr, dmaCountPtr)
    DevSCSITape	*tapePtr;	/* Tape drive state */
    int *commandPtr;		/* In/Out tape command */
    DevSCSITapeControlBlock *controlBlockPtr;	/* CMD Block to set up */
    int *countPtr;		/* In - Transfer count in bytes.
				 * Out - The proper byte count for CMD block */
    int *dmaCountPtr;		/* In - Transfer count in bytes.
				 * Out - The proper DMA byte count for caller */
{
    switch (*commandPtr) {
	case SCSI_TEST_UNIT_READY:
	    break;
	case SCSI_REWIND:
	    /*
	     * Note that it is possible to get status back immediately
	     * by setting  code = 1, but it is probably cleaner to
	     * rely on connect/dis-connect.
	     */
	    if (tapePtr->state & SCSI_TAPE_RETENSION) {
		tapePtr->state &= ~SCSI_TAPE_RETENSION;
		/* No notion of retentioning the Exabyte tape */
	    }
	    tapePtr->state &= ~SCSI_TAPE_AT_EOF;
	    break;
	case SCSI_REQUEST_SENSE:
	    *dmaCountPtr = *countPtr = sizeof(DevExabyteSense);
	    break;
	case SCSI_READ:
	case SCSI_WRITE:
	    /*
	     * The command block takes a block count.  The code value
	     * of 1 indicates fixed size blocks.  FIX HERE to handle
	     * transfers smaller than 1K.
	     */
	    controlBlockPtr->code = 1;
	    *countPtr /= tapePtr->blockSize;
	    break;
	case SCSI_WRITE_EOF:
	    /*
	     * Note that bit vendor57 can be used to write "short" filemarks,
	     * which take up less tape but are not eraseable.  You can
	     * write another file after them, but not append to the
	     * existing file.
	     */
	    *dmaCountPtr = 0;
	    *countPtr = 1;
	    break;
	case SCSI_SPACE_EOT:
	    printf("Exabyte does not support SCSI_SPACE_EOT, skipping to EOF instead\n");
	    /* Fall Through */;
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
	case SCSI_ERASE_TAPE:
	    controlBlockPtr->code = 1;
	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ExabyteStatus --
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
ExabyteStatus(devPtr, statusPtr)
    DevSCSIDevice *devPtr;
    Dev_TapeStatus *statusPtr;
{
    unsigned char *senseBytes;

    statusPtr->type = DEV_TAPE_EXABYTE;
    /*
     * Return byte2 from the class 7 extended sense plus
     * Byte19 and Byte20 of the Exabyte Sense.  Byte2 of Extended Sense
     * has a fileMark bit, endOfMedia bit, badBlockLen bit, a reserved bit,
     * and a 4 bit sense key.  The 19'th and 20'th bytes have 8 bits
     * defined in devSCSIExabyte.h
     */
    senseBytes = (unsigned char *)devPtr->scsiPtr->senseBuffer;

    statusPtr->errorReg = senseBytes[2] |
			 (senseBytes[19] << 16) |
			 (senseBytes[20] << 24);
}

/*
 *----------------------------------------------------------------------
 *
 * ExabyteError --
 *
 *	Handle error conditions from a Exabyte based tape drive.
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
ExabyteError(devPtr, sensePtr)
    DevSCSIDevice *devPtr;
    DevSCSISense *sensePtr;
{
    register ReturnStatus status;
    DevSCSITape *tapePtr = (DevSCSITape *)devPtr->data;
    register DevExabyteSense *exabyteSensePtr = (DevExabyteSense *)sensePtr;

    status = SUCCESS;
    if (exabyteSensePtr->extSense.endOfMedia) {
	status = DEV_END_OF_TAPE;
    }
    if (exabyteSensePtr->extSense.badBlockLen) {
	printf("Exabyte Block Length Mismatch\n");
	status = DEV_HARD_ERROR;
    }
    if (exabyteSensePtr->extSense.fileMark) {
	/*
	 * Hit the file mark after reading good data. Setting this bit causes
	 * the next read to return zero bytes.
	 */
	tapePtr->state |= SCSI_TAPE_AT_EOF;
    }
    /*
     * The sense key indicates the most serious error.
     */
    switch(exabyteSensePtr->extSense.key) {
	case SCSI_NO_SENSE:
	    return(status);
	case SCSI_NOT_READY_KEY:
	    printf("Exabyte cartridge not loaded?\n");
	    status = DEV_OFFLINE;
	    break;
	case SCSI_MEDIA_ERROR:
	    printf("Exabyte media error\n");
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_HARDWARE_ERROR:
	    printf("Exabyte hardware error\n");
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_ILLEGAL_REQUEST:
	    printf("Exabyte illegal request\n");
	    status = GEN_INVALID_ARG;
	    break;
	case SCSI_UNIT_ATTN_KEY:
	    /* no big deal, usually */
	    status = SUCCESS;
	    break;
	case SCSI_WRITE_PROTECT_KEY:
	    printf("Exabyte write protected\n");
	    status = FS_NO_ACCESS;
	    break;
	case SCSI_BLANK_CHECK:
	    printf("Exabyte hit blank tape\n");
	    status = DEV_END_OF_TAPE;
	    break;
	case SCSI_VENDOR:
	    if (exabyteSensePtr->TMD) {
		printf("Exabyte Tape Mark Detect error\n");
		status = DEV_HARD_ERROR;
	    } else if (exabyteSensePtr->XFR) {
		printf("Exabyte Transfer abort error\n");
		status = DEV_HARD_ERROR;
	    }
	    break;
	case SCSI_ABORT_KEY:
	    printf("Exabyte aborted command\n");
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_OVERFLOW:
	    printf("Exabyte overflowed physical media\n");
	    status = DEV_END_OF_TAPE;
	    break;
	default:
	    printf("Exabyte: Unsupported sense key <%x>, HARD ERROR\n",
		exabyteSensePtr->extSense.key);
	    status = DEV_HARD_ERROR;
	    break;
    }
    /*
     * There are 8 bits that indicate the nature of the error more precisely.
     */
    if (exabyteSensePtr->PF) {
	/* Media changed or after power up */
    }
    if (exabyteSensePtr->BPE) {
	printf("Exabyte SCSI Bus Parity error\n");
    }
    if (exabyteSensePtr->FPE) {
	printf("Exabyte Formatter Parity error\n");
    }
    if (exabyteSensePtr->ME) {
	/* Media Error already reported via sense key */
    }
    if (exabyteSensePtr->ECO) {
	printf("Exabyte error counter overflow\n");
    }
    if (exabyteSensePtr->TME) {
	printf("Exabyte Tape Motion error\n");
    }
    if (exabyteSensePtr->TNP) {
	printf("Exabyte tape not present\n");
    }
    if (exabyteSensePtr->BOT) {
	/* At the beginning of tape */
    }
    if (exabyteSensePtr->FMKE) {
	printf("Exabyte File Mark Error\n");
    }
    if (exabyteSensePtr->URE) {
	printf("Exabyte Data Flow Underrun\n");
    }
    if (exabyteSensePtr->WE1) {
	printf("Exabyte maximum write retries attempted\n");
    }
    if (exabyteSensePtr->SSE) {
	printf("Exabyte Servo System error, catastrophic failure!\n");
    }
    if (exabyteSensePtr->FE) {
	printf("Exabyte Formatter error, catastrophic failure!\n");
    }
    return(status);
}
