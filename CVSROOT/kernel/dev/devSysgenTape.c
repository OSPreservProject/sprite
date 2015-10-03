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
#endif /* not lint */


#include <sprite.h>
#include <dev.h>
#include <devInt.h>
#include <sys/scsi.h>
#include <scsiDevice.h>
#include <scsiHBA.h>
#include <scsiTape.h>
#include <fs.h>
#include <sysgenTape.h>

/*
 * Sense data returned from the Sysgen tape controller.
 * This matches the ARCHIVE Sidewinder drive specifications, and the
 * CIPHER Quarterback drive specifications.
 */
#define SYSGEN_SENSE_BYTES	16
typedef struct {
    /*
     * Standard 4-bytes of sense data, not class 7 extended sense.
     */
    unsigned char valid		:1;	/* Sense data is valid */
    unsigned char error		:7;	/* 3 bits class and 4 bits code */
    unsigned char highAddr;		/* High byte of block address */
    unsigned char midAddr;		/* Middle byte of block address */
    unsigned char lowAddr;		/* Low byte of block address */
    /*
     * Additional 12 bytes of sense data specific to Sysgen drives.
     */
    unsigned char bitSet1	:1;	/* More bits set in this byte */
    unsigned char noCartridge	:1;	/* The tape cartridge isn't there */
    unsigned char noDrive	:1;	/* No such drive (check subUnitID) */
    unsigned char writeProtect	:1;	/* The drive is write protected */
    unsigned char endOfTape	:1;	/* End of tape encountered */
    unsigned char dataError	:1;	/* Data error on the tape, fatal */
    unsigned char noError	:1;	/* No error in the data */
    unsigned char fileMark	:1;	/* File mark encountered */

    unsigned char bitSet2	:1;	/* More bits set in this byte */
    unsigned char badCommand	:1;	/* A bad command was specified */
    unsigned char noData	:1;	/* Counld't find the data */
    unsigned char retries	:1;	/* Had to retry more than 8 times */
    unsigned char beginOfTape	:1;	/* At beginning of tape */
    unsigned char pad1		:2;	/* reserved */
    unsigned char powerOnReset	:1;	/* Drive reset sinse last command */

    short	numRetries;		/* Number of retries */
    short	underruns;		/* Number of underruns */
    /*
     * The following comes from the sysgen controller in copy commands
     * which we don't use.
     */
    char numDiskBlocks[3];		/* Num disk blocks transferred */
    char numTapeBlocks[3];		/* Num tape blocks transferred */

} DevQICIISense;			/* Known to be 16 Bytes big */


/*
 *----------------------------------------------------------------------
 *
 * DevSysgenAttach --
 *
 *	Verify and attach a Sysgen tape drive.
 *
 * Results:
 *	SUCCESS if the device is a working Sysgen tape drive.
 *	DEV_NO_DEVICE if the device is not a working Sysgen tape drive.
 *
 * Side effects:
 *	Sets the type and call-back procedures.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevSysgenAttach(devicePtr, devPtr, tapePtr)
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
     * the Sysgen tape, check using the size of the SENSE data returned.
     */
    DevScsiSenseCmd(devPtr, SCSI_MAX_SENSE_LEN, senseData, &senseCmd);
    status = DevScsiSendCmdSync(devPtr, &senseCmd, &length);
    if ( (status != SUCCESS) || 
         (senseCmd.statusByte != 0) ||
	 (senseCmd.senseLen != SYSGEN_SENSE_BYTES)) {
	return DEV_NO_DEVICE;
    }
    /*
     * Take all the defaults for the tapePtr.
     */
    tapePtr->name = "Sysgen Tape";
    return SUCCESS;
}
