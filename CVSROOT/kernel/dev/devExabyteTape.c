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


#include <sprite.h>
#include <stdio.h>
#include <dev.h>
#include <sys/scsi.h>
#include <scsiTape.h>
#include <scsiDevice.h>
#include <stdlib.h>
#include <fs.h>
#include <exabyteTape.h>
#include <string.h>
#include <dev/exabyte.h>

/*
 * The Exabyte drives have 1K blocks.
 */
#define EXABYTE_BLOCK_SIZE	1024


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


/*
 * Parameter list header returned by mode sense.
 */

typedef struct {
#if BYTE_ORDER == BIG_ENDIAN
    unsigned char modeDataLen;		/* Size of mode parameter data. */
    unsigned char mediumType;		/* Type of medium. */
    unsigned char writeProtect  :1; 	/* Write protect. */
    unsigned char bufferedMode  :3;   	/* Buffered mode. */
    unsigned char speed		:4;	/* Speed. */
    unsigned char blockDescLen;		/* Block descriptor length. */
#else
    unsigned char modeDataLen;		/* Size of mode parameter data. */
    unsigned char mediumType;		/* Type of medium. */
    unsigned char speed		:4;	/* Speed. */
    unsigned char bufferedMode  :3;   	/* Buffered mode. */
    unsigned char writeProtect  :1; 	/* Write protect. */
    unsigned char blockDescLen;		/* Block descriptor length. */
#endif
} ParamListHeader;

/*
 * Block descriptor returned by mode sense.
 */

typedef struct {
    unsigned char	density;	/* Density code. */
    unsigned char	num2;		/* MSB of number of blocks. */
    unsigned char	num1;		/* ... */
    unsigned char	num0;		/* LSB of number of blocks. */
    unsigned char	pad0;		/* Reserved. */
    unsigned char	len2;		/* MSB of block length. */
    unsigned char	len1;		/* ... */
    unsigned char	len0;		/* LSB of block length. */
} BlockDesc;

/*
 * Exabyte 8500 Inquiry data.
 */

typedef struct {
    ScsiInquiryData	stdData;
    char		padding[52];
    char		serial[10];
} Exb8500Inquiry;

static ReturnStatus ExabyteError _ARGS_((ScsiDevice *devPtr,
	ScsiCmd *scsiCmdPtr));
ReturnStatus DevExabyteStatus _ARGS_((ScsiTape *tapePtr, 
	Dev_TapeStatus *statusPtr, Boolean *readPositionPtr));


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
    devPtr->errorProc = ExabyteError;
    if (!(strncmp((char *) inquiryPtr->productID, "EXB-8200",8))) {
	tapePtr->name = "Exabyte 8200";
	tapePtr->type = DEV_TAPE_EXB8200;
    } else if (!(strncmp((char *) inquiryPtr->productID, "EXB-8500",8))) {
	tapePtr->name = "Exabyte 8500";
	tapePtr->type = DEV_TAPE_EXB8500;
    } else {
	tapePtr->name = "Exabyte UNKNOWN";
    }
    tapePtr->statusProc = DevExabyteStatus;
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
ExabyteError(devPtr, scsiCmdPtr)
    ScsiDevice	 *devPtr;	/* SCSI device that's complaining. */
    ScsiCmd	*scsiCmdPtr;	/* SCSI command that had the problem. */
{
    ReturnStatus status;
    register volatile Exb8200Sense *exabyteSensePtr =
        (volatile Exb8200Sense *) scsiCmdPtr->senseBuffer;
    Exb8500Sense *newSensePtr;

    status = DevSCSITapeError(devPtr, scsiCmdPtr);
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
    printf("Additional Sense Code: 0x%x\n", exabyteSensePtr->senseCode);
    printf("Additional Sense Code Qualifier: 0x%x\n", 
	exabyteSensePtr->senseCodeQualifier);
    if (scsiCmdPtr->senseLen == sizeof(Exb8500Sense)) {
	newSensePtr = (Exb8500Sense *) exabyteSensePtr;
	printf("EXB8500 Fault Symptom Code = 0x%x\n", newSensePtr->faultCode);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevExabyteStatus --
 *
 *	Fill in some of the fields in the status structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Some scsi commands are sent to the device.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevExabyteStatus(tapePtr, statusPtr, readPositionPtr)
    ScsiTape	 	*tapePtr;	/* SCSI Tape */
    Dev_TapeStatus	*statusPtr;	/* Status to fill in. */
    Boolean		*readPositionPtr; /* OUT: should read position cmd
					   * done? */
{
    ReturnStatus	status = SUCCESS;
    int			size;
    union {
	Exb8200Sense	sense8200;
	Exb8500Sense	sense8500;
    } sense;

    size = sizeof(sense);
    bzero((char *) &sense, size);
    status = DevScsiRequestSense(tapePtr->devPtr, 0, 0, &size, 
	    (char *) &sense);
    if (status != SUCCESS) {
	goto done;
    }
    if (tapePtr->type == DEV_TAPE_EXB8200) {
	Exb8200Sense	*sensePtr;
	int		bufSize;
	ParamListHeader	header;
	BlockDesc	desc;
	char		buffer[16];
	if (size != sizeof(Exb8200Sense)) {
	    status = FAILURE;
	    goto done;
	}
	sensePtr = &sense.sense8200;
	statusPtr->remaining = 
		((unsigned int) sensePtr->highRemainingTape << 16) |
		((unsigned int) sensePtr->midRemainingTape << 8) |
		(sensePtr->lowRemainingTape);
	statusPtr->dataError = 
		((unsigned int) sensePtr->highErrorCnt << 16) |
		((unsigned int) sensePtr->midErrorCnt << 8) |
		(sensePtr->lowErrorCnt);
	bufSize = sizeof(buffer);
	status = DevScsiModeSense(tapePtr->devPtr, 0, 0, 0, 0, &bufSize, 
			buffer);
	if (status != SUCCESS) {
	    goto done;
	}
	bcopy((char *) buffer, (char *) &header, sizeof(header));
	bcopy((char *) buffer + sizeof(header), (char *) &desc, sizeof(desc));
	statusPtr->writeProtect = (header.writeProtect) ? TRUE : FALSE;
	statusPtr->bufferedMode = header.bufferedMode;
	statusPtr->serial[0] = '\0';
	*readPositionPtr = FALSE;
    } else if (tapePtr->type == DEV_TAPE_EXB8500) {
	Exb8500Sense	*sensePtr;
	int		bufSize;
	ParamListHeader	header;
	BlockDesc	desc;
	Exb8500Inquiry	*inqPtr;
	char		buffer[sizeof(ParamListHeader) + sizeof(BlockDesc)];
	if (size != sizeof(Exb8500Sense)) {
	    status = FAILURE;
	    goto done;
	}
	sensePtr = &sense.sense8500;
	statusPtr->remaining = 
		((unsigned int) sensePtr->highRemainingTape << 16) |
		((unsigned int) sensePtr->midRemainingTape << 8) |
		(sensePtr->lowRemainingTape);
	statusPtr->dataError = 
		((unsigned int) sensePtr->highErrorCnt << 16) |
		((unsigned int) sensePtr->midErrorCnt << 8) |
		(sensePtr->lowErrorCnt);
	statusPtr->readWriteRetry = sensePtr->readWriteRetry;
	statusPtr->trackingRetry = sensePtr->trackingRetry;
	bufSize = sizeof(buffer);
	status = DevScsiModeSense(tapePtr->devPtr, 0, 0, 0, 0, &bufSize, 
			buffer);
	if (status != SUCCESS) {
	    goto done;
	}
	bcopy((char *) buffer, (char *) &header, sizeof(header));
	bcopy((char *) buffer + sizeof(header), (char *) &desc, sizeof(desc));
	statusPtr->writeProtect = (header.writeProtect) ? TRUE : FALSE;
	statusPtr->bufferedMode = header.bufferedMode;
	statusPtr->speed = header.speed;
	statusPtr->density = desc.density;
	inqPtr = (Exb8500Inquiry *) tapePtr->devPtr->inquiryDataPtr;
	bcopy(inqPtr->serial, statusPtr->serial, sizeof(statusPtr->serial));
	statusPtr->serial[10] = '\0';
	*readPositionPtr = TRUE;
    }
done:
    return status;
}
