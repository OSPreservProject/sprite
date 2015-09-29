/*
 * fsrecovDirLog.h --
 *
 *	Declarations of dir log recovery from recovery box.
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
 * $Header: /sprite/src/kernel/Cvsroot/kernel/fsrecov/fsrecovDirLog.h,v 1.1 92/08/10 17:32:56 mgbaker Exp $ SPRITE (Berkeley)
 */

#ifndef _FSRECOV_DIR_LOG
#define _FSRECOV_DIR_LOG

/* constants */
#define	FSRECOV_DIR_LOG_TYPE	1
#define	FSRECOV_NUM_DIR_LOG_ENTRIES	100


#define	FSRECOV_LOG_START_ENTRY FSDM_LOG_START_ENTRY
#define	FSRECOV_LOG_END_ENTRY FSDM_LOG_END_ENTRY

typedef struct Fsrecov_DirLogEntry {
    int                 logSeqNum;      /* Log sequence number of entry. */
    int                 opFlags;        /* Directory operation, see fsdm.h */
    int                 dirFileNumber;  /* Directory being operated on. */
    int                 dirOffset;      /* Offset into directory of entry. */
    int                 linkCount;      /* Link count of object before op. */
    int			startTime;	/* For figuring first entry on recov. */
    Fslcl_DirEntry      dirEntry;       /* Directory entry for op. */
} Fsrecov_DirLogEntry;

#endif /* _FSRECOV_DIR_LOG */
