/* 
 * devExabyteTape.c --
 *
 *      Procedures that set up command blocks and process sense
 *	data for Exabyte tape drives.
 * Definitions for sense data format and status information returned
 * from Exabyte tape drives.  Reference, the "EXB-8200 8mm Tape Drive
 * User's Guide" by Perfect Byte, Inc. 7121 Cass St. Omaha, NE 68132
 *
 * Copyright 1989 Regents of the University of California
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
#include "stdio.h"
#include "dev.h"
#include "scsi.h"
#include "scsiTape.h"
#include "scsiDevice.h"
#include "stdlib.h"
#include "fs.h"
#include "exabyteTape.h"
#include "string.h"

/*
 * The Exabyte drives have 1K blocks.
 */
#define EXABYTE_BLOCK_SIZE	1024

/*
 * Sense data returned from the Exabyte tape controller.
 */
#define EXABYTE_SENSE_BYTES	26
typedef struct {
    ScsiClass7Sense	extSense;	/* 8 Bytes */
    unsigned char pad8;			/* Reserved */
    unsigned char pad;			/* Reserved */
    unsigned char pad10;		/* Reserved */
    unsigned char pad11;		/* Reserved */
    /*
     * SCSI 2 support.
     */
    unsigned char senseCode;		/* 0x4 if sense key is NOT_READY */
    unsigned char senseCodeQualifier;	/* 00 - volume not mounted.
					 * 01 - rewinding or loading */
    unsigned char pad14;		/* Reserved */
    unsigned char pad15;		/* Reserved */
    unsigned char highErrorCnt;		/* High byte of error count */
    unsigned char midErrorCnt;		/* Middle byte of error count */
    unsigned char lowErrorCnt;		/* Low byte of error count */
    /*
     * Error bits that are command dependent.  0 is ok, 1 means error.
     * These are defined on pages 37-38 of the User Manual, Rev.03
     */
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char PF		:1;	/* Power failure */
    unsigned char BPE		:1;	/* SCSI Bus Parity Error */
    unsigned char FPE		:1;	/* Formatted buffer parity error */
    unsigned char ME		:1;	/* Media error */
    unsigned char ECO		:1;	/* Error counter overflow */
    unsigned char TME		:1;	/* Tape motion error */
    unsigned char TNP		:1;	/* Tape not present */
    unsigned char BOT		:1;	/* Set when tape is at BOT */

    unsigned char XFR		:1;	/* Transfer Abort Error */
    unsigned char TMD		:1;	/* Tape Mark Detect Error */
    unsigned char WP		:1;	/* Write Protect */
    unsigned char FMKE		:1;	/* File Mark Error */
    unsigned char URE		:1;	/* Data flow underrun. Media error. */
    unsigned char WE1		:1;	/* Max write retries attempted */
    unsigned char SSE		:1;	/* Servo System error.  Catastrophic */
    unsigned char FE		:1;	/* Formatter error.  Catastrophic */

    unsigned char pad21		:6;	/* Reserved */
    unsigned char WSEB		:1;	/* Write Splice Error, hit blank tape */
    unsigned char WSEO		:1;	/* Write Splice Error, overshoot */
#else /* BYTE_ORDER == LITTLE_ENDIAN */

    unsigned char BOT		:1;	/* Set when tape is at BOT */
    unsigned char TNP		:1;	/* Tape not present */
    unsigned char TME		:1;	/* Tape motion error */
    unsigned char ECO		:1;	/* Error counter overflow */
    unsigned char ME		:1;	/* Media error */
    unsigned char FPE		:1;	/* Formatted buffer parity error */
    unsigned char BPE		:1;	/* SCSI Bus Parity Error */
    unsigned char PF		:1;	/* Power failure */

    unsigned char FE		:1;	/* Formatter error.  Catastrophic */
    unsigned char SSE		:1;	/* Servo System error.  Catastrophic */
    unsigned char WE1		:1;	/* Max write retries attempted */
    unsigned char URE		:1;	/* Data flow underrun. Media error. */
    unsigned char FMKE		:1;	/* File Mark Error */
    unsigned char WP		:1;	/* Write Protect */
    unsigned char TMD		:1;	/* Tape Mark Detect Error */
    unsigned char XFR		:1;	/* Transfer Abort Error */

    unsigned char WSEO		:1;	/* Write Splice Error, overshoot */
    unsigned char WSEB		:1;	/* Write Splice Error, hit blank tape */
    unsigned char pad21		:6;	/* Reserved */

#endif /* BYTE_ORDER */

    unsigned char pad22;		/* Reserved */
    unsigned char highRemainingTape;	/* High byte of remaining tape len */
    unsigned char midRemainingTape;	/* Middle byte of remaining tape len */
    unsigned char lowRemainingTape;	/* Low byte of remaining tape len */

} ExabyteSense;				/* Known to be 26 Bytes big (for
					 * Drives made in/after 1988) */


/*
 * Definitions for the mode select command.  The MODE_SELECT data
 * consists of a 4 byte header, zero or one 8 byte block descriptors,
 * and finally from zero to 4 bytes of Vendor Unique Parameters.
 * For simplicity we'll always send 1 block descriptor and 4 parameter bytes.
 */

typedef struct ExabyteModeSelBlock {
    unsigned char density;		/* Density code == 0.  Only one dens. */
    unsigned char highCount;		/* == 0 */
    unsigned char midCount;		/* == 0 */
    unsigned char lowCount;		/* == 0 */
    unsigned char pad1;			/* Reserved */
    unsigned char highLength;		/* Length of the blocks on tape */
    unsigned char midLength;		/*	0 means variable length */
    unsigned char lowLength;		/*	Default is 1024 bytes */
} ExabyteModeSelBlock;		/* 8 Bytes */


typedef struct ExabyteModeSelParams {
    ScsiTapeModeSelectHdr	header;
    ExabyteModeSelBlock	block;
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char cartidgeType	:1;	/* 1 == p5 European.
					 * 0 == P6 Domestic */
    unsigned char		:3;	/* Reserved */
    unsigned char noBusyEnable	:1;	/* 0 == Report Busy Status (default)
					 * 1 == No Busy Enable, cmd queued */
    unsigned char evenByteDscnct :1;	/* 0 == Even or Odd byte disconnect
					 * 1 == Even Byte disconnect */
    unsigned char parityEnable	:1;	/* 0 == Parity disabled (default) */
    unsigned char noAutoLoad	:1;	/* 0 == Auto load enabled (default) */
#else /* BYTE_ORDER == LITTLE_ENDIAN */

    unsigned char noAutoLoad	:1;	/* 0 == Auto load enabled (default) */
    unsigned char parityEnable	:1;	/* 0 == Parity disabled (default) */

    unsigned char evenByteDscnct :1;	/* 0 == Even or Odd byte disconnect
					 * 1 == Even Byte disconnect */
    unsigned char noBusyEnable	:1;	/* 0 == Report Busy Status (default)
					 * 1 == No Busy Enable, cmd queued */

    unsigned char		:3;	/* Reserved */
    unsigned char cartidgeType	:1;	/* 1 == p5 European.
					 * 0 == P6 Domestic */

#endif /* BYTE_ORDER */
    unsigned char pad1;			/* RESERVED */
    /*
     * The Motion threashold must exceed the Reconnect threshold.
     * Values represent 1K byte increments.
     * Motion - default 0xF0, valid range 0x01 -> 0xF7
     * Reconnect - default 0x40, valid range 0x01 to 0xF7
     * WRITE - lower motion threshold for faster transfer.
     * READ - raise reconnect threshold for faster transfer.
     *	Basically these control the amount of data kept in the buffer
     *	and hence the latency.
     */
    unsigned char motion;		/* Defines how many Kbytes are buffered
					 * before writing to the tape begins,
					 * or when reconnecting on a read */
    unsigned char reconnect;		/* Defines how many Kbytes are left
					 * in the buffer when the drive
					 * begins filling it again, either
					 * by reading the tape or reconnecting
					 * and getting more data from the 
					 * SCSI bus. */
} ModeSelParams;

static ReturnStatus ExabyteError _ARGS_((ScsiTape *tapePtr, 
	unsigned int statusByte, int senseLength, char *senseDataPtr));


/*
 *----------------------------------------------------------------------
 *
 * DevExabyteAttach --
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
/*ARGSUSED*/
ReturnStatus
DevExabyteAttach(devicePtr, devPtr, tapePtr)
    Fs_Device	*devicePtr;	/* Fs_Device being attached. */
    ScsiDevice	*devPtr;	/* SCSI device handle for drive. */
    ScsiTape	*tapePtr;	/* Tape drive state to be filled in. */
{
    ScsiInquiryData	*inquiryPtr;
    /*
     * First we must verify that the attached device is a Exabyte. We do
     * that by examinging the Inquiry data in the ScsiDevice handle. The
     * lack of Inquiry data would imply its not a Exabyte. 
     */
    inquiryPtr = (ScsiInquiryData *) (devPtr->inquiryDataPtr);
    if ( (devPtr->inquiryLength < sizeof(ScsiInquiryData)) ||
	 (strncmp((char *) (inquiryPtr->vendorID), "EXABYTE ",8) != 0) ) {
	 return DEV_NO_DEVICE;
    }
    /*
     * The exabyte has a different BLOCK and we liked to check the
     * vendor unique bits on error.
     */
    tapePtr->blockSize = EXABYTE_BLOCK_SIZE;
    tapePtr->errorProc = ExabyteError;
    if (!(strncmp((char *)(inquiryPtr->productID), "EXB-8200",8))) {
	tapePtr->name = "Exabyte 8200";
    } else if (!(strncmp((char *)(inquiryPtr->productID), "EXB-8500",8))) {
	tapePtr->name = "Exabyte 8500";
    } else {
	tapePtr->name = "Exabyte UNKNOWN";
    }
    return SUCCESS;
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
static ReturnStatus
ExabyteError(tapePtr, statusByte, senseLength, senseDataPtr)
    ScsiTape	 *tapePtr;	/* SCSI Tape that's complaining. */
    unsigned int statusByte;	/* The status byte of the command. */
    int		 senseLength;	/* Length of SCSI sense data in bytes. */
    char	 *senseDataPtr;	/* Sense data. */
{
    ReturnStatus status;
    register volatile ExabyteSense *exabyteSensePtr =
        (volatile ExabyteSense *)senseDataPtr;

    status = DevSCSITapeError(tapePtr, statusByte, senseLength, senseDataPtr);
    if (status == SUCCESS) {
	return status;
    }
    if (exabyteSensePtr->extSense.badBlockLen) {
	printf("Exabyte Block Length Mismatch\n");
	status = DEV_HARD_ERROR;
    }
    if (exabyteSensePtr->TMD) {
	    printf("Warning: Exabyte Tape Mark Detect error\n");
	    status = DEV_HARD_ERROR;
    } 
    if (exabyteSensePtr->XFR) {
	    printf("Warning: Exabyte Transfer abort error\n");
	    status = DEV_HARD_ERROR;
    }
    if (exabyteSensePtr->PF) {
	/* Media changed or after power up */
    }
    if (exabyteSensePtr->BPE) {
	printf("Warning: Exabyte SCSI Bus Parity error\n");
    }
    if (exabyteSensePtr->FPE) {
	printf("Warning: Exabyte Formatter Parity error\n");
    }
    if (exabyteSensePtr->ME) {
	/* Media Error already reported via sense key */
    }
    if (exabyteSensePtr->ECO) {
	printf("Warning: Exabyte error counter overflow\n");
    }
    if (exabyteSensePtr->TME) {
	printf("Warning: Exabyte Tape Motion error\n");
    }
    if (exabyteSensePtr->TNP) {
	printf("Warning: Exabyte tape not present\n");
    }
    if (exabyteSensePtr->BOT) {
	/* At the beginning of tape */
    }
    if (exabyteSensePtr->FMKE) {
	printf("Exabyte File Mark Error\n");
    }
    if (exabyteSensePtr->URE) {
	printf("Warning: Exabyte Data Flow Underrun\n");
    }
    if (exabyteSensePtr->WE1) {
	printf("Warning: Exabyte maximum write retries attempted\n");
    }
    if (exabyteSensePtr->SSE) {
	printf("Warning: Exabyte Servo System error, catastrophic failure!\n");
    }
    if (exabyteSensePtr->FE) {
	printf("Warning: Exabyte Formatter error, catastrophic failure!\n");
    }
    return(status);
}
