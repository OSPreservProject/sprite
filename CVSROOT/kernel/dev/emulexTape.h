/*
 * devSCSIEmulex.h
 *
 * Definitions for sense data format and status information returned
 * from Emulex tape drives.
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

#ifndef _DEVSCSIEMULEX
#define _DEVSCSIEMULEX

/*
 * Sense data returned from the Emulux tape controller in the shoeboxes.
 */
typedef struct {
    DevSCSIExtendedSense	extSense;	/* 8 Bytes */
    unsigned char pad1		:1;
    unsigned char error		:7;	/* Regular SCSI error code */
    unsigned char highRetries;		/* High byte of retry count */
    unsigned char lowRetries;		/* Low byte of retry count */
} DevEmuluxTapeSense;			/* Known to be 11 Bytes big */

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



#endif _DEVSCSIEMULEX
