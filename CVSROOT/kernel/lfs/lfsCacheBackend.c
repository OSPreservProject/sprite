/* 
 * lfsCacheBackend.c --
 *
 *	Routines for file cache backend of an LFS file system.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <lfsInt.h>
#include <stdlib.h>
#include <string.h>

static Fscache_BackendRoutines  lfsBackendRoutines = {
	    Fsdm_BlockAllocate,
	    Fsdm_FileTrunc,
	    Fsdm_FileBlockRead,
	    Fsdm_FileBlockWrite,
	    Lfs_ReallocBlock,
	    Lfs_StartWriteBack,

};

#define	LOCKPTR	&lfsPtr->cacheBackendLock


/*
 *----------------------------------------------------------------------
 *
 * LfsCacheBackendInit --
 *
 *	Initialized the cache backend of an LFS file system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Fscache_Backend *
LfsCacheBackendInit(lfsPtr)
    Lfs		*lfsPtr;	/* LFS file system structure. */
{
    Sync_LockInitDynamic(&(lfsPtr->cacheBackendLock), "LfscacheBackendLock");
    lfsPtr->writeBackActive = FALSE;
    lfsPtr->writeBackMoreWork = FALSE;
    lfsPtr->shutDownActive = FALSE;
    lfsPtr->cacheBlocksReserved = 0;  /* Filled in at end of attach. */
    return Fscache_RegisterBackend(&lfsBackendRoutines,(ClientData) lfsPtr, 0);
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_StartWriteBack --
 *
 *	Request that the segment manager start the segment write sequence
 *	for the specified file system.
 *
 * Results:
 *	True is a backend backend was started.
 *
 * Side effects:
 *	A write back process may be started.
 *
 *----------------------------------------------------------------------
 */

Boolean
Lfs_StartWriteBack(backendPtr)
    Fscache_Backend	*backendPtr; /* LFS file system backend. */
{
    Lfs	 *lfsPtr;	/* File system with data to write. */

    lfsPtr = (Lfs *) (backendPtr->clientData);

    LOCK_MONITOR;
    LFS_STATS_INC(lfsPtr->stats.backend.startRequests);
    if (lfsPtr->writeBackActive) {
	LFS_STATS_INC(lfsPtr->stats.backend.alreadyActive);
	/*
	 * If we already have a SegmentWriteProcess active just set
	 * a bit indicating more work may be present.  This elimates the
	 * race been being notified for work while a WriteBack process
	 * is terminating.
	 */
	lfsPtr->writeBackMoreWork = TRUE;
	UNLOCK_MONITOR;
	return FALSE;
    }
    lfsPtr->writeBackActive = TRUE;
    Proc_CallFunc(LfsSegmentWriteProc, (ClientData) lfsPtr, 0);
    UNLOCK_MONITOR;
    return TRUE;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsStopWriteBack --
 *
 *	Stop cache writeback processing on the specified file system. 
 *	This routine is called at file system shutdown time to wait
 *	for the writeback processes to exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
LfsStopWriteBack(lfsPtr)
    Lfs	*lfsPtr; /* File system to stop writeback for. */
{
    Time time;

    LOCK_MONITOR;
    lfsPtr->shutDownActive = TRUE;
    while (lfsPtr->writeBackActive) {
	time.seconds = 1;
	time.microseconds = 0;
	Sync_WaitTime(time);
    }
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsMoreToWriteBack --
 *
 *	Check to see if we got another request for writeback.. 
 *
 * Results:
 *	TRUE if we got request. FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsMoreToWriteBack(lfsPtr)
    Lfs	*lfsPtr; /* File system to check for more work. */
{

    LOCK_MONITOR;
    if (lfsPtr->shutDownActive) {
	lfsPtr->writeBackMoreWork = FALSE;
    }
    if (lfsPtr->writeBackMoreWork) {
	lfsPtr->writeBackMoreWork = FALSE;
	UNLOCK_MONITOR;
	return TRUE;
    }
    lfsPtr->writeBackActive = FALSE;
    lfsPtr->writeBackMoreWork = FALSE;
    UNLOCK_MONITOR;
    FscacheBackendIdle(lfsPtr->domainPtr->backendPtr);
    return FALSE;
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_ReallocBlock --
 *
 *	Allocate a new block on disk to replace the given block.  This is
 *	intended to be used by the cache when it can't write out a block
 *	because of a disk error.
 *
 * Results:
 * 	None
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Lfs_ReallocBlock(data, callInfoPtr)
    ClientData		data;			/* Block to move */
    Proc_CallInfo	*callInfoPtr;	
{
    panic("Lfs_ReallocBlock called.\n");
}

