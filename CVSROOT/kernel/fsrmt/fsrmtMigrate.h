/*
 * fsRmtMigrate.h --
 *
 *	Declarations for RMT file migration routines.
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

#ifndef _FSRMTMIGRATE
#define _FSRMTMIGRATE

#include "fsNameOps.h"
#include "fsNameOpsInt.h"
extern Boolean fsio_MigDebug;	/* enable migration debugging statements? */

/*
 * The following record defines what parameters the I/O server returns
 * after being told about a migration.
 */
typedef struct FsrmtMigrateReply {
    int flags;		/* New stream flags, the FS_RMT_SHARED bit is modified*/
    int offset;		/* New stream offset */
} FsrmtMigrateReply;

/*
 * This structure is for byte-swapping the rpc parameters correctly.
 */
typedef struct  FsrmtMigParam {
    int			dataSize;
    FsrmtUnionData		data;
    FsrmtMigrateReply	migReply;
} FsrmtMigParam;

#endif _FSRMTMIGRATE
