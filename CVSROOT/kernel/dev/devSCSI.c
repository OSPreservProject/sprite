/* 
 * devSCSI.c --
 *
 *	SCSI = Small Computer System Interface. The routines in this file
 *	are indented to aid in formatting SCSI command blocks.
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
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "scsiDevice.h"
#include "dbg.h"
#include "vm.h"
#include "sys.h"
#include "sync.h"
#include "fs.h"
#include "sched.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bstring.h"

/*
 * The error codes for class 0-6 sense data are class specific.
 * The follow arrays of strings are used to print error messages.
 */
static char *Class0Errors[] = {
    "No sense data",
    "No index signal",
    "No seek complete",
    "Write fault",
    "Drive notready",
    "Drive not selected",
    "No Track 00",
    "Multiple drives selected",
    "No address acknowledged",
    "Media not loaded",
    "Insufficient capacity",
};
static char *Class1Errors[] = {
    "ID CRC error",
    "Unrecoverable data error",
    "ID address mark not found",
    "Data address mark not found",
    "Record not found",
    "Seek error",
    "DMA timeout error",
    "Write protected",
    "Correctable data check",
    "Bad block found",
    "Interleave error",
    "Data transfer incomplete",
    "Unformatted or bad format on drive",
    "Self test failed",
    "Defective track (media errors)",
};
static char *Class2Errors[] = {
    "Invalid command",
    "Illegal block address",
    "Aborted",
    "Volume overflow",
};
int devScsiNumErrors[] = {
    sizeof(Class0Errors) / sizeof (char *),
    sizeof(Class1Errors) / sizeof (char *),
    sizeof(Class2Errors) / sizeof (char *),
    0, 0, 0, 0, 0,
};
char **devScsiErrors[] = {
    Class0Errors,
    Class1Errors,
    Class2Errors,
};

int devSCSIDebug = FALSE;

/*
 *----------------------------------------------------------------------
 *
 * DevScsiMapSense --
 *
 *	Map a SCSI Class7 Sense data structure into a Sprite ReturnStatus
 *	and a printable error string.
 *
 * Results:
 *	TRUE if the mapping succeeded. FALSE if the argument is not 
 *	Class7 sense data.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevScsiMapClass7Sense(senseLength,senseDataPtr,statusPtr,errorString)
    int		senseLength;	/* Length of the sense data at senseDataPtr. */
    char	*senseDataPtr;	/* The sense data. */
    ReturnStatus *statusPtr;	/* OUT - The Sprite ReturnStatus. */
    char	*errorString;	/* OUT - A buffer to write a printable string
				 * describing the data. Must be at least 
				 * MAX_SCSI_ERROR_STRING in length. */
{
    register ScsiClass7Sense	*sensePtr = (ScsiClass7Sense *) senseDataPtr;
    ReturnStatus	status;

    /*
     * Default to no error string. 
     */
    *errorString = 0;

    if (senseLength < sizeof(ScsiClass7Sense)) {
	return (FALSE);
    }
    if (sensePtr->error7 != 0x70) {
	return (FALSE);
    }

    switch (sensePtr->key) {
	case SCSI_CLASS7_NO_SENSE:
	    status = SUCCESS;
	    break;
	case SCSI_CLASS7_RECOVERABLE:
	    /*
	     * The drive recovered from an error.
	     */
	     sprintf(errorString,
		    "recoverable error - info bytes 0x%x 0x%x 0x%x 0x%x",
		     sensePtr->info1, sensePtr->info2,
		     sensePtr->info3,sensePtr->info4);
	    status = SUCCESS;
	    break;
	case SCSI_CLASS7_NOT_READY:
	    status = DEV_OFFLINE;
	    break;
	case SCSI_CLASS7_MEDIA_ERROR:
	case SCSI_CLASS7_HARDWARE_ERROR:
	     sprintf(errorString, "%s error - info bytes 0x%x 0x%x 0x%x 0x%x",
		(sensePtr->key == SCSI_CLASS7_MEDIA_ERROR) ? "media" :
							     "hardware",
		sensePtr->info1 & 0xff,
		sensePtr->info2 & 0xff,
		sensePtr->info3 & 0xff,
		sensePtr->info4 & 0xff);
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_CLASS7_ILLEGAL_REQUEST:
	    /*
	     * Probably a programming error.
	     */
	    sprintf(errorString,"illegal request");
	    status = DEV_INVALID_ARG;
	    break;
	case SCSI_CLASS7_UNIT_ATTN:
	    /*
	     * This is an error that occurs after the drive is reset.
	     * It can probably be ignored.
	     */
	    status = SUCCESS;
	    break;
	case SCSI_CLASS7_WRITE_PROTECT:
	    sprintf(errorString,"write protected");
	    status = FS_NO_ACCESS;
	    break;
	case SCSI_CLASS7_BLANK_CHECK:
	    sprintf(errorString,"blank check - info bytes  0x%x 0x%x 0x%x 0x%x",
		sensePtr->info1,
		sensePtr->info2,
		sensePtr->info3,
		sensePtr->info4);
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_CLASS7_VENDOR:
	case SCSI_CLASS7_ABORT:
	case SCSI_CLASS7_EQUAL:
	case SCSI_CLASS7_OVERFLOW:
	    sprintf(errorString,"unsupported class7 error 0x%x\n",
		    sensePtr->key);
	    status = DEV_HARD_ERROR;
	    break;
	default: {
	    sprintf(errorString,"unknown class7 error 0x%x\n", sensePtr->key);
	    status = DEV_HARD_ERROR;
	    break;
	}
    }
    *statusPtr = status;
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * DevScsiGroup0Cmd --
 *
 *      Setup a ScsiCmd block for a SCSI Group0 command.
 *
 * Results:
 *	SUCCESS if the command block was filled in correctly, FAILURE
 *	otherwise
 *
 * Side effects:
 *	Set the various fields in the control block.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevScsiGroup0Cmd(devPtr, cmd, blockNumber,countNumber,scsiCmdPtr)
    ScsiDevice	*devPtr; /* SCSI device target for this command. */
    int		cmd;	 /* Group0 scsi command. */
    unsigned int blockNumber;	/* The starting block number for the transfer */
    unsigned int countNumber;	/* Number of sectors (or bytes!) to transfer */
    register ScsiCmd	*scsiCmdPtr; /* Scsi command block to be filled in. */
{
    register  ScsiGroup0Cmd	*c;

    if ((cmd < 0) || (cmd > 0x1f)) {
	panic("Bad SCSI command 0x%x giving to DevScsiGroup0Cmd.\n",cmd);
    }
    if (blockNumber > 0x1fffff) {
	printf("DevScsiGroup0Cmd: block number too big (%d > %d)\n",
	    blockNumber, 0x1fffff);
	return FAILURE;
    }
    if (countNumber > 0xff) {
	printf("DevScsiGroup0Cmd: count too big (%d > %d)\n",
	    countNumber, 0xff);
	return FAILURE;
    }
    bzero((char *)scsiCmdPtr, sizeof(ScsiCmd));
    scsiCmdPtr->commandBlockLen = sizeof(ScsiGroup0Cmd);
    c = (ScsiGroup0Cmd *) scsiCmdPtr->commandBlock;
    c->command = cmd;
    c->unitNumber = devPtr->LUN;
    c->highAddr = (blockNumber & 0x1f0000) >> 16;
    c->midAddr =  (blockNumber & 0x00ff00) >> 8;
    c->lowAddr =  (blockNumber & 0x0000ff);
    c->blockCount =  countNumber;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyAndTerminateString --
 *
 *	Copy a fixed length string into a null terminate string stripping
 *	off the trailing blanks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CopyAndTerminateString(length, string, outString)
    int		length;	/* Length of the input string argument string. */
    char	*string; /* Input string. */
    char	*outString; /* Output string area. Must be at least (length+1) 
			     * bytes. */
{
    /*
     * Find last non blank charater in string. Update length of string.
     */
    while ( (length > 0) && (string[length] == ' ') ) {
	length--;
    }
    /*
     * Copy the string, terminate, and return.
     */
    bcopy(string, outString, length);
    outString[length] = 0;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * DevScsiFormatInquiry --
 *
 *	Format SCSI inquiry data into a ascii string suitable for printing.
 *
 * Results:
 *	The string length.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
DevScsiFormatInquiry(dataPtr, outputString)
    ScsiInquiryData *dataPtr;		/* Data structure returned by the 
					 * SCSI inquiry command. 
					 */
    char	*outputString;		/* String to format into. */
{
    static char *deviceTypeNames[] = {
	"Disk", "Tape", "Printer", "Processor", "WORM", "ROM",
    };

    if (dataPtr->type > (sizeof(deviceTypeNames)/sizeof(char *))) {
	/*
	 * If the device type is SCSI_NODEVICE_TYPE the rest of the
	 * data is not really meaningful so we return.
	 */
	if (dataPtr->type == SCSI_NODEVICE_TYPE) {
	    (void) sprintf(outputString,"Logical unit not present");
	    return strlen(outputString);
	} else {
	    (void) sprintf(outputString,"Unknown 0x%x",dataPtr->type);
	    return strlen(outputString);
	}
    } else {
	(void) sprintf(outputString,"%s",deviceTypeNames[dataPtr->type]);
    }
    if (dataPtr->length < 0x1f) {
	return strlen(outputString);
    } else {
#ifdef notdef
	char	v[32], p[32], f[32];
	CopyAndTerminateString(8,dataPtr->vendorInfo, v);
	CopyAndTerminateString(8,dataPtr->productInfo, p);
	CopyAndTerminateString(4,dataPtr->firmwareInfo, f);
	(void) sprintf(outputString + strlen(outputString),"%s %s %s",v,p,f);
#endif
	char	v[32], p[32], rl[32], rd[32];
	CopyAndTerminateString(8,(char *) (dataPtr->vendorID), v);
	CopyAndTerminateString(16,(char *) (dataPtr->productID), p);
	CopyAndTerminateString(4,(char *) (dataPtr->revLevel), rl);
	CopyAndTerminateString(8,(char*) (dataPtr->revData), rd);
	(void) sprintf(outputString + strlen(outputString),
	    "%s %s %s", v, p, rl, rd);
    }
    return strlen(outputString);
}

