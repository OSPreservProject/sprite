/* 
 * devEmulexTape.c --
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


#include <sprite.h>
#include <dev.h>
#include <devInt.h>
#include <sys/scsi.h>
#include <scsiDevice.h>
#include <scsiHBA.h>
#include <scsiTape.h>
#include <emulexTape.h>

/*
 * Sense data returned from the Emulex tape controller in the shoeboxes.
 */
#define EMULEX_SENSE_BYTES	11
typedef struct {
    ScsiClass7Sense	extSense;	/* 8 Bytes */
    unsigned char pad1		:1;
    unsigned char error		:7;	/* Regular SCSI error code */
    unsigned char highRetries;		/* High byte of retry count */
    unsigned char lowRetries;		/* Low byte of retry count */
} EmulexTapeSense;			/* Known to be 11 Bytes big */


/*
 * Definitions for the mode select command.  This is specific to the
 * Emulex controller.  The mode select command is used to change from
 * QIC_24 format (one standard, not the one we use) to QIC_02 format
 * (the more common, older, standard that we do use).
 */

typedef struct EmulexModeSelBlock {
    unsigned char density;		/* Density code */
    unsigned char highCount;		/* Count of blocks at this density */
    unsigned char midCount;		/*	middle byte of above */
    unsigned char lowCount;		/*	low byte */
    unsigned char pad1;			/* Reserved */
    unsigned char highLength;		/* Length of the blocks */
    unsigned char midLength;		/*	middle byte of above */
    unsigned char lowLength;		/*	low byte */
} EmulexModeSelBlock;

/*
 * Density values for the mode select block.
 */
#define EMULEX_QIC_24	0x05
#define EMULEX_QIC_02	0x84

typedef struct EmulexModeSelParams {
    ScsiTapeModeSelectHdr	header;
    EmulexModeSelBlock	block;
    unsigned char		:5;	/* Reserved */
    unsigned char disableErase	:1;	/* disable erase ahead */
    unsigned char autoLoadInhibit :1;
    unsigned char softErrorCount  :1;
} EmulexModeSelParams;


/*
 *----------------------------------------------------------------------
 *
 * DevEmulexAttach --
 *
 *	Verify and initialize the attached scsi device as a emulex tape..
 *
 * Results:
 *	SUCCESS if the device is a working emulex tape drive.
 *	DEV_NO_DEVICE if the device is not a emulex tape drive.
 *	A Sprite return status if the device is a broken emulex tape drive.
 *
 * Side effects:
 *	Sets the type and call-back procedures.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevEmulexAttach(devicePtr, devPtr, tapePtr)
    Fs_Device	*devicePtr;	/* Fs_Device being attached. */
    ScsiDevice	*devPtr;	/* SCSI device handle for drive. */
    ScsiTape	*tapePtr;	/* Tape drive state to be filled in. */
{
    ScsiCmd		senseCmd;
    ReturnStatus	status;
    static char		senseData[SCSI_MAX_SENSE_LEN];
    int			length;
    /*
     * Since we don't know about the inquiry data (if any) returned by 
     * the Emulex tape, check using the size of the SENSE data returned.
     */
    DevScsiSenseCmd(devPtr, SCSI_MAX_SENSE_LEN, senseData, &senseCmd);
    status = DevScsiSendCmdSync(devPtr, &senseCmd, &length);
    if ( (status != SUCCESS) || 
         (senseCmd.statusByte != 0) ||
	 ((senseCmd.senseLen != EMULEX_SENSE_BYTES) &&
	  (senseCmd.senseLen != SCSI_MAX_SENSE_LEN) &&
	  (senseCmd.senseLen != 14)) ){
	return DEV_NO_DEVICE;
    }
    /*
     * Take all the defaults for the tapePtr.
     */
    tapePtr->name = "Emulex Tape";
    return SUCCESS;
}
