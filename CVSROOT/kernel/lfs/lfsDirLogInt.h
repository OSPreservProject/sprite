/*
 * lfsDirLogInt.h --
 *
 *	Declarations of data structure internal to the implemenation of
 *	a directory change log for a LFS file system.
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

#ifndef _LFSDIRLOGINT
#define _LFSDIRLOGINT

#include <lfsDirOpLog.h>

/* data structures */

typedef struct LfsDirLog {
    int			nextLogSeqNum;	/* The next log sequence number to be
					 * allocated. */
    LfsDirOpLogBlockHdr *curBlockHdrPtr; /* The log block header of the block
					 * current being filled in. */
    char		*nextBytePtr;	/* The next available byte in the
					 * block being filled in. */
    int			bytesLeftInBlock;/* Number of bytes left in the
					  * block being filled in. */
    List_Links		activeListHdr;   /* List cache blocks of log blocks. */
    List_Links		writingListHdr;   /* List cache blocks of log blocks
					   * being written. */
    Fsio_FileIOHandle   handle;		 /* File handle used to cache blocks
					  * under. */
    int			leastCachedSeqNum; /* The least log sequence number in
					    * the in memory log. */
    Boolean		paused;		/* Log traffic is currently paused. */
    Sync_Condition	logPausedWait;  /* Wait for paused to become false. */
} LfsDirLog;

extern LfsDirOpLogEntry *LfsDirLogEntryAlloc _ARGS_((struct Lfs *lfsPtr, 
			int entrySize, int logSeqNum, Boolean *foundPtr));

#endif /* _LFSDIRLOGINT */

