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

#include "fsNameOpsInt.h"
/*
 * Structure that is transfered when a process is migrated.
 */

typedef struct FsMigInfo {
    Fs_FileID	streamID;	/* Stream identifier. */
    Fs_FileID    ioFileID;     	/* I/O handle for the stream. */
    Fs_FileID	nameID;		/* ID of name of the file.  Used for attrs. */
    Fs_FileID	rootID;		/* ID of the root of the file's domain. */
    int		srcClientID;	/* Client transfering from. */
    int         offset;     	/* File access position. */
    int         flags;      	/* Usage flags from the stream. */
} FsMigInfo;

extern Boolean fsMigDebug;	/* enable migration debugging statements? */

/*
 * The following record defines what parameters the I/O server returns
 * after being told about a migration.
 */
typedef struct FsMigrateReply {
    int flags;		/* New stream flags, the FS_RMT_SHARED bit is modified*/
    int offset;		/* New stream offset */
} FsMigrateReply;

/*
 * This structure is for byte-swapping the rpc parameters correctly.
 */
typedef struct  FsMigParam {
    int			dataSize;
    FsUnionData		data;
    FsMigrateReply	migReply;
} FsMigParam;

/*
 * File migration utilities.
 */
extern ReturnStatus	FsMigrateUseCounts();
extern void		FsIOClientMigrate();
extern ReturnStatus	FsNotifyOfMigration();
#endif /* _FSMIGRATE */
