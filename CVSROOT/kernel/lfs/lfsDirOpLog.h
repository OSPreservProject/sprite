/*
 * lfsDirOpLog.h --
 *
 *	Declarations of directory operation log for a LFS file system.
 *
 * Copyright 1990 Regents of the University of California
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

#ifndef _LFSDIROPLOG
#define _LFSDIROPLOG

/* constants */

/* data structures */

typedef struct LfsDirOpLogBlockHdr {
    int		magic;		/* Better be LFS_DIROP_LOG_MAGIC. */
    int		size;		/* Size in bytes of log entries on block. */
    int		nextLogBlock;	/* Block offset of the next log block in 
				 * this segment.  */
    int		reserved;	/* Reserved, must be zero. */
} LfsDirOpLogBlockHdr;

typedef struct LfsDirOpLogEntryHdr {
    int		logSeqNum;	/* Log sequence number of entry. */
    int		opFlags;	/* Directory operation, see fsdm.h */
    int		dirFileNumber;	/* Directory being operated on. */
    int		dirOffset;	/* Offset into directory dirFileNumber of
				 * entry. */
    int		linkCount;	/* Link count of object before operation. */
} LfsDirOpLogEntryHdr;

typedef struct LfsDirOpLogEntry {
    LfsDirOpLogEntryHdr	hdr;	  /* Operation type, fileNumbers and flags. */
    Fslcl_DirEntry      dirEntry; /* Directory entry being operated on. */
} LfsDirOpLogEntry;

#define	LFS_DIR_OP_LOG_ENTRY_SIZE(entryPtr) \
	((entryPtr)->dirEntry.recordLength + sizeof(LfsDirOpLogEntryHdr))


#define	LFS_DIROP_LOG_MAGIC 0x1f5d109
/* procedures */

#endif /* _LFSDIROPLOG */

