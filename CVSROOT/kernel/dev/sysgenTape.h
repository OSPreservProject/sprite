/*
 * devSCSISysgen.h
 *
 * Definitions for sense data format and status information returned
 * from Sysgen tape drives.
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

#ifndef _DEVSCSISYSGEN
#define _DEVSCSISYSGEN

/*
 * The Sysgen controller uses 512 byte blocks.
 */
#define DEV_SYSGEN_BLOCK_SIZE	512

/*
 * Sense data returned from the Sysgen tape controller.
 * This matches the ARCHIVE Sidewinder drive specifications, and the
 * CIPHER Quarterback drive specifications.
 */
#define DEV_SYSGEN_SENSE_BYTES	16
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

void DevSysgenInit();

#endif _DEVSCSISYSGEN
