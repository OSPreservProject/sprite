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

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/RCS/devSCSI.c,v 8.8 89/05/23 10:02:18 mendel Exp Locker: mendel $ SPRITE (Berkeley)";
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
#include "stdlib.h"
#include "sched.h"


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
	    status = SUCCESS;
	    break;
	case SCSI_CLASS7_NOT_READY:
	    status = DEV_OFFLINE;
	    break;
	case SCSI_CLASS7_MEDIA_ERROR:
	case SCSI_CLASS7_HARDWARE_ERROR:
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_CLASS7_ILLEGAL_REQUEST:
	    /*
	     * Probably a programming error.
	     */
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
	    status = FS_NO_ACCESS;
	    break;
	case SCSI_CLASS7_BLANK_CHECK:
	    status = DEV_HARD_ERROR;
	    break;
	case SCSI_CLASS7_VENDOR:
	case SCSI_CLASS7_ABORT:
	case SCSI_CLASS7_EQUAL:
	case SCSI_CLASS7_OVERFLOW:
	    status = DEV_HARD_ERROR;
	    break;
	default: {
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
 *	None.
 *
 * Side effects:
 *	Set the various fields in the control block.
 *
 *----------------------------------------------------------------------
 */
void
DevScsiGroup0Cmd(devPtr, cmd, blockNumber,countNumber,scsiCmdPtr)
    ScsiDevice	*devPtr; /* SCSI device target for this command. */
    int		cmd;	 /* Group0 scsi command. */
    unsigned int blockNumber;	/* The starting block number for the transfer */
    unsigned int countNumber;	/* Number of sectors (or bytes!) to transfer */
    register ScsiCmd	*scsiCmdPtr; /* Scsi command block to be filled in. */
{
    register  ScsiGroup0Cmd	*c;

     bzero((char *)scsiCmdPtr, sizeof(ScsiCmd));
    scsiCmdPtr->commandBlockLen = sizeof(ScsiGroup0Cmd);
    c = (ScsiGroup0Cmd *) scsiCmdPtr->commandBlock;
    c->command = cmd;
    c->unitNumber = devPtr->LUN;
    c->highAddr = (blockNumber & 0x1f0000) >> 16;
    c->midAddr =  (blockNumber & 0x00ff00) >> 8;
    c->lowAddr =  (blockNumber & 0x0000ff);
    c->blockCount =  countNumber;
}

