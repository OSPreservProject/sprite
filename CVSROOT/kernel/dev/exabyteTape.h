/*
 * devSCSIExabyte.h
 *
 * Definitions for sense data format and status information returned
 * from Exabyte tape drives.  Reference, the "EXB-8200 8mm Tape Drive
 * User's Guide" by Perfect Byte, Inc. 7121 Cass St. Omaha, NE 68132
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
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIEXABYTE
#define _DEVSCSIEXABYTE

/*
 * Sense data returned from the Exabyte tape controller.
 */
typedef struct {
    DevSCSIExtendedSense	extSense;	/* 8 Bytes */
    unsigned char pad8			/* Reserved */
    unsigned char pad9			/* Reserved */
    unsigned char pad10			/* Reserved */
    unsigned char pad11			/* Reserved */
    /*
     * SCSI 2 support.
     */
    unsigned char senseCode;		/* 0x4 if sense key is NOT_READY */
    unsigned char senseCodeQualifier;	/* 00 - volume not mounted.
					 * 01 - rewinding or loading */
    unsigned char pad14			/* Reserved */
    unsigned char pad15			/* Reserved */
    unsigned char highErrorCnt;		/* High byte of error count */
    unsigned char midErrorCnt;		/* Middle byte of error count */
    unsigned char lowErrorCnt;		/* Low byte of error count */
    /*
     * Error bits that are command dependent.  0 is ok, 1 means error.
     * These are defined on pages 37-38 of the User Manual, Rev.03
     */
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

    unsigned char pad22;		/* Reserved */
    unsigned char highRemainingTape;	/* High byte of remaining tape len */
    unsigned char midRemainingTape;	/* Middle byte of remaining tape len */
    unsigned char lowRemainingTape;	/* Low byte of remaining tape len */

} DevExabyteSense;			/* Known to be 26 Bytes big (for
					 * Drives made in/after 1988) */

/*
 * Sense data returned from the Emulex disk drive in the scsibox.  There's
 * some more information available past the additional sense code, but we don't
 * use it (at least for now).
 */
typedef struct {
    DevSCSIExtendedSense	extSense;
    unsigned char pad1;		/* reserved */
    unsigned char pad2;		/* reserved */
    unsigned char pad3;		/* reserved */
    unsigned char pad4;		/* reserved */
    unsigned char code2;	/* additional sense code */
}  DevEmulexDiskSense;


/*
 * Definitions for the mode select command.  This is specific to the
 * Emulux controller.  The mode select command is used to change from
 * QIC_24 format (one standard, not the one we use) to QIC_02 format
 * (the more common, older, standard that we do use).
 */
typedef struct DevEmuluxModeSelHdr {
    unsigned char pad1;			/* Reserved */
    unsigned char pad2;			/* Reserved */
    unsigned char		:1;	/* Reserved */
    unsigned char bufMode	:3;	/* == 1 */
    unsigned char speed		:4;	/* == 0 */
    unsigned char blockLength;		/* Length of block descriptors that
					 * follow the header */
} DevEmuluxModeSelHdr;

typedef struct DevEmuluxModeSelBlock {
    unsigned char density;		/* Density code */
    unsigned char highCount;		/* Count of blocks at this density */
    unsigned char midCount;		/*	middle byte of above */
    unsigned char lowCount;		/*	low byte */
    unsigned char pad1;			/* Reserved */
    unsigned char highLength;		/* Length of the blocks */
    unsigned char midLength;		/*	middle byte of above */
    unsigned char lowLength;		/*	low byte */
} DevEmuluxModeSelBlock;

/*
 * Density values for the mode select block.
 */
#define SCSI_EMULUX_QIC_24	0x05
#define SCSI_EMULUX_QIC_02	0x84

typedef struct DevEmuluxModeSelParams {
    DevEmuluxModeSelHdr		header;
    DevEmuluxModeSelBlock	block;
    unsigned char		:5;	/* Reserved */
    unsigned char disableErase	:1;	/* disable erase ahead */
    unsigned char autoLoadInhibit :1;
    unsigned char softErrorCount  :1;
} DevEmuluxModeSelParams;



#endif _DEVSCSIEXABYTE
