/* 
 * fsRecovery.c
 *
 *	Routines for filesytem recovery.  A file server keeps state about
 *	client use, and this needs to be recovered after the server reboots.
 *	The first routine, Fsutil_Reopen, goes through a sequence of steps that
 *	a client makes in order to help the server in this way.  Other
 *	routines are used by other parts of the filesystem to wait for
 *	recovery to happen, and are typically invoked after a remote
 *	operation fails due to a communication failure.  These are
 *	Fsutil_WantRecovery, and Fsutil_WaitForRecovery.
 *	The routine Fsutil_WaitForHost
 *	is combines these two calls and is used by routines outside
 *	the filesystem, i.e. when vm waits on swap files.
 *
 * Copyright 1987 Regents of the University of California.
 * All rights reserved.
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
#include <fsutil.h>
#include <fsprefix.h>
#include <fsio.h>
#include <fsrmt.h>
#include <fsNameOps.h>
#include <fsStat.h>
#include <recov.h>
#include <hash.h>
#include <rpc.h>
#include <vm.h>
#include <rpcPacket.h>

#include <string.h>
#include <stdio.h>


static void ReopenHandles _ARGS_((int serverID));
static void RecoveryComplete _ARGS_((Fsutil_RecoveryInfo *recovPtr, 
			ReturnStatus status));
static void RecoveryNotify _ARGS_((Fsutil_RecoveryInfo *recovPtr));
static Boolean RecoveryFailed _ARGS_((Fsutil_RecoveryInfo *recovPtr));
static Boolean OkToScavenge _ARGS_((register Fsutil_RecoveryInfo *recovPtr));
static void RecoveryDone _ARGS_((int serverID));
extern Boolean RemoteHandle _ARGS_((Fs_HandleHeader *hdrPtr));
extern ReturnStatus RecoveryWait _ARGS_((Fsutil_RecoveryInfo *recovPtr));

static ReturnStatus DoBulkReopen _ARGS_((int serverID));
static void FinishReopenHandles _ARGS_((void));
static ReturnStatus AddReopenHandle _ARGS_((Fs_HandleHeader *hdrPtr));
static void InitReopenHandles _ARGS_((void));
void Fsutil_InitBulkReopenOps _ARGS_((int type,
	Fsutil_BulkReopenOps *reopenOpsPtr));

/*
 * The recovery state for each file is monitored.
 */
#define LOCKPTR (&recovPtr->lock)

/*
 * Flags for the recovery state.
 *	RECOVERY_NEEDED		The handle needs to be re-opened at the server.
 *	RECOVERY_COMPLETE	The recovery actions have been completed.
 *	RECOVERY_FAILED		The last re-open attempt failed.
 */
#define RECOVERY_NEEDED		0x1
#define RECOVERY_COMPLETE	0x2
#define RECOVERY_FAILED		0x4

/*
 * A global counter of the clients active in recovery is kept.
 * This is used to control print statements so we aren't noisey
 * in the middle of recovery, but only at the beginning and end.
 */
int fsutil_NumRecovering = 0;


/*
 *----------------------------------------------------------------------
 *
 * Fsutil_Reopen --
 *
 *	Re-establish state with a file server.  First the prefixes for
 * 	the server are re-opened, then the handle table is scanned and
 *	all handles from that server are re-opened.  We are called via
 *	the recovery callbacks.  As a special favor to the VM module
 *	we tell it after we have recovered state so that it can
 *	recover after the swap server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None by this routine itself, but when this returns the routines
 *	it has called will have reestablished state with the server.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Fsutil_Reopen(serverID, clientData)
    int serverID;		/* Server we are recovering with */
    ClientData clientData;	/* IGNORED */
{
    Boolean		fastBoot;

    fastBoot = Recov_GetHostState(serverID) & RECOV_FAST_BOOT;
    /*
     * Ensure only one instance of Fsutil_Reopen by doing a set-and-test.
     */
    if (Recov_SetClientState(serverID, SRV_RECOV_IN_PROGRESS)
	    & SRV_RECOV_IN_PROGRESS) {
	return;
    }
    /*
     * Recover the prefix table.
     */
    Fsprefix_Reopen(serverID);

    if (!fastBoot || recov_ClientIgnoreTransparent) {
	/*
	 * Wait for opens in progress, then block opens.
	 */
	Fsprefix_RecoveryCheck(serverID);
	/*
	 * Recover file handles
	 */
	ReopenHandles(serverID);
	/*
	 * Tell the server we're done.
	 */
	RecoveryDone(serverID);
	/*
	 * Allow regular opens.
	 */
	Fsprefix_AllowOpens(serverID);
    }
    /*
     * Clear the recovery bit before kicking processes.  Some processes
     * may be locked down doing pseudo-device request/response, and the
     * proc wakeup call will block on them.  To prevent deadlock we
     * have to mark recovery as complete first.
     */
    Recov_ClearClientState(serverID, SRV_RECOV_IN_PROGRESS);
    /*
     * Tell VM that we have recovered in case this was the swap server.  
     * This comes before we kick blocked processes, so that we can avoid a
     * potential deadlock: if a process in the middle of forking hangs
     * because the swap server is down, its child might be locked.  If this
     * happens, the wakeup routine will hang waiting to unlock the child,
     * which means the parent must be started first, so that the child will
     * eventually be unlocked.  (See Sprite bug report 32338.)
     */
    Vm_Recovery();
    /*
     * Kick all processes in case any are blocking on I/O
     */
    Proc_WakeupAllProcesses();
}

/*
 *----------------------------------------------------------------------------
 *
 * ReopenHandles --
 *
 *	Called after the prefix table handles for the given server
 *	have been recovered.  This scans the handle table twice
 *	to recover I/O handles and streams.  The first pass re-opens
 *	the I/O handles, and then streams are done.  A file-type specific
 *	routine is called to do the reopen.  If the recovery fails this
 *	logs a warning message and marks the handle as invalid.  A final
 *	third pass over the handles is done to wakeup processes that are
 *	waiting on recovery.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This will invalidate handles for which recovery fails.  Invalid
 *	I/O handles are removed from the handle table, while invalid
 *	streams are left around because they get removed at close-time.
 *
 *----------------------------------------------------------------------------
 */

static void
ReopenHandles(serverID)
    int		serverID;	/* The to re-establish contact with. */
{
    Hash_Search			hashSearch;
    register	Fs_HandleHeader	*hdrPtr;
    register	Fs_Stream	*streamPtr;
    register	Fsrmt_IOHandle *rmtHandlePtr;
    ReturnStatus		status = SUCCESS;
    Boolean			printed = FALSE;
    int				succeeded = fs_Stats.recovery.succeeded;
    int				failed = fs_Stats.recovery.failed;
					/* This boolean is only to retry
					 * stuff with single rpc's if the
					 * server doesn't recognize a
					 * bulk rpc.   GET RID OF IT WHEN
					 * ALL KERNELS HAVE BULK RPC'S. */
    Boolean			doBulkRpc = TRUE;
    ReturnStatus		checkStatus = FAILURE;

doSingleRpcsInstead:
    if (doBulkRpc && recov_BulkHandles && serverID == 53) {
	InitReopenHandles();
    }
    if (!doBulkRpc) {
	printf("Server %d couldn't do bulk reopen - starting again.\n",
		serverID);
    }
    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {
	 if ((hdrPtr->fileID.type != FSIO_STREAM) &&
		 (hdrPtr->fileID.serverID == serverID)) {
	    if (!RemoteHandle(hdrPtr)) {
		panic("ReopenHandles, local I/O handle at remote server?\n");
	    }
	    if (!printed) {
		Net_HostPrint(serverID, "- recovering handles\n");
		printed = TRUE;
	    }
#ifdef NOTDEF

/*
 * For debugging it would be nice to be able to check for this, but
 * FS_HANDLE_INVALID isn't defined here.
 */
	    if (hdrPtr->flags & FS_HANDLE_INVALID) {
		printf("Attempting to reopen invalid handle!!!\n");
	    }
#endif NOTDEF
	    if (doBulkRpc && recov_BulkHandles && serverID == 53) {
		status = AddReopenHandle(hdrPtr);
		if (status != SUCCESS && status != FS_NO_HANDLE) {
		    Fsutil_HandleUnlock(hdrPtr);
		}
	    } else {
		status = (*fsio_StreamOpTable[hdrPtr->fileID.type].reopen)
			(hdrPtr, rpc_SpriteID, (ClientData)NIL, (int *)NIL,
			(ClientData *)NIL);
		RecoveryComplete(&(((Fsrmt_IOHandle *)hdrPtr)->recovery),
				    status);
		/*
		 * If we removed the handle because it was no longer needed,
		 * we can't try unlocking it.
		 */
		if (status != FS_NO_HANDLE) {
		    Fsutil_HandleUnlock(hdrPtr);
		}
		switch (status) {
		    case SUCCESS:
			break;
		    case FS_NO_HANDLE:
			/* We didn't need to recover this. */
			break;
		    case RPC_SERVICE_DISABLED:
		    case RPC_TIMEOUT:
			goto reopenReturn;
		    case FS_FILE_REMOVED:
			/*
			 * No noisy message, this is a common case.
			 */
			break;
		    default:
			Fsutil_FileError(hdrPtr, "Reopen failed ", status);
			break;
		}
	    }
	} else {
	    Fsutil_HandleUnlock(hdrPtr);
	}
    }
    if (doBulkRpc && recov_BulkHandles && serverID == 53) {
	status = DoBulkReopen(serverID);
	FinishReopenHandles();
	if (status == RPC_INVALID_RPC) {
	    doBulkRpc = FALSE;
	    goto doSingleRpcsInstead;
	}
	if (status != SUCCESS) {
	    goto reopenReturn;
	}
    }
    /*
     * Now go through and recover streams, once we've gotten the regular
     * I/O handles re-opened.  This ensures that the I/O handle will be
     * around on the server when it re-creates our streams.  If recovery
     * fails on a stream we invalidate its handle, but don't remove it
     * because that happens in the top-level close routine.
     */
    if (doBulkRpc && recov_BulkHandles && serverID == 53) {
	InitReopenHandles();
    }
    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {
	checkStatus = FAILURE;
	if ((hdrPtr->fileID.type == FSIO_STREAM) &&
		 (hdrPtr->fileID.serverID == serverID)) {
	    streamPtr = (Fs_Stream *)hdrPtr;
	    rmtHandlePtr = (Fsrmt_IOHandle *)streamPtr->ioHandlePtr;

	    if (rmtHandlePtr == (Fsrmt_IOHandle *)NIL) {
		Fsutil_FileError((Fs_HandleHeader *)streamPtr,
			"ReopenHandles: NIL I/O handle", 0);
	    } else if (!RemoteHandle((Fs_HandleHeader *)rmtHandlePtr)) {
		panic( "ReopenHandles: local I/O handle for remote stream?\n");
	    } else if (RecoveryFailed(&rmtHandlePtr->recovery)) {
		Fsutil_HandleInvalidate((Fs_HandleHeader *)streamPtr);
	    } else {
		if (doBulkRpc && recov_BulkHandles && serverID == 53) {
		    checkStatus =
			    AddReopenHandle((Fs_HandleHeader *) streamPtr);
		} else {
		    status = Fsio_StreamReopen((Fs_HandleHeader *)streamPtr,
				    rpc_SpriteID, (ClientData)NIL, (int *)NIL,
				    (ClientData *)NIL);
		    if (status != SUCCESS) {
			Fsutil_FileError((Fs_HandleHeader *)streamPtr,
			    "Reopen failed", status);
			Fsutil_FileError(streamPtr->ioHandlePtr,"I/O handle",
				    SUCCESS);
			if ((status == RPC_TIMEOUT) ||
			    (status == RPC_SERVICE_DISABLED)) {
			    Fsutil_HandleUnlock(streamPtr);
			    goto reopenReturn;
			}
			Fsutil_HandleInvalidate((Fs_HandleHeader *)streamPtr);
		    }
		}
	    }
	}
	/*
	 * The only time we don't unlock the handle in this loop is if
	 * it was a stream handle we've put into the array for a bulk
	 * reopen.  That's only the case if the AddReopenHandle called
	 * returned with success.
	 */
	if (!(doBulkRpc && recov_BulkHandles && serverID == 53 &&
		checkStatus == SUCCESS)) {	
	    Fsutil_HandleUnlock(hdrPtr);
	}
    }

    if (doBulkRpc && recov_BulkHandles && serverID == 53) {
	status = DoBulkReopen(serverID);
	FinishReopenHandles();
	if (status != SUCCESS) {
	    if (status != RPC_TIMEOUT && status != RPC_SERVICE_DISABLED) {
		panic("Bad status from DoBulkReopen should be rpc-related.");
	    }
	    goto reopenReturn;
	}
    }
    /*
     * Now we notify processes waiting on I/O handles, and invalidate
     * those I/O handles which failed recovery.
     */
    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {
	 if (!RemoteHandle(hdrPtr)) {
	     Fsutil_HandleUnlock(hdrPtr);
	 } else {
	     RecoveryNotify(&((Fsrmt_IOHandle *)hdrPtr)->recovery);
	     if (RecoveryFailed(&((Fsrmt_IOHandle *)hdrPtr)->recovery)) {
		 Fsutil_HandleInvalidate(hdrPtr);
	     }
	     Fsutil_HandleUnlock(hdrPtr);
	 }
    }
reopenReturn:
    if (status != SUCCESS) {
	Net_HostPrint(serverID, "Recovery failed: ");
	Fsutil_PrintStatus(status);
	printf("\n");
    } else if (printed) {
	Net_HostPrint(serverID, "Recovery complete");
	printf(" %d handles reopened", fs_Stats.recovery.succeeded - succeeded);
	if (fs_Stats.recovery.failed - failed > 0) {
	    printf(" %d failed reopens", fs_Stats.recovery.failed - failed);
	}
	printf("\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_RecoveryInit --
 *
 *	This routine is called to reset the recovery state for
 *	a handle when it is first set up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Zero's out the struct.
 *
 *----------------------------------------------------------------------
 */
void
Fsutil_RecoveryInit(recovPtr)
    register Fsutil_RecoveryInfo	*recovPtr;	/* Recovery state */
{
    bzero((Address) recovPtr, sizeof(Fsutil_RecoveryInfo));
    Sync_LockInitDynamic(&recovPtr->lock, "fs:recoveryLock");
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_RecoverySyncLockCleanup --
 *
 *	This routine is called when removing a handle to .
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Zero's out the struct.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Fsutil_RecoverySyncLockCleanup(recovPtr)
    Fsutil_RecoveryInfo	*recovPtr;	/* Recovery state */
{
    Sync_LockClear(&recovPtr->lock);
}

/*
 *----------------------------------------------------------------------
 *
 * RemoteHandle --
 *
 *	This checks the type of a handle to see if it is remote and thus
 *	has a Fsrmt_IOHandle structure embedded into it.  Only these
 *	kinds of handles are manipulated by the recovery routines.
 *
 * Results:
 *	TRUE if the handle has a remote server.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
RemoteHandle(hdrPtr)
    Fs_HandleHeader *hdrPtr;
{
    switch(hdrPtr->fileID.type) {
	case FSIO_RMT_FILE_STREAM:
	case FSIO_RMT_DEVICE_STREAM:
	case FSIO_RMT_PIPE_STREAM:
	case FSIO_RMT_PSEUDO_STREAM:
	case FSIO_PFS_NAMING_STREAM:
	case FSIO_RMT_PFS_STREAM:
	case FSIO_CONTROL_STREAM:
	case FSIO_PFS_CONTROL_STREAM:
	    return(TRUE);
	default:
	    return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_WaitForHost --
 *
 *	Wait until recovery actions have completed for the stream.
 *	This will return failure codes if the recovery aborts.
 *	If the non-blocking flag is passed in this just marks the
 *	handle as needing recovery and returns SUCCESS.
 *
 * Results:
 *	SUCCESS unless there was a recovery error.
 *
 * Side effects:
 *	Block the process.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsutil_WaitForHost(streamPtr, flags, rpcStatus)
    Fs_Stream	*streamPtr;
    int		flags;		/* 0 or FS_NON_BLOCKING */
    ReturnStatus rpcStatus;	/* Status that caused us to need recovery. */
{
    register Fs_HandleHeader *hdrPtr = streamPtr->ioHandlePtr;
    register ReturnStatus status;

    Fsutil_WantRecovery(hdrPtr);
    if (flags & FS_NON_BLOCKING) {
	status = SUCCESS;
    } else {
	status = Fsutil_WaitForRecovery(hdrPtr, rpcStatus);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_WantRecovery --
 *
 *	This routine is called when an error has occurred trying to
 *	contact the file's IO server indicating that recovery actions
 *	should take place when the server comes back.  We depend on
 *	recovery call-backs already being installed as we don't even
 *	mention our need to the recovery module here.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Marks the handle as needing recovery.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Fsutil_WantRecovery(hdrPtr)
    Fs_HandleHeader *hdrPtr;	/* Handle needing recovery. The handle should
				 * start with a FsRemoteHandle struct. */
{
    register Fsutil_RecoveryInfo *recovPtr = &((Fsrmt_IOHandle *)hdrPtr)->recovery;
    if (!RemoteHandle(hdrPtr)) {
	printf( "Fsutil_WantRecovery: no recovery for %s handles\n",
		Fsutil_FileTypeToString(hdrPtr->fileID.type));
    } else {
	/*
	 * The monitor lock is embedded in RemoteIOHandles so we can
	 * only lock/unlock with the right kind of handle.
	 */
	LOCK_MONITOR;
	fs_Stats.recovery.wants++;
	recovPtr->flags |= RECOVERY_NEEDED;
	UNLOCK_MONITOR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_AttemptRecovery --
 *
 *	Attempt recovery if the server appears up.  This is called
 *	(in the background) by the block cleaner if it gets a stale
 *	handle error on a write-back.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invokes the reopen procedure if the server responds to an RPC.
 *
 *----------------------------------------------------------------------
 */
void
Fsutil_AttemptRecovery(data, callInfoPtr)
    ClientData		data;		/* Ref to Fs_HandleHeader */
    Proc_CallInfo	*callInfoPtr;
{
    register Fs_HandleHeader *hdrPtr =
	    (Fs_HandleHeader *)data;

    if (!RemoteHandle(hdrPtr)) {
	/*
	 * We get called on a local naming request-response stream after the
	 * pseudo-filesystem server crashes.  There is no recovery possible
	 * so we just return an error and the naming operation fails.
	 */
	printf( "Fsutil_AttemptRecovery, no recovery for type: %s\n",
		Fsutil_FileTypeToString(hdrPtr->fileID.type));
	return;
    }
    if (!Recov_IsHostDown(hdrPtr->fileID.serverID)) {
	Fsutil_Reopen(hdrPtr->fileID.serverID, (ClientData)NIL);
    }
    callInfoPtr->interval = 0;	/* no more callbacks, please */
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_WaitForRecovery --
 *
 *	Wait until recovery actions have completed for the stream.
 *	This will return failure codes if the recovery aborts.
 *	The wait is interruptable by a signal so the user can abort.
 *
 * Results:
 *	SUCCESS unless there was a recovery error or a signal came in.
 *
 * Side effects:
 *	Block the process.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsutil_WaitForRecovery(hdrPtr, rpcStatus)
    register Fs_HandleHeader	*hdrPtr;
    ReturnStatus		rpcStatus;	/* Status that caused us to
						 * need recovery. */
{
    ReturnStatus		status = SUCCESS;

    if (!RemoteHandle(hdrPtr)) {
	/*
	 * We get called on a local naming request-response stream after the
	 * pseudo-filesystem server crashes.  There is no recovery possible
	 * so we just return an error and the naming operation fails.
	 */
	printf( "Fsutil_WaitForRecovery, no recovery for type: %s\n",
		Fsutil_FileTypeToString(hdrPtr->fileID.type));
	return(FAILURE);
    } else if (!Fsutil_HandleValid(hdrPtr)) {
	/*
	 * Handle has already failed recovery.
	 */
	return(FAILURE);
    }
    /*
     * If our caller got a stale handle then the server is probably up
     * we try to re-establish state with the server now.  Otherwise
     * we depend on a reboot callback to invoke Fsutil_Reopen.
     */
    if (rpcStatus == FS_STALE_HANDLE) {
	Fsutil_FileError(hdrPtr, "", rpcStatus);
	if (!Recov_IsHostDown(hdrPtr->fileID.serverID)) {
	    Fsutil_Reopen(hdrPtr->fileID.serverID, (ClientData)NIL);
	}
    }
    status = RecoveryWait(&((Fsrmt_IOHandle *)hdrPtr)->recovery);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * RecoveryWait --
 *
 *	Wait until recovery actions have completed for the stream.
 *	This will return failure codes if the recovery aborts.
 *
 * Results:
 *	SUCCESS unless there was a recovery error.
 *
 * Side effects:
 *	Block the process.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
RecoveryWait(recovPtr)
    Fsutil_RecoveryInfo *recovPtr;
{
    register ReturnStatus status = SUCCESS;
    LOCK_MONITOR;
    while (recovPtr->flags & RECOVERY_NEEDED) {
	if (Sync_Wait(&recovPtr->reopenComplete, TRUE)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    fs_Stats.recovery.waitAbort++;
	    break;
	}
    }
    if (recovPtr->flags & RECOVERY_FAILED) {
	status = recovPtr->status;
	fs_Stats.recovery.waitFailed++;
    } else if (status == SUCCESS) {
	fs_Stats.recovery.waitOK++;
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------------
 *
 * RecoveryComplete --
 *
 *	Mark a remote handle as having competed recovery actions.  The waiting
 *	process is not woken up yet, however, because it may also depend
 *	on recovery of the shadow streams kept on the server.  FsHandleReopen
 *	will call RecoveryNotify to poke waiters after all handles have been
 *	recovered.  Note that this procedure won't mark the handle as
 *	having completed recovery if the error status indicates a timeout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The handle is marked as having competed recovery if the error
 *	status does not indicate another communcation fail with the server.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY static void
RecoveryComplete(recovPtr, status)
    Fsutil_RecoveryInfo *recovPtr;
    ReturnStatus status;
{
    LOCK_MONITOR;

    recovPtr->status = status;
    switch(status) {
	case RPC_TIMEOUT:
	case RPC_SERVICE_DISABLED:
	    fs_Stats.recovery.timeout++;
	    break;
	case FS_FILE_REMOVED:
	    recovPtr->flags |= RECOVERY_FAILED|RECOVERY_COMPLETE;
	    fs_Stats.recovery.deleted++;
	    break;
	default:
	    recovPtr->flags |= RECOVERY_FAILED|RECOVERY_COMPLETE;
	    fs_Stats.recovery.failed++;
	    break;
	 case SUCCESS:
	    recovPtr->flags |= RECOVERY_COMPLETE;
	    fs_Stats.recovery.succeeded++;
	    break;
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * RecoveryNotify --
 *
 *	Wakeup processes waiting on recovery for a handle, if appropriate.
 *	This is called from FsReopenHandle after all handles, both I/O
 *	handles and top-level stream, have been recovered.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Processes waiting on the handle are awakened and the RECOVERY_NEEDED
 *	flags is cleared from the recovery flags.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY static void
RecoveryNotify(recovPtr)
    register Fsutil_RecoveryInfo *recovPtr;
{
    LOCK_MONITOR;

    if (recovPtr->flags & RECOVERY_COMPLETE) {
	recovPtr->flags &= ~(RECOVERY_COMPLETE|RECOVERY_NEEDED);
	Sync_Broadcast(&recovPtr->reopenComplete);
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_RecoveryNeeded --
 *
 *	Returns TRUE if recovery is pending for the handle.  This is
 *	called when scavenging a remote file handle to make sure noone
 *	is waiting for recovery on the handle.
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY Boolean
Fsutil_RecoveryNeeded(recovPtr)
    Fsutil_RecoveryInfo *recovPtr;
{
    register Boolean recovWanted;
    LOCK_MONITOR;
    recovWanted = (recovPtr->flags & RECOVERY_NEEDED);
    UNLOCK_MONITOR;
    return(recovWanted);
}

/*
 *----------------------------------------------------------------------
 *
 * RecoveryFailed --
 *
 *	Returns TRUE if recovery has failed on the handle.  This is
 *	called when recovering streams to see if the I/O handle is ok.
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY static Boolean
RecoveryFailed(recovPtr)
    Fsutil_RecoveryInfo *recovPtr;
{
    register Boolean recovFailed;
    LOCK_MONITOR;
    recovFailed = (recovPtr->flags & RECOVERY_FAILED);
    UNLOCK_MONITOR;
    return(recovFailed);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsRemoteHandleScavange --
 *
 *	Scavenging routine for remote handles, not including remote
 *	file handles. This nukes the handle if there are no uses of
 *	it and it doesn't need recovery.
 *
 * Results:
 *	TRUE if the handle was removed.
 *
 * Side effects:
 *	Either removes or unlocks the handle.
 *
 *----------------------------------------------------------------------------
 *
 */
Boolean
Fsutil_RemoteHandleScavenge(hdrPtr)
    Fs_HandleHeader *hdrPtr;
{
    if (OkToScavenge(&((Fsrmt_IOHandle *)hdrPtr)->recovery)) {
	Fsutil_RecoverySyncLockCleanup(&((Fsrmt_IOHandle *)hdrPtr)->recovery);
	Fsutil_HandleRemove(hdrPtr);
	return(TRUE);
    } else {
	Fsutil_HandleUnlock(hdrPtr);
	return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * OkToScavenge --
 *
 *	Internal routine to check monitored state.  This returns TRUE if
 *	the handle should be scavenged, in which case our caller can
 *	remove the handle, or FALSE, in which our caller should just
 *	unlock it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 *
 */
static ENTRY Boolean
OkToScavenge(recovPtr)
    register Fsutil_RecoveryInfo *recovPtr;
{
    register Boolean okToScavenge = FALSE;
    LOCK_MONITOR;
    if (recovPtr->use.ref == 0 &&
	(recovPtr->flags & RECOVERY_NEEDED) == 0) {
	okToScavenge = TRUE;
    }
    UNLOCK_MONITOR;
    return(okToScavenge);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_ClientCrashed --
 *
 *	Clean up state after a host goes down.  Mainly we want to close
 *	files opened on that client.  We also take care to reset various
 *	watchdogs like the bit that turns off opens until re-opens are done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up references the client had to files.  Clears the
 *	CLT_RECOV_IN_PROGRESS bit from the client recovery state.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Fsutil_ClientCrashed(spriteID, clientData)
    int spriteID;		/* Client that crashed */
    ClientData clientData;	/* IGNORED */
{
    fs_Stats.recovery.clientCrashed++;
    /*
     * Reset the 'recovery in progress' bit (it would be set if the
     * client crashed during recovery) so the client can open files
     * the next time it boots.
     */
    Recov_ClearClientState(spriteID, CLT_RECOV_IN_PROGRESS);
    /*
     * Clean up references to our files.
     */
    Fsutil_RemoveClient(spriteID);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_RemoveClient --
 *
 *	Go through all of the handles and delete all references to the
 *	handle for the client.  This is to be called when a client reboots.
 *	The clientKill procedure for each I/O handle type must either
 *	unlock the handle or remove it after cleaning up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the stream-type-specific clientKill procedures.
 *
 *----------------------------------------------------------------------------
 *
 */

void
Fsutil_RemoveClient(clientID)
    int		clientID;	/* The client to remove the files for. */
{
    Hash_Search			hashSearch;
    register	Fs_HandleHeader	*hdrPtr;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {
	(*fsio_StreamOpTable[hdrPtr->fileID.type].clientKill)(hdrPtr, clientID);
    }
}


#ifdef not_used

/*
 *----------------------------------------------------------------------
 *
 * FsRecoveryStarting --
 *
 *	This tells the server that we are starting recovery so that it
 *	can prevent open requests by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An RPC to the server.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsRecoveryStarting(serverID)
    int serverID;		/* Server we are recovering with */
{
    Rpc_Storage storage;
    int flags = CLT_RECOV_IN_PROGRESS;

    storage.requestParamPtr = (Address)&flags;
    storage.requestParamSize = sizeof(int);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(serverID, RPC_FS_RECOVERY, &storage);
    if (status != SUCCESS) {
	printf( "FsRecoveryDone: got status %x\n", status);
    }
}
#endif not_used

/*
 *----------------------------------------------------------------------
 *
 * RecoveryDone --
 *
 *	This tells the server that we are done with recovery so that it
 *	will start handling open requests from us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An RPC to the server.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
RecoveryDone(serverID)
    int serverID;		/* Server we are recovering with */
{
    Rpc_Storage storage;
    ReturnStatus status;
    int flags = 0;

    storage.requestParamPtr = (Address)&flags;
    storage.requestParamSize = sizeof(int);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(serverID, RPC_FS_RECOVERY, &storage);
    if (status != SUCCESS) {
	printf( "RecoveryDone: got status %x\n", status);
    } else {
	fs_Stats.recovery.number++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_RpcRecovery --
 *
 *	Rpc server stub for RPC_FS_RECOVERY RPC.  The client us
 *	before and after it is re-opening its files.  This lets us
 *	block out regular open requests until the re-opening is done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Maintains the "client is recovering" state bit.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsutil_RpcRecovery(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;	/* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    int *flagsPtr = (int *)storagePtr->requestParamPtr;
    if (*flagsPtr & CLT_RECOV_IN_PROGRESS) {
	if ((Recov_GetClientState(clientID) & CLT_RECOV_IN_PROGRESS) == 0) {
	    fsutil_NumRecovering++;
	    if (fsutil_NumRecovering == 1) {
		Net_HostPrint(clientID, "initiated client recovery\n");
	    }
	    Recov_SetClientState(clientID, CLT_RECOV_IN_PROGRESS);
	}
	fs_Stats.recovery.clientRecovered++;
    } else {
	Recov_ClearClientState(clientID, CLT_RECOV_IN_PROGRESS);
	fsutil_NumRecovering--;
	if (fsutil_NumRecovering == 0) {
	    Net_HostPrint(clientID, "completed client recovery\n");
	}
    }
    Rpc_Reply(srvToken, SUCCESS, storagePtr, (int(*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsutil_FsRecovInfo --
 *
 *	Info, including file names, about recovery and file status for testing.
 *
 * Results:
 *	SUCCESS or not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsutil_FsRecovInfo(length, resultPtr, lengthNeededPtr)
    int				length;		/* size of data buffer */
    Fsutil_FsRecovNamedStats	*resultPtr;	/* data buffer */
    int				*lengthNeededPtr;
{
    Hash_Search			hashSearch;
    Fs_HandleHeader		*hdrPtr;
    Fsutil_FsRecovNamedStats	*infoPtr;
    int				numNeeded;
    int				numAvail;
    Fs_HandleHeader		*testHandlePtr;

    if (resultPtr != (Fsutil_FsRecovNamedStats *) NIL) {
	bzero((char *) resultPtr, length);
    }
    numNeeded = 0;
    numAvail = length / sizeof (Fsutil_FsRecovNamedStats);

    infoPtr = (Fsutil_FsRecovNamedStats *) resultPtr;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {

	numNeeded++;
	if (numNeeded > numAvail) {
	    Fsutil_HandleUnlock(hdrPtr);
	    continue;
	}
	if (hdrPtr->fileID.type == FSIO_STREAM) {
	    Fs_Stream	*streamPtr;

	    streamPtr = (Fs_Stream *) hdrPtr;

	    infoPtr->streamRefCount = hdrPtr->refCount;
	    infoPtr->mode = streamPtr->flags;
	    infoPtr->streamHandle = TRUE;

	    testHandlePtr = streamPtr->ioHandlePtr;
	} else {
	    infoPtr->streamHandle = FALSE;
	    testHandlePtr = hdrPtr;
	}

	if (testHandlePtr != (Fs_HandleHeader *) NIL) {
	    infoPtr->fileID = testHandlePtr->fileID;

	    if (fsio_StreamRecovTestFuncs[testHandlePtr->fileID.type].refFunc
		    != (int ((*)())) NIL) {
		infoPtr->refCount =
		    (*(fsio_StreamRecovTestFuncs[
		    testHandlePtr->fileID.type]).refFunc) (testHandlePtr);
	    }
	    if (fsio_StreamRecovTestFuncs[
		    testHandlePtr->fileID.type].numBlocksFunc !=
		    (int ((*)())) NIL) {
		infoPtr->numBlocks =
			(*(fsio_StreamRecovTestFuncs[
			testHandlePtr->fileID.type]).numBlocksFunc)
			(testHandlePtr);
	    }
	    if (fsio_StreamRecovTestFuncs[
		    testHandlePtr->fileID.type].numDirtyBlocksFunc
		    != (int ((*)())) NIL) {
		infoPtr->numDirtyBlocks =
			(*(fsio_StreamRecovTestFuncs[
			testHandlePtr->fileID.type]).numDirtyBlocksFunc)
			(testHandlePtr);
	    }
	    (void)strncpy(infoPtr->name, Fsutil_HandleName(testHandlePtr), 49);
	    infoPtr->name[50] = '\0';
	}
	infoPtr++;
	Fsutil_HandleUnlock(hdrPtr);
    }
    *lengthNeededPtr = numNeeded * sizeof (Fsutil_FsRecovNamedStats);

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_TestForHandles --
 *
 *	Called to see if we still have handles for the given serverID.
 *
 * Results:
 *	Number of file and device handles for server in question.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
int
Fsutil_TestForHandles(serverID)
    int		serverID;	/* Server we're interested in. */
{
    Hash_Search			hashSearch;
    register	Fs_HandleHeader	*hdrPtr;
    int				count = 0;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = Fsutil_GetNextHandle(&hashSearch);
	 hdrPtr != (Fs_HandleHeader *) NIL;
         hdrPtr = Fsutil_GetNextHandle(&hashSearch)) {
	 if (hdrPtr->fileID.serverID == serverID) {
	    switch(hdrPtr->fileID.type) {
/*
	    case FSIO_RMT_PSEUDO_STREAM:
	    case FSIO_STREAM:
*/
	    case FSIO_RMT_FILE_STREAM:
	    case FSIO_RMT_DEVICE_STREAM:
		count++;
		break;
	    default:
		break;
	    }
	}
	Fsutil_HandleUnlock(hdrPtr);
    }
    return count;
}

/* Number of handles to be put together. */
int			numBulkHandles = 0;
/* Current handle we're filling in. */
int			nextHandleIndex = 0;
/* Array of handle reopen information, etc. */
static Fsutil_BulkHandle	*bulkHandleSpace = (Fsutil_BulkHandle *) NIL;
/* Array of info returned from server for bulk reopen. */
static Fsutil_BulkReturn	*bulkReturnSpace = (Fsutil_BulkReturn *) NIL;
static Fsutil_BulkReopenOps	bulkReopenOps[FSIO_NUM_STREAM_TYPES];


/*
 *----------------------------------------------------------------------------
 *
 * InitReopenHandles --
 *
 *	Initialize memory for storing future reopen rpc info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *----------------------------------------------------------------------------
 */
static void
InitReopenHandles()
{
    int 	i;

    if (numBulkHandles < fs_Stats.handle.maxNumber) {
	if (numBulkHandles > 0) {
	    free((Address) bulkHandleSpace);
	}
	numBulkHandles = fs_Stats.handle.maxNumber;
	bulkHandleSpace = (Fsutil_BulkHandle *)
		malloc(numBulkHandles * sizeof (Fsutil_BulkHandle));
	bulkReturnSpace = (Fsutil_BulkReturn *)
		malloc(numBulkHandles * sizeof (Fsutil_BulkReturn));
    }
    /*
     * Initialize all to be "empty."
     */
    for (i = 0; i < numBulkHandles; i++) {
	bulkHandleSpace[i].type = -1;
    }
    nextHandleIndex = 0;
    return;
}

int	reopensSkipped;

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_InitBulkReopenOps --
 *
 *	Copy a handle type's reopen ops to the array.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
void
Fsutil_InitBulkReopenOps(type, reopenOpsPtr)
    int				type;
    Fsutil_BulkReopenOps	*reopenOpsPtr;
{
    if (type >= FSIO_NUM_STREAM_TYPES) {
	panic("Fsutil_InitBulkReopenOps: bad handle type.");
    }
    bulkReopenOps[type].setup = reopenOpsPtr->setup;
    bulkReopenOps[type].finish = reopenOpsPtr->finish;
    reopensSkipped = 0;

    return;
}


/*
 *----------------------------------------------------------------------------
 *
 * AddReopenHandle --
 *
 *	Add a handle to the future reopen rpc info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
static ReturnStatus
AddReopenHandle(hdrPtr)
    Fs_HandleHeader	*hdrPtr;
{
    ReturnStatus	status;

    status = (*bulkReopenOps[hdrPtr->fileID.type].setup) (hdrPtr,
	    &(bulkHandleSpace[nextHandleIndex].reopenParams));
    if (status == SUCCESS) {
	bulkHandleSpace[nextHandleIndex].type = hdrPtr->fileID.type;
	bulkHandleSpace[nextHandleIndex].serverID = hdrPtr->fileID.serverID;
	bulkHandleSpace[nextHandleIndex].hdrPtr = hdrPtr;
	nextHandleIndex++;
	return SUCCESS;
    }

    if (hdrPtr->fileID.type != FSIO_STREAM) {
	/*
	 * NO_REFERENCE means that there is no current use of the handle,
	 * so it shouldn't be reopened, but there was no failure either, so
	 * don't mark it as a failed reopen.  FS_RECOV_SKIP means it
	 * was a handle with no references but with some clean blocks in
	 * the cache and we're currently skipping reopens on those handles.
	 */
	if (status == FS_RECOV_SKIP) {
	    RecoveryComplete(&(((Fsrmt_IOHandle *)hdrPtr)->recovery), SUCCESS);
	    reopensSkipped++;
	    return status;
	}
	if (status == FS_NO_REFERENCE) {
	    status = SUCCESS;
	}
	RecoveryComplete(&(((Fsrmt_IOHandle *)hdrPtr)->recovery), status);
    }
    return status;
}

/*
 *----------------------------------------------------------------------------
 *
 * FinishReopenHandles --
 *
 *	Clean up, mark done with recovery.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
static void
FinishReopenHandles()
{
    int		i;
    Fsutil_BulkHandle	*bulkHandlePtr;
    Fsutil_BulkReturn	*returnInfoPtr;
    Fs_HandleHeader	*hdrPtr;
    ReturnStatus	status;

    for (i = 0; i < nextHandleIndex; i++) {
	bulkHandlePtr = &(bulkHandleSpace[i]);
	returnInfoPtr = &(bulkReturnSpace[i]);
	status = returnInfoPtr->status;
	/*
	 * If there was a bad return status, it's only safe to get header
	 * info, etc., from the handle space and not the return info, because
	 * the return info may not have been filled in on the server if
	 * there was an error.
	 */
	hdrPtr = bulkHandlePtr->hdrPtr;

	/* Does server recognize bulk rpc? */
	if (status == RPC_INVALID_RPC) {
	    Fsutil_HandleUnlock(hdrPtr);
	    continue;
	}

	(*bulkReopenOps[bulkHandlePtr->type].finish)
		(bulkHandlePtr->hdrPtr, &(returnInfoPtr->state),
		returnInfoPtr->status);
	if (bulkHandlePtr->type != FSIO_STREAM) {
	    RecoveryComplete(&(((Fsrmt_IOHandle *)hdrPtr)->recovery), status);
	}

	if (bulkHandlePtr->type == FSIO_STREAM && status != SUCCESS) {
	    if (status != RPC_TIMEOUT && status != RPC_SERVICE_DISABLED) {
		Fsutil_HandleInvalidate(hdrPtr);
	    }
	}
		
	/*
	 * If we removed the handle because it was no longer needed,
	 * we can't try unlocking it.  Otherwise, unlock all I/O handles.
	 */
	if (status != FS_NO_HANDLE) {
	    Fsutil_HandleUnlock(hdrPtr);
	}
    }
    printf("Reopens skipped: %d\n", reopensSkipped);
    return;
}

/*
 *----------------------------------------------------------------------------
 *
 * DoBulkReopen --
 *
 *	Do an RPC to send the collected reopen handles.
 *
 * Results:
 *	Status is SUCCESS unless there's an rpc/network-type failure.
 *
 * Side effects:
 *	RPC.  Status for each handle gets filled in.
 *
 *----------------------------------------------------------------------------
 */
static ReturnStatus
DoBulkReopen(serverID)
    int			serverID;
{
    int			inSize;
    int			outSize;
    int			i;
    Fs_HandleHeader	*hdrPtr;
    ReturnStatus	status;
    int			numSent;
    int			numSending;
    int			maxSendHandles;

    /*
     * Unlock all the handles first.  Relock them afterwards.  (In case
     * the server asks us to do an invalidate or write-back.
     */
    if (nextHandleIndex <= 0) {
	return SUCCESS;
    }
    for (i = 0; i < nextHandleIndex; i++) {
	hdrPtr = bulkHandleSpace[i].hdrPtr;
	Fsutil_HandleUnlock(hdrPtr);
    }

    /*
     * How many handles can we send in one RPC?  We only want to do our
     * own fragmentation when sending, so if the return information is
     * larger, adjust for that on this end.
     */
    if (sizeof (Fsutil_BulkHandle) < sizeof (Fsutil_BulkReturn)) {
	maxSendHandles = RPC_MAX_DATASIZE / sizeof (Fsutil_BulkReturn);
    } else {
	maxSendHandles = RPC_MAX_DATASIZE / sizeof (Fsutil_BulkHandle);
    }
    numSent = 0;
    do {
	numSending = nextHandleIndex - numSent;
	if (numSending > maxSendHandles) {
	    numSending = maxSendHandles;
	} 
	inSize = numSending *  sizeof (Fsutil_BulkHandle);
	outSize = numSending * sizeof (Fsutil_BulkReturn);
	status = FsrmtBulkReopen(serverID, inSize,
		(Address) &(bulkHandleSpace[numSent]), &outSize,
		(Address) &(bulkReturnSpace[numSent]));
	if (status != SUCCESS) {
	    break;
	}
	numSent += numSending;
    } while (numSent < nextHandleIndex);

    for (i = 0; i < nextHandleIndex; i++) {
	hdrPtr = bulkHandleSpace[i].hdrPtr;
	Fsutil_HandleLock(hdrPtr);
	if (status != SUCCESS) {
	    bulkReturnSpace[i].status = status;
	}
    }
    /*
     * A bad status will only be something like timeout or other rpc-type
     * failure.
     */
    return status;
}
