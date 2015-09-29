/*
 * exabyte.h --
 *
 *	Declarations for Exabyte tape drives.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/exabyte.h,v 1.3 91/09/02 14:04:31 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _EXABYTE
#define _EXABYTE

#include <sys/scsi.h>
/*
 * Sense data for exb8200.
 */

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

} Exb8200Sense;				

typedef struct {
    ScsiClass7Sense	extSense;	/* 8 Bytes */
    unsigned char pad8;			/* Reserved */
    unsigned char pad;			/* Reserved */
    unsigned char pad10;		/* Reserved */
    unsigned char underrun;		/* Underrun/overrun counter */
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

    unsigned char trackingRetry;	/* Tracking retry counter. */
    unsigned char readWriteRetry;	/* Read/Write retry counter. */
    unsigned char faultCode;		/* Fault symptom code. */
} Exb8500Sense;				

/*
 * Values for the drive-specific fields in Dev_TapeStatus.
 */

/*
 * bufferedMode
 */

#define DEV_EXB8500_UNBUFFERED_MODE	0x00 	/* Data is unbuffered. */
#define DEV_EXB8500_BUFFERED_MODE	0x01    /* Data is buffered. */

/*
 * Density
 */
#define DEV_EXB8500_8200_MODE		0x14	/* Exb8200 mode. */

#endif /* _EXABYTE */

