/*
 * devSCSITape.h --
 *
 *	External definitions for the tape drives on the SCSI I/O bus.
 *	There are several variants of tape drives and each has an
 *	associated header file that defines drive-specific sense
 *	data and other info.
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
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSITAPE
#define _DEVSCSITAPE

#include "dev/tape.h"

/*
 * State info for an SCSI tape drive.  This is used to map from a device
 * unit number back to the SCSI controller for the drive.
 */
typedef struct DevSCSITape {
     int state;			/* State bits used to determine if it's open,
				 * it it really exists, etc. */
    int type;			/* Type of the drive */
    void (*setupProc)();	/* Procedure to customize command block */
    void (*statusProc)();	/* Procedure to fill in Dev_TapeStatus */
    ReturnStatus (*errorProc)();/* Procedure to handle sense data */
} DevSCSITape;

extern DevSCSIDevice *scsiTape[];
extern int scsiTapeIndex;

/*
 * SCSI_MAX_TAPES the maximum number of tape devices that can be hung
 *	off ALL the SCSI controllers together.
 */
#define SCSI_MAX_TAPES	4

/*
 * Tape drive state:
 *	SCSI_TAPE_CLOSED	The device file corresponding to the drive
 *		is not open.  This inhibits read/writes, and allows opens.
 *	SCSI_TAPE_OPEN		The device file is open.
 *	SCSI_TAPE_AT_EOF	The file mark was hit on the last read.
 *	SCSI_TAPE_WRITTEN	The last op on the tape was a write.  This is
 *				used to write an end-of-tape mark before
 *				closing or rewinding.
 *	SCSI_TAPE_RETENSION	A retention operation is pending.  This is
 *				is implemented as part of rewind operations.
 */
#define SCSI_TAPE_CLOSED	0x0
#define SCSI_TAPE_OPEN		0x1
#define SCSI_TAPE_AT_EOF	0x2
#define SCSI_TAPE_WRITTEN	0x4
#define SCSI_TAPE_RETENSION	0x8

/*
 * Tape drive types:
 *	SCSI_SYSGEN		Sysgen is a controller type found on 2/120's
 *				It controls a QIC drive.
 *	SCSI_EMULEX		The Emulex drives are found in the Shoeboxes.
 *	SCSI_EXABYTE		The 8mm Exabyte drive.
 */
#define SCSI_UNKNOWN	0
#define SCSI_SYSGEN	1
#define SCSI_EMULEX	2
#define SCSI_EXABYTE	3


/*
 * Version of the control block used for tape commands.
 */
typedef struct DevSCSITapeControlBlock {
    unsigned char command;		/* command code, defined below */
    unsigned char unitNumber	:3;	/* LUN to pass command to */
    unsigned char code		:5;	/* command dependent value */
    unsigned char highCount;		/* High bits of block count */
    unsigned char midCount;		/* Mid bits of block count */
    unsigned char lowCount;		/* Low bits of block count */
    unsigned char vendor57	:1;	/* Vendor unique bit */
    unsigned char vendor56	:1;	/* Vendor unique bit */
    unsigned char pad1		:4;	/* Reserved */
    unsigned char linkIntr	:1;	/* Interrupt after linked command */
    unsigned char link		:1;	/* Another command follows */
} DevSCSITapeControlBlock;

/*
 * The MODE_SELECT command takes a standard 4 byte header and is
 * followed by zero or more BlockDescriptors and zero or more Vendor
 * Unique Parameter bytes.  The total length of the header, block
 * descriptors, and parameter bytes is put in the count field
 * of the command block.
 */
typedef struct DevSCSIModeSelectHdr {
    unsigned char pad1;			/* Reserved */
    unsigned char pad2;			/* Reserved */
    unsigned char		:1;	/* Reserved */
    unsigned char bufMode	:3;	/* == 1 means buffered, the default */
    unsigned char speed		:4;	/* == 0 means default speed */
    unsigned char blockLength;		/* Length of block descriptors that
					 * follow the header */
} DevSCSIModeSelectHdr;

/*
 * Forward Declarations.
 */
ReturnStatus Dev_SCSITapeOpen();
ReturnStatus Dev_SCSITapeRead();
ReturnStatus Dev_SCSITapeWrite();
ReturnStatus Dev_SCSITapeIOControl();
ReturnStatus Dev_SCSITapeClose();

ReturnStatus Dev_SCSITapeBlockIOInit();
ReturnStatus Dev_SCSITapeBlockIO();

ReturnStatus	DevSCSITapeInit();
int		DevSCSITapeType();
ReturnStatus	DevSCSITapeIO();
void		DevSCSITapeSetupCommand();
ReturnStatus	DevSCSITapeError();

#endif _DEVSCSITAPE
