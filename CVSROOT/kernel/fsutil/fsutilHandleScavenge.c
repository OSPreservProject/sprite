/* 
 * fsHanldeScavenge.c --
 *
 *	Routines controlling the scavenging of file system handles.
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


#include "sprite.h"

#include "fs.h"
#include "vm.h"
#include "rpc.h"
#include "fsutil.h"
#include "fsprefix.h"
#include "fsNameOps.h"
#include "fsio.h"
#include "fsutilTrace.h"
#include "fsStat.h"
#include "sync.h"
#include "timer.h"
#include "proc.h"
#include "trace.h"
#include "hash.h"
#include "fsrmt.h"


/*
 * Monitor for OkToScavenge and DoneScavenge
 */
static Sync_Lock scavengeLock = Sync_LockInitStatic("Fs:scavengeLock");
#define LOCKPTR (&scavengeLock)

static Boolean OkToScavenge();
void DoneScavenge();


/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleScavengeStub --
 *
 *	This is a thin layer on top of Fsutil_HandleScavenge.  It is called
 *	when L1-x is pressed at the keyboard, and also from Fsutil_HandleInstall
 *	when a threashold number of handles have been created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invokes the handle scavenger.
 *
 *----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
void Fsutil_HandleScavengeStub(data)
    ClientData	data;	/* IGNORED */
{
    /*
     * This is called when the L1-x keys are held down at the console.
     * We set up a call to Fsutil_HandleScavenge, unless there is already
     * an extra scavenger scheduled.
     */
    if (OkToScavenge()) {
	Proc_CallFunc(Fsutil_HandleScavenge, (ClientData)FALSE, 0);
    }
}

Boolean		scavengerScheduled = FALSE;
int		fsScavengeInterval = 2;			/* 2 Minutes */
int		fsLastScavengeTime = 0;


/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleScavenge --
 *
 *	Go through all of the handles looking for clients that have crashed
 *	and for handles that are no longer needed.  This expects to be
 *	called by a helper kernel processes at regular intervals defined
 *	by fsScavengeInterval.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The handle-specific routines may remove handles.
 *
 *----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
void
Fsutil_HandleScavenge(data, callInfoPtr)
    ClientData		data;			/* Whether to reschedule again
						 */
    Proc_CallInfo	*callInfoPtr;		/* Specifies interval */
{
    Hash_Search				hashSearch;
    register	Fs_HandleHeader		*hdrPtr;

    /*
     * Note that this is unsynchronized access to a global variable, which
     * works fine on a uniprocessor.  We don't want a monitor lock here
     * because we don't want a locked handle to hang up all Proc_ServerProcs.
     */
    fsLastScavengeTime = fsutil_TimeInSeconds;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {
	 if (fsio_StreamOpTable[hdrPtr->fileID.type].scavenge !=
		 (Boolean (*)())NIL) {
	     (*fsio_StreamOpTable[hdrPtr->fileID.type].scavenge)(hdrPtr);
	 } else {
	     Fsutil_HandleUnlock(hdrPtr);
	 }
    }
    /*
     * We are called in two cases.  A regular call background call is indicated
     * by a TRUE data value, while an extra scavenge that is done in an
     * attempt to free space is the other case.
     */
    if ((Boolean)data) {
	/*
	 * Set up next background call.
	 */
	callInfoPtr->interval = fsScavengeInterval * timer_IntOneMinute;
    } else {
	/*
	 * Indicate that the extra scavenger has completed.
	 */
	callInfoPtr->interval = 0;
	DoneScavenge();
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * OkToScavenge --
 *
 *	Checks for already active scavengers.  Returns FALSE if there
 *	is already a scavenger.
 *
 * Results:
 *	TRUE if there is no scavenging in progress.
 *
 * Side effects:
 *	Sets scavengerScheduled to TRUE if it had been FALSE.
 *
 *----------------------------------------------------------------------------
 *
 */
static ENTRY Boolean
OkToScavenge()
{
    register Boolean ok;
    LOCK_MONITOR;
    ok = !scavengerScheduled;
    if (ok) {
	scavengerScheduled = TRUE;
    }
    UNLOCK_MONITOR;
    return(ok);
}

/*
 *----------------------------------------------------------------------------
 *
 * DoneScavenge --
 *
 *	Called when done scavenging.  This clears the flag that indicates
 *	an extra scavenger is present.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears scavengerScheduled.
 *
 *----------------------------------------------------------------------------
 *
 */
void
DoneScavenge()
{
    LOCK_MONITOR;
    scavengerScheduled = FALSE;
    UNLOCK_MONITOR;
}
