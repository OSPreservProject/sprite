/*
 * fsMigrate.h --
 *
 *	Declarations for file migration routines.
 *
 * Copyright 1987, 1988 Regents of the University of California
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

#ifndef _FSMIGRATE
#define _FSMIGRATE

/*
 * Structure that is transfered when a process is migrated.
 */

typedef struct FsMigInfo {
    FsFileID	streamID;	/* Stream identifier. */
    FsFileID    ioFileID;     	/* I/O handle for the stream. */
    FsFileID	nameID;		/* ID of name of the file.  Used for attrs. */
    FsFileID	rootID;		/* ID of the root of the file's domain. */
    int		srcClientID;	/* Client transfering from. */
    int         offset;     	/* File access position. */
    int         flags;      	/* Usage flags from the stream. */
} FsMigInfo;

extern Boolean fsMigDebug;	/* enable migration debugging statements? */

/*
 * File migration routines.
 */
#endif _FSMIGRATE
