/* 
 * fsutilSync.c --
 *
 * Routines controlling the syncing of cached data to disk or the server.
 *
 * Copyright 1989 Regents of the University of California
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
#endif not lint


#include <sprite.h>

#include <fs.h>
#include <vm.h>
#include <rpc.h>
#include <fsutil.h>
#include <fsdm.h>
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsprefix.h>
#include <fsStat.h>
#include <sync.h>
#include <timer.h>
#include <proc.h>
#include <trace.h>
#include <hash.h>
#include <fsrmt.h>

#include <stdio.h>

#define	MAX_WAIT_INTERVALS	5


Boolean fsutil_ShouldSyncDisks;

int		fsWriteBackInterval = 30;	/* How long blocks have to be
						 * dirty before they are
						 * written back. */
int		fsWriteBackCheckInterval = 5;	/* How often to scan the
						 * cache for blocks to write
						 * back. */
Boolean		fsutil_ShouldSyncDisks = TRUE;	/* TRUE means that we should
						 * sync the disks when
						 * Fsutil_SyncProc is called. */
int		lastHandleWBTime = 0;		/* Last time that wrote back
						 * file handles. */
/*
 *----------------------------------------------------------------------
 *
 * Fsutil_SyncProc --
 *
 *	Process to loop and write back things every thiry seconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Fsutil_SyncProc(data, callInfoPtr)
    ClientData		data;		/* IGNORED */
    Proc_CallInfo	*callInfoPtr;
{
    int	blocksLeft;

    if (Fsutil_TimeInSeconds() - lastHandleWBTime >= fsWriteBackInterval) {
	(void) Fsutil_HandleDescWriteBack(FALSE, -1);
	lastHandleWBTime = Fsutil_TimeInSeconds();
    }

    if (fsutil_ShouldSyncDisks) {
	Fscache_WriteBack((unsigned) (Fsutil_TimeInSeconds() -
		fsWriteBackInterval), &blocksLeft, FALSE);
    }
    if (fsWriteBackCheckInterval < fsWriteBackInterval) {
	callInfoPtr->interval = fsWriteBackCheckInterval * timer_IntOneSecond;
    } else {
	callInfoPtr->interval = fsWriteBackInterval * timer_IntOneSecond;
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Fsutil_Sync --
 *
 *	Write back bit maps, file descriptors, and all dirty cache buffers.
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
Fsutil_Sync(writeBackTime, shutdown)
    unsigned int writeBackTime;	/* Write back all blocks in the cache and file
			           descriptors that were dirtied before 
				   this time. */
    Boolean	shutdown;	/* TRUE if the kernel is being shutdown. */
{
    int		blocksLeft = 0;
    /*
     * Force all file descriptors into the cache.
     */
    (void) Fsutil_HandleDescWriteBack(shutdown, -1);
    /*
     * Write back the cache.
     */
    Fscache_WriteBack(writeBackTime, &blocksLeft, shutdown);
    if (shutdown) {
	if (blocksLeft) {
	    printf("Fsutil_Sync: %d blocks still locked\n", blocksLeft);
	}
#ifdef notdef
	Fscache_CleanBlocks((ClientData) FALSE, (Proc_CallInfo *) NIL);
#endif
    }
    /*
     * Finally write all domain information to disk.  This will mark each
     * domain to indicate that we went down gracefully and recovery is in
     * fact possible.
     */
    Fsdm_DomainWriteBack(-1, shutdown, FALSE);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_SyncStub --
 *
 *	Procedure bound to the L1-w keystoke.  This is called at
 *	keyboard interrupt time and so it makes a Proc_CallFunc
 *	to invoke the Fsutil_Sync procedure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Attempts to sync the disks.
 *
 *----------------------------------------------------------------------
 */
void SyncCallBack();

void
Fsutil_SyncStub(data)
    ClientData		data;
{
    printf("Queueing call to Fsutil_Sync() ... ");
    Proc_CallFunc(SyncCallBack, data, 0);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_SyncCallBack --
 *
 *	Procedure called via Proc_CallFunc to sync the disks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Syncs the disk.
 *
 *----------------------------------------------------------------------
 */
void
SyncCallBack(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    printf("Syncing disks");
    Fsutil_Sync(-1, (Boolean)data);
    callInfoPtr->interval = 0;
    printf(".\n");
}

