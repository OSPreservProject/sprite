/* 
 * fsRecovery.c
 *
 *	Routines for filesytem recovery.  A file server keeps state about
 *	client use, and this needs to be recovered after the server reboots.
 *	The first routine, FsReopen, goes through a sequence of steps that
 *	a client makes in order to help the server in this way.  Other
 *	routines are used by other parts of the filesystem to wait for
 *	recovery to happen, and are typically invoked after a remote
 *	operation fails due to a communication failure.  These are
 *	FsWantRecovery, and FsWaitForRecovery.  The routine Fs_WaitForHost
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

#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsRecovery.h"
#include "fsPrefix.h"
#include "fsOpTable.h"
#include "fsStat.h"
#include "recov.h"
#include "hash.h"
#include "rpc.h"
#include "vm.h"

#ifndef sun4
static void		ReopenHandles();
static void		RecoveryDone();
static Boolean		RecoveryFailed();
static void		RecoveryNotify();
#else
/*
 * The sun4 compiler doesn't seem to allow this sort of redeclaration.
 */
extern void		ReopenHandles();
extern void		RecoveryDone();
extern Boolean		RecoveryFailed();
extern void		RecoveryNotify();
#endif sun4
static void		RecoveryComplete();
static Boolean		OkToScavenge();

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
 *----------------------------------------------------------------------
 *
 * FsReopen --
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
FsReopen(serverID, clientData)
    int serverID;		/* Server we are recovering with */
    ClientData clientData;	/* IGNORED */
{
    /*
     * Ensure only one instance of FsReopen by doing a set-and-test.
     */
    if (Recov_SetClientState(serverID, SRV_RECOV_IN_PROGRESS)
	    & SRV_RECOV_IN_PROGRESS) {
	return;
    }
    /*
     * Recover the prefix table.
     */
    FsPrefixReopen(serverID);
    /*
     * Wait for opens in progress, then block opens.
     */
    FsPrefixRecoveryCheck(serverID);
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
    FsPrefixAllowOpens(serverID);
    /*
     * Clear the recovery bit before kicking processes.  Some processes
     * may be locked down doing pseudo-device request/response, and the
     * proc wakeup call will block on them.  To prevent deadlock we
     * have to mark recovery as complete first.
     */
    Recov_ClearClientState(serverID, SRV_RECOV_IN_PROGRESS);
    /*
     * Kick all processes in case any are blocking on I/O
     */
    Proc_WakeupAllProcesses();
    /*
     * Tell VM that we have recovered in case this was the swap server.
     */
    Vm_Recovery();
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

void
ReopenHandles(serverID)
    int		serverID;	/* The to re-establish contact with. */
{
    Hash_Search			hashSearch;
    register	FsHandleHeader	*hdrPtr;
    register	Fs_Stream	*streamPtr;
    register	FsRemoteIOHandle *rmtHandlePtr;
    ReturnStatus		status = SUCCESS;
    Boolean			printed = FALSE;
    int				succeeded = fsStats.recovery.succeeded;
    int				failed = fsStats.recovery.failed;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	 if ((hdrPtr->fileID.type != FS_STREAM) &&
		 (hdrPtr->fileID.serverID == serverID)) {
	    if (!RemoteHandle(hdrPtr)) {
		panic("ReopenHandles, local I/O handle at remote server?\n");
	    }
	    if (!printed) {
		Net_HostPrint(serverID, "- recovering handles\n");
		printed = TRUE;
	    }
	    status = (*fsStreamOpTable[hdrPtr->fileID.type].reopen)(hdrPtr,
		rpc_SpriteID, (ClientData)NIL, (int *)NIL, (ClientData *)NIL);
	    RecoveryComplete(&(((FsRemoteIOHandle *)hdrPtr)->recovery),
				status);
	    FsHandleUnlock(hdrPtr);
	    switch (status) {
		case SUCCESS:
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
		    FsFileError(hdrPtr, "Reopen failed ", status);
		    break;
	    }
	} else {
	    FsHandleUnlock(hdrPtr);
	}
    }
    /*
     * Now go through and recover streams, once we've gotten the regular
     * I/O handles re-opened.  This ensures that the I/O handle will be
     * around on the server when it re-creates our streams.  If recovery
     * fails on a stream we invalidate its handle, but don't remove it
     * because that happens in the top-level close routine.
     */
    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	if ((hdrPtr->fileID.type == FS_STREAM) &&
		 (hdrPtr->fileID.serverID == serverID)) {
	    streamPtr = (Fs_Stream *)hdrPtr;
	    rmtHandlePtr = (FsRemoteIOHandle *)streamPtr->ioHandlePtr;

	    if (rmtHandlePtr == (FsRemoteIOHandle *)NIL) {
		FsFileError((FsHandleHeader *)streamPtr,
			"ReopenHandles: NIL I/O handle", 0);
	    } else if (!RemoteHandle((FsHandleHeader *)rmtHandlePtr)) {
		panic( "ReopenHandles: local I/O handle for remote stream?\n");
	    } else if (RecoveryFailed(&rmtHandlePtr->recovery)) {
		FsHandleInvalidate((FsHandleHeader *)streamPtr);
	    } else {
		status = FsStreamReopen((FsHandleHeader *)streamPtr,
				rpc_SpriteID, (ClientData)NIL, (int *)NIL,
				(ClientData *)NIL);
		if (status != SUCCESS) {
		    FsFileError((FsHandleHeader *)streamPtr,
			"Reopen failed", status);
		    FsFileError(streamPtr->ioHandlePtr,"I/O handle",
				SUCCESS);
		    if ((status == RPC_TIMEOUT) ||
			(status == RPC_SERVICE_DISABLED)) {
			FsHandleUnlock(streamPtr);
			goto reopenReturn;
		    }
		    FsHandleInvalidate((FsHandleHeader *)streamPtr);
		}
	    }
	}
	FsHandleUnlock(hdrPtr);
    }
    /*
     * Now we notify processes waiting on I/O handles, and invalidate
     * those I/O handles which failed recovery.
     */
    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	 if (!RemoteHandle(hdrPtr)) {
	     FsHandleUnlock(hdrPtr);
	 } else {
	     RecoveryNotify(&((FsRemoteIOHandle *)hdrPtr)->recovery);
	     if (RecoveryFailed(&((FsRemoteIOHandle *)hdrPtr)->recovery)) {
		 FsHandleInvalidate(hdrPtr);
	     }
	     FsHandleUnlock(hdrPtr);
	 }
    }
reopenReturn:
    if (status != SUCCESS) {
	Net_HostPrint(serverID, "Recovery failed");
	printf(" <%x>\n", status);
    } else if (printed) {
	Net_HostPrint(serverID, "Recovery complete");
	printf(" %d handles reopened", fsStats.recovery.succeeded - succeeded);
	if (fsStats.recovery.failed - failed > 0) {
	    printf(" %d failed reopens", fsStats.recovery.failed - failed);
	}
	printf("\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsRecoveryInit --
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
FsRecoveryInit(recovPtr)
    register FsRecoveryInfo	*recovPtr;	/* Recovery state */
{
    bzero((Address) recovPtr, sizeof(FsRecoveryInfo));
    Sync_LockInitDynamic(&recovPtr->lock, "fs:recoveryLock");
}

/*
 *----------------------------------------------------------------------
 *
 * FsRecoverySyncLockCleanup --
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
void
FsRecoverySyncLockCleanup(recovPtr)
    register FsRecoveryInfo	*recovPtr;	/* Recovery state */
{
    Sync_LockClear(&recovPtr->lock);
}

/*
 *----------------------------------------------------------------------
 *
 * RemoteHandle --
 *
 *	This checks the type of a handle to see if it is remote and thus
 *	has a FsRemoteIOHandle structure embedded into it.  Only these
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
    FsHandleHeader *hdrPtr;
{
    switch(hdrPtr->fileID.type) {
	case FS_RMT_FILE_STREAM:
	case FS_RMT_DEVICE_STREAM:
	case FS_RMT_PIPE_STREAM:
	case FS_RMT_PSEUDO_STREAM:
	case FS_PFS_NAMING_STREAM:
	case FS_RMT_PFS_STREAM:
	case FS_CONTROL_STREAM:
	case FS_PFS_CONTROL_STREAM:
	    return(TRUE);
	default:
	    return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_WaitForHost --
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
Fs_WaitForHost(streamPtr, flags, rpcStatus)
    Fs_Stream	*streamPtr;
    int		flags;		/* 0 or FS_NON_BLOCKING */
    ReturnStatus rpcStatus;	/* Status that caused us to need recovery. */
{
    register FsHandleHeader *hdrPtr = streamPtr->ioHandlePtr;
    register ReturnStatus status;

    FsWantRecovery(hdrPtr);
    if (flags & FS_NON_BLOCKING) {
	status = SUCCESS;
    } else {
	status = FsWaitForRecovery(hdrPtr, rpcStatus);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsWantRecovery --
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
FsWantRecovery(hdrPtr)
    FsHandleHeader *hdrPtr;	/* Handle needing recovery. The handle should
				 * start with a FsRemoteHandle struct. */
{
    register FsRecoveryInfo *recovPtr = &((FsRemoteIOHandle *)hdrPtr)->recovery;
    if (!RemoteHandle(hdrPtr)) {
	printf( "FsWantRecovery: no recovery for %s handles\n",
		FsFileTypeToString(hdrPtr->fileID.type));
    } else {
	/*
	 * The monitor lock is embedded in RemoteIOHandles so we can
	 * only lock/unlock with the right kind of handle.
	 */
	LOCK_MONITOR;
	fsStats.recovery.wants++;
	recovPtr->flags |= RECOVERY_NEEDED;
	UNLOCK_MONITOR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsWaitForRecovery --
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
FsWaitForRecovery(hdrPtr, rpcStatus)
    register FsHandleHeader	*hdrPtr;
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
	printf( "FsWaitForRecovery, no recovery for type: %s\n",
		FsFileTypeToString(hdrPtr->fileID.type));
	return(FAILURE);
    } else if (!FsHandleValid(hdrPtr)) {
	/*
	 * Handle has already failed recovery.
	 */
	return(FAILURE);
    }
    /*
     * If our caller got a stale handle then the server is probably up
     * we try to re-establish state with the server now.  Otherwise
     * we depend on a reboot callback to invoke FsReopen.
     */
    if (rpcStatus == FS_STALE_HANDLE) {
	FsFileError(hdrPtr, "", rpcStatus);
	if (!Recov_IsHostDown(hdrPtr->fileID.serverID)) {
	    FsReopen(hdrPtr->fileID.serverID, (ClientData)NIL);
	}
    }
    status = RecoveryWait(&((FsRemoteIOHandle *)hdrPtr)->recovery);
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
    FsRecoveryInfo *recovPtr;
{
    register ReturnStatus status = SUCCESS;
    LOCK_MONITOR;
    while (recovPtr->flags & RECOVERY_NEEDED) {
	if (Sync_Wait(&recovPtr->reopenComplete, TRUE)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    fsStats.recovery.waitAbort++;
	    break;
	}
    }
    if (recovPtr->flags & RECOVERY_FAILED) {
	status = recovPtr->status;
	fsStats.recovery.waitFailed++;
    } else if (status == SUCCESS) {
	fsStats.recovery.waitOK++;
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
    FsRecoveryInfo *recovPtr;
    ReturnStatus status;
{
    LOCK_MONITOR;

    recovPtr->status = status;
    switch(status) {
	case RPC_TIMEOUT:
	case RPC_SERVICE_DISABLED:
	    fsStats.recovery.timeout++;
	    break;
	case FS_DOMAIN_UNAVAILABLE:
	    fsStats.recovery.failed++;
	    break;
	default:
	    recovPtr->flags |= RECOVERY_FAILED|RECOVERY_COMPLETE;
	    fsStats.recovery.failed++;
	    break;
	 case SUCCESS:
	    recovPtr->flags |= RECOVERY_COMPLETE;
	    fsStats.recovery.succeeded++;
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
ENTRY void
RecoveryNotify(recovPtr)
    register FsRecoveryInfo *recovPtr;
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
 * FsRecoveryNeeded --
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
FsRecoveryNeeded(recovPtr)
    FsRecoveryInfo *recovPtr;
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
ENTRY Boolean
RecoveryFailed(recovPtr)
    FsRecoveryInfo *recovPtr;
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
FsRemoteHandleScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;
{
    if (OkToScavenge(&((FsRemoteIOHandle *)hdrPtr)->recovery)) {
	FsRecoverySyncLockCleanup(&((FsRemoteIOHandle *)hdrPtr)->recovery);
	FsHandleRemove(hdrPtr);
	return(TRUE);
    } else {
	FsHandleUnlock(hdrPtr);
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
    register FsRecoveryInfo *recovPtr;
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
 * FsClientCrashed --
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
FsClientCrashed(spriteID, clientData)
    int spriteID;		/* Client that crashed */
    ClientData clientData;	/* IGNORED */
{
    fsStats.recovery.clientCrashed++;
    /*
     * Reset the 'recovery in progress' bit (it would be set if the
     * client crashed during recovery) so the client can open files
     * the next time it boots.
     */
    Recov_ClearClientState(spriteID, CLT_RECOV_IN_PROGRESS);
    /*
     * Clean up references to our files.
     */
    FsRemoveClient(spriteID);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsRemoveClient --
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
FsRemoveClient(clientID)
    int		clientID;	/* The client to remove the files for. */
{
    Hash_Search			hashSearch;
    register	FsHandleHeader	*hdrPtr;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	(*fsStreamOpTable[hdrPtr->fileID.type].clientKill)(hdrPtr, clientID);
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
ENTRY void
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
	fsStats.recovery.number++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcRecovery --
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
Fs_RpcRecovery(srvToken, clientID, command, storagePtr)
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
	Net_HostPrint(clientID, "started recovery\n");
	Recov_SetClientState(clientID, CLT_RECOV_IN_PROGRESS);
    } else {
	fsStats.recovery.clientRecovered++;
	Net_HostPrint(clientID, "completed recovery\n");
	Recov_ClearClientState(clientID, CLT_RECOV_IN_PROGRESS);
    }
    Rpc_Reply(srvToken, SUCCESS, storagePtr, (int(*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}
