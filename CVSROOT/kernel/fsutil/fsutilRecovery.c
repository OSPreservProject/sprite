/* 
 * fsRecovery.c
 *
 *	Routines for filesytem recovery.
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

void FsRecoveryDone();
void FsHandleReopen();

#define LOCKPTR (&recovPtr->lock)


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
 *	Does recovery RPCs to the file server.  When successful, recovering
 *	a handle unblocks processes that are blocked on it waiting for
 *	recovery to complete.
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
    FsHandleReopen(serverID);
    /*
     * Tell the server we're done.
     */
    FsRecoveryDone(serverID);
    /*
     * Allow regular opens.
     */
    FsPrefixAllowOpens(serverID);
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
 * FsHandleReopen --
 *
 *	Called when a server reboots.  This scans the handle table twice
 *	to recover I/O handles and streams.  The first pass re-opens
 *	the I/O handles, and then streams are done.  When this completes
 *	the recovery condition in the handle is notified to unblock
 *	waiting processes. If the recovery fails this logs a warning
 *	message and marks the handle as invalid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A re-open transaction with the server.  Notifies the recovery
 *	condition in the handle, see FsHandleWait.
 *
 *----------------------------------------------------------------------------
 *
 */

void
FsHandleReopen(serverID)
    int		serverID;	/* The to re-establish contact with. */
{
    Hash_Search			hashSearch;
    register	FsHandleHeader	*hdrPtr;
    ReturnStatus		status = SUCCESS;
    Boolean			invalid;

    Net_HostPrint(serverID, "- recovering handles\n");

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	 invalid = FALSE;
	 if ((hdrPtr->fileID.type != FS_STREAM) &&
		 (hdrPtr->fileID.serverID == serverID)) {
	    status = (*fsStreamOpTable[hdrPtr->fileID.type].reopen)(hdrPtr,
		rpc_SpriteID, (ClientData)NIL, (int *)NIL, (ClientData *)NIL);
	    switch (status) {
		case SUCCESS:
		    break;
		case RPC_TIMEOUT:
		    FsHandleUnlock(hdrPtr);
		    goto reopenReturn;
		default:
		    FsFileError(hdrPtr, "Reopen failed ", status);
		    /* FALL THROUGH */
		case FS_VERSION_MISMATCH:
		case FS_FILE_REMOVED:
		    FsHandleInvalidate(hdrPtr);
		    FsHandleRemove(hdrPtr);
		    invalid = TRUE;
		    break;
	    }
	}
	if (!invalid) {
	    FsHandleUnlock(hdrPtr);
	}
    }
    /*
     * Now go through and recover streams, once we've gotten the regular
     * I/O handles re-opened.  This ensures that the I/O handle will be
     * around on the server when it re-creates our streams.
     */
    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	 if ((hdrPtr->fileID.type == FS_STREAM) &&
		 (hdrPtr->fileID.serverID == serverID)) {
	    status = (*fsStreamOpTable[hdrPtr->fileID.type].reopen)(hdrPtr,
		rpc_SpriteID, (ClientData)NIL, (int *)NIL, (ClientData *)NIL);
	    if (status != SUCCESS) {
		FsFileError(hdrPtr, "Reopen failed ", status);
		if (status == RPC_TIMEOUT) {
		    FsHandleUnlock(hdrPtr);
		    goto reopenReturn;
		}
		/*
		 * Don't remove stream handles because the user-level
		 * close will remove them later.
		 */
		FsHandleInvalidate(hdrPtr);
	    }
	}
	FsHandleUnlock(hdrPtr);
    }

reopenReturn:

    if (status != SUCCESS) {
	Sys_Printf("Recovery failed <%x>\n", status);
    } else {
	Sys_Printf("Recovery complete\n");
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
    Byte_Zero(sizeof(FsRecoveryInfo), (Address) recovPtr); 
}

/*
 *----------------------------------------------------------------------
 *
 * FsWantRecovery --
 *
 *	This routine is called when an error has occurred trying to
 *	contact the file's IO server indicating that recovery actions
 *	should take place when the server comes back.
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
    register FsRemoteIOHandle *handlePtr = (FsRemoteIOHandle *)hdrPtr;
    FsRecoveryInfo *recovPtr = &handlePtr->recovery;

    LOCK_MONITOR;
    switch(hdrPtr->fileID.type) {
	case FS_RMT_FILE_STREAM:
	case FS_RMT_DEVICE_STREAM:
	case FS_RMT_PIPE_STREAM:
	case FS_RMT_NAMED_PIPE_STREAM:
	case FS_RMT_PSEUDO_STREAM:
	case FS_CONTROL_STREAM:
	    recovPtr->flags |= FS_WANT_RECOVERY;
	    break;
	default:
	    Sys_Panic(SYS_FATAL, "FsWantRecovery, wrong handle type <%d>\n",
		hdrPtr->fileID.type);
	    break;
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * FsWaitForRecovery --
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
ReturnStatus
FsWaitForRecovery(hdrPtr, rpcStatus)
    register FsHandleHeader	*hdrPtr;
    ReturnStatus		rpcStatus;	/* Status that caused us to
						 * need recovery. */
{
    ReturnStatus		status = SUCCESS;

    switch(hdrPtr->fileID.type) {
	case FS_RMT_FILE_STREAM:
	case FS_RMT_DEVICE_STREAM:
	case FS_RMT_PIPE_STREAM:
	case FS_RMT_NAMED_PIPE_STREAM:
	case FS_RMT_PSEUDO_STREAM:
	    break;
	default:
	    Sys_Panic(SYS_FATAL, "FsWaitForRecovery, wrong handle type <%d>\n",
		hdrPtr->fileID.type);
	    return(FS_STALE_HANDLE);
    }
    /*
     * If our caller got a stale handle (the server is probably up)
     * we want to re-establish state with the server now.  There is
     * already a reboot call-back registered with the recovery module
     * so if the server is down the re-open procedure gets called that way.
     */
    if (rpcStatus == FS_STALE_HANDLE) {
	Sys_Panic(SYS_WARNING, "Stale handle <%d,%d,%d> type %d\n",
		hdrPtr->fileID.serverID,
		hdrPtr->fileID.major,
		hdrPtr->fileID.minor,
		hdrPtr->fileID.type);
	if (!Recov_IsHostDown(hdrPtr->fileID.serverID)) {
	    FsReopen(hdrPtr->fileID.serverID, (ClientData)NIL);
	}
    }

    /*
     * Wait for the server to reboot.
     */
    status = RecoveryWait(&((FsRemoteIOHandle *)hdrPtr)->recovery);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_WaitForHost --
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
ReturnStatus
Fs_WaitForHost(streamPtr, flags, rpcStatus)
    Fs_Stream	*streamPtr;
    int		flags;
    ReturnStatus rpcStatus;	/* Status that caused us to
						 * need recovery. */
{
    register FsHandleHeader *hdrPtr = streamPtr->ioHandlePtr;
    ReturnStatus status;

    if (flags & FS_NON_BLOCKING) {
	FsWantRecovery(hdrPtr);
	status = SUCCESS;
    } else {
	status = FsWaitForRecovery(hdrPtr, rpcStatus);
    }
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
    while (recovPtr->flags & FS_WANT_RECOVERY) {
	if (Sync_Wait(&recovPtr->reopenComplete, TRUE)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    break;
	}
	if (recovPtr->flags & FS_RECOVERY_FAILED) {
	    status = FS_STALE_HANDLE;
	    break;
	}
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsRecoveryWakeup --
 *
 *	Wakeup processes waiting on recovery for a handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Processes waiting on the handle are awakened and the FS_HANDLE_RECOVERY
 *	flags is cleared from the recovery flags.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsRecoveryWakeup(recovPtr)
    FsRecoveryInfo *recovPtr;
{
    LOCK_MONITOR;

    recovPtr->flags &= ~FS_WANT_RECOVERY;
    Sync_Broadcast(&recovPtr->reopenComplete);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * FsRecoveryNeeded --
 *
 *	Returns TRUE if recovery is pending for the handle.
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
    recovWanted = (recovPtr->flags & FS_WANT_RECOVERY);
    UNLOCK_MONITOR;
    return(recovWanted);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsRemoteHandleScavange --
 *
 *	All purpose scavenging routine for remote handles.  This nukes
 *	the handle if there are no uses of it and it doesn't need recovery.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Either removes or unlocks the handle.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsRemoteHandleScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;
{
    FsRemoteIOHandle *handlePtr = (FsRemoteIOHandle *)hdrPtr;
    register FsRecoveryInfo *recovPtr = &handlePtr->recovery;

    LOCK_MONITOR;
    if (recovPtr->use.ref == 0 &&
	(recovPtr->flags & FS_WANT_RECOVERY) == 0) {
	/*
	 * This strange unlocking sequence is because the unlock monitor
	 * references data inside the handle.
	 */
	UNLOCK_MONITOR;
	FsHandleRemove(hdrPtr);
    } else {
	FsHandleUnlock(hdrPtr);
	UNLOCK_MONITOR;
    }
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
 *	RECOV_IN_PROGRESS bit from the client recovery state.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
FsClientCrashed(spriteID, clientData)
    int spriteID;		/* Client that crashed */
    ClientData clientData;	/* IGNORED */
{
    /*
     * Reset the 'recovery in progress' bit (it would be set if the
     * client crashed during recovery) so the client can open files
     * the next time it boots.
     */
    Recov_ClearClientState(spriteID, RECOV_IN_PROGRESS);
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
    int flags = RECOV_IN_PROGRESS;

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
	Sys_Panic(SYS_WARNING, "FsRecoveryDone: got status %x\n", status);
    }
}
#endif not_used

/*
 *----------------------------------------------------------------------
 *
 * FsRecoveryDone --
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
FsRecoveryDone(serverID)
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
	Sys_Panic(SYS_WARNING, "FsRecoveryDone: got status %x\n", status);
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
    if (*flagsPtr & RECOV_IN_PROGRESS) {
	Net_HostPrint(clientID, "started recovery\n");
	Recov_SetClientState(clientID, RECOV_IN_PROGRESS);
    } else {
	Net_HostPrint(clientID, "completed recovery\n");
	Recov_ClearClientState(clientID, RECOV_IN_PROGRESS);
    }
    Rpc_Reply(srvToken, SUCCESS, storagePtr, (int(*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}
