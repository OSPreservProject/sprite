/* 
 * fsPdevSetup.c --
 *
 *	Open/Close/Migration routines for pseudo-devices.
 *	
 *	There are three kinds of streams involved in the implementation,
 *	a "control" stream that is returned to the server when it first
 *	opens the pseudo-device.  When a client opens the pseudo-device
 *	two streams are created and looked together.  The "client" stream
 *	is returned to the client process, and the "server" stream is
 *	passed to the server process via the control stream.
 *
 * Copyright 1987, 1988 Regents of the University of California
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
#include "fsOpTable.h"
#include "fsStream.h"
#include "fsClient.h"
#include "fsFile.h"
#include "fsDisk.h"
#include "fsMigrate.h"
#include "fsLock.h"
#include "proc.h"
#include "rpc.h"
#include "swapBuffer.h"
#include "fsPdev.h"

/*
 *----------------------------------------------------------------------------
 *
 * FsPseudoDevSrvOpen --
 *
 *	Early open time processing, this is called on a fileserver
 *	when setting up state for a call to the CltOpen routines on
 *	the client host.  For pseudo-device server processes, which
 *	are indicated by the FS_PDEV_MASTER flag, check that no other
 *	server exists.  For all other processes, which are referred to
 *	as "clients", make sure that a server process exists and
 *	generate a new ioFileID for the connection between the client
 *	and the server.
 *
 * Results:
 *	For server processes, SUCCESS if it is now the server,
 *	FS_FILE_BUSY if there already exists a server process.  For
 *	clients, SUCCESS if there is a server or the parameters
 *	indicate this is only for get/set attributes, DEV_OFFLINE if
 *	there is no server. 
 *
 * Side effects:
 *	Save the hostID of the calling process if
 *	it is to be the server for the pseudo-device.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FsPseudoDevSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
	dataSizePtr, clientDataPtr)
     register FsLocalFileIOHandle *handlePtr;	/* A handle from FsLocalLookup.
					 * Should be LOCKED upon entry,
					 * unlocked upon exit. */
     int		clientID;	/* Host ID of client doing the open */
     register int	useFlags;	/* FS_MASTER, plus
					 * FS_READ | FS_WRITE | FS_EXECUTE*/
     register FsFileID	*ioFileIDPtr;	/* Return - I/O handle ID */
     FsFileID		*streamIDPtr;	/* Return - stream ID. 
					 * NIL during set/get attributes */
     int		*dataSizePtr;	/* Return - sizeof(FsPdevState) */
     ClientData		*clientDataPtr;	/* Return - a reference to FsPdevState.
					 * Nothing is returned during set/get
					 * attributes */

{
    register	ReturnStatus status = SUCCESS;
    FsFileID	ioFileID;
    Boolean	found;
    register	PdevControlIOHandle *ctrlHandlePtr;
    register	Fs_Stream *streamPtr;
    register	FsPdevState *pdevStatePtr;

    /*
     * The control I/O handle is identified by the fileID of the pseudo-device
     * file with type CONTROL.  The minor field has the disk decriptor version
     * number xor'ed into it to avoid conflict when you delete the
     * pdev file and recreate one with the same file number (minor field).
     * Note, if this mapping is changed here on the file server, then
     * regular control stream recovery by clients won't work.  They will
     * recover their control handles, but we will map to a different handle
     * and thus think there is no server process.
     */
    ioFileID = handlePtr->hdr.fileID;
    ioFileID.type = FS_CONTROL_STREAM;
    ioFileID.minor ^= (handlePtr->descPtr->version << 16);
    ctrlHandlePtr = FsControlHandleInit(&ioFileID, handlePtr->hdr.name,
					&found);

    if (useFlags & FS_PDEV_MASTER) {
	/*
	 * When a server opens we ensure there is only one.
	 */
	if (found && ctrlHandlePtr->serverID != NIL) {
	    status = FS_FILE_BUSY;
	} else {
	    /*
	     * Note which host is running the pseudo-device server.
	     */
	    ctrlHandlePtr->serverID = clientID;
	    /*
	     * Note our hostID is still in the hdr.serverID field of the
	     * control handle being returned to the opening process. This is
	     * used when closing the control stream to get back to us
	     * so we can clear the serverID field here.  We also set up
	     * a shadow stream here, which has us as the server so
	     * recovery and closing work right.
	     */
	    *ioFileIDPtr = ioFileID;
	    *clientDataPtr = (ClientData)NIL;
	    *dataSizePtr = 0;
	    streamPtr = FsStreamNew(rpc_SpriteID,
		(FsHandleHeader *)ctrlHandlePtr, useFlags,
				    handlePtr->hdr.name);
	    *streamIDPtr = streamPtr->hdr.fileID;
	    (void)FsStreamClientOpen(&streamPtr->clientList,
				     clientID,useFlags);
	    FsHandleRelease(streamPtr, TRUE);
	}
    } else {
	if (streamIDPtr == (FsFileID *)NIL) {
	    /*
	     * Set up for get/set attributes.  We point the client
	     * at the name of the pseudo-device, what else?
	     */
	    *ioFileIDPtr = handlePtr->hdr.fileID;
	} else if (!found || ctrlHandlePtr->serverID == NIL) {
	    /*
	     * No server process.
	     */
	    status = DEV_OFFLINE;
	} else {
	    /*
	     * The server exists.  Create a new I/O handle for the client.
	     * The major and minor numbers are generated from the fileID
	     * of the pseudo-device name (to avoid conflict with other
	     * pseudo-devices) and a clone seed (to avoid conflict with
	     * other clients of this pseudo-device).
	     */
	    if (ctrlHandlePtr->serverID == clientID) {
		ioFileIDPtr->type = FS_LCL_PSEUDO_STREAM;
	    } else {
		ioFileIDPtr->type = FS_RMT_PSEUDO_STREAM;
	    }
	    ioFileIDPtr->serverID = ctrlHandlePtr->serverID;
	    ioFileIDPtr->major = (handlePtr->hdr.fileID.serverID << 16) |
				  handlePtr->hdr.fileID.major;
	    ctrlHandlePtr->seed++;
	    ioFileIDPtr->minor = ((handlePtr->descPtr->version << 24) ^
				  (handlePtr->hdr.fileID.minor << 12)) |
				 ctrlHandlePtr->seed;
	    /*
	     * Return the control stream file ID so it can be found again
	     * later when setting up the client's stream and the
	     * corresponding server stream.  The procID and uid fields are
	     * extra here, but will be used later if the client is remote.
	     */
	    pdevStatePtr = Mem_New(FsPdevState);
	    pdevStatePtr->ctrlFileID = ctrlHandlePtr->rmt.hdr.fileID;
	    pdevStatePtr->procID = (Proc_PID)NIL;
	    pdevStatePtr->uid = NIL;
	    *clientDataPtr = (ClientData)pdevStatePtr ;
	    *dataSizePtr = sizeof(FsPdevState);
	    /*
	     * Create a streamID for the opening process.  No shadow
	     * stream is kept here.  Instead, the streamID is returned to
	     * the pdev server who sets up the shadow stream.
	     */
	    streamPtr = FsStreamNew(ctrlHandlePtr->serverID,
			(FsHandleHeader *)NIL, useFlags, handlePtr->hdr.name);
	    *streamIDPtr = streamPtr->hdr.fileID;
	    pdevStatePtr->streamID = streamPtr->hdr.fileID;
	    FsStreamDispose(streamPtr);
	}
    }
    FsHandleRelease(ctrlHandlePtr, TRUE);
    FsHandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamCltOpen --
 *
 *	This is called from Fs_Open, or from the RPC stub if the client
 *	is remote, to complete setup of a client's
 *	stream to the pseudo-device.  The server is running on this
 *	host.  This routine creates a trivial client I/O handle
 *	that references the server's I/O handle that has the main
 *	state for the connection to the server.  FsServerStreamCreate
 *	is then called to set up the server's I/O handle, and the control
 *	stream is used to pass a server stream to the server.  Finally
 *	an open transaction is made with the server process
 *	to see if it will accept the client.
 * 
 * Results:
 *	SUCCESS, unless the server process has died recently, or the
 *	server rejects the open.
 *
 * Side effects:
 *	Creates the client's I/O handle.  Calls FsServerStreamCreate
 *	which sets up the servers corresponding I/O handle.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsPseudoStreamCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register FsFileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to FsPdevState. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    ReturnStatus		status;
    Boolean			found;
    register PdevClientIOHandle	*cltHandlePtr;
    register PdevControlIOHandle *ctrlHandlePtr;
    register FsPdevState	*pdevStatePtr;
    Fs_Stream			*cltStreamPtr;
    Fs_Stream			*srvStreamPtr;
    PdevNotify			*notifyPtr;
    Proc_ControlBlock		*procPtr;
    Proc_PID 			procID;
    int				uid;

    pdevStatePtr = (FsPdevState *)streamData;
    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle,
				    &pdevStatePtr->ctrlFileID);
    /*
     * If there is no server present the creation of the stream
     * can't succeed.  This case arises when the pseudo-device
     * master goes away between FsPseudoDevSrvOpen and this call.
     */
    if ((ctrlHandlePtr == (PdevControlIOHandle *)NIL) ||
	(ctrlHandlePtr->serverID == NIL)) {
	status = DEV_OFFLINE;
	if (ctrlHandlePtr != (PdevControlIOHandle *)NIL) {
	    FsHandleRelease(ctrlHandlePtr, TRUE);
	}
	goto exit;
    }

    /*
     * Extract the seed from the minor field (see the SrvOpen routine).
     * This done in case of recovery when we'll need to reset the
     * seed kept on the file server.
     */
    ctrlHandlePtr->seed = ioFileIDPtr->minor & 0x0FFF;

    found = FsHandleInstall(ioFileIDPtr, sizeof(PdevClientIOHandle), name,
			    ioHandlePtrPtr);
    cltHandlePtr = (PdevClientIOHandle *)(*ioHandlePtrPtr);
    if (found) {
	if ((cltHandlePtr->pdevHandlePtr != (PdevServerIOHandle *)NIL) &&
	    (cltHandlePtr->pdevHandlePtr->clientPID != (unsigned int)NIL)) {
	    Sys_Panic(SYS_WARNING,
		"FsPseudoStreamCltOpen found client handle\n");
	    Sys_Printf("Check (and kill) client process %x\n",
		cltHandlePtr->pdevHandlePtr->clientPID);
	}
	/*
	 * Invalidate this lingering handle.  The client process is hung
	 * or suspended and hasn't closed its end of the pdev connection.
	 */
	FsHandleInvalidate((FsHandleHeader *)cltHandlePtr);
	FsHandleRelease(cltHandlePtr, TRUE);

	found = FsHandleInstall(ioFileIDPtr, sizeof(PdevClientIOHandle), name,
			ioHandlePtrPtr);
	cltHandlePtr = (PdevClientIOHandle *)(*ioHandlePtrPtr);
	if (found) {
	    Sys_Panic(SYS_FATAL, "FsPseudoStreamCltOpen handle still there\n");
	}
    }
    /*
     * We have to look around and decide if we are being called
     * from Fs_Open, or via RPC from a remote client.  A remote client's
     * processID and uid are passed to us via the FsPdevState.  We also
     * have to ensure that a FS_STREAM exists and has the client
     * on its list so the client's remote I/O ops. are accepted here, and
     * so that we have the right number of client references.
     */
    cltStreamPtr = FsStreamFind(&pdevStatePtr->streamID,
		(FsHandleHeader *)cltHandlePtr, *flagsPtr, name, &found);
    (void)FsStreamClientOpen(&cltStreamPtr->clientList,
			    clientID, *flagsPtr);
    if (clientID == rpc_SpriteID) {
	procPtr = Proc_GetEffectiveProc();
	procID = procPtr->processID;
	uid = procPtr->effectiveUserID;
	FsHandleRelease(cltStreamPtr, TRUE);
    } else {
	procID = pdevStatePtr->procID;
	uid = pdevStatePtr->uid;
	/*
	 * Keep a reference to the stream that will be released by Fs_RpcClose.
	 */
	FsHandleUnlock(cltStreamPtr);
    }
    /*
     * Set up the connection state and hook the client handle to it.
     */
    cltHandlePtr->pdevHandlePtr = FsServerStreamCreate(ioFileIDPtr, name);
    if (cltHandlePtr->pdevHandlePtr == (PdevServerIOHandle *)NIL) {
	status = FAILURE;
    } else {
	/*
	 * Set up a stream for the server process.  This will be picked
	 * up by FsControlRead and converted to a user-level streamID.
	 */
	srvStreamPtr = FsStreamNew(rpc_SpriteID,
				(FsHandleHeader *)cltHandlePtr->pdevHandlePtr,
				FS_READ|FS_USER, name);
	notifyPtr = Mem_New(PdevNotify);
	notifyPtr->streamPtr = srvStreamPtr;
	List_InitElement((List_Links *)notifyPtr);
	List_Insert((List_Links *)notifyPtr,
		    LIST_ATREAR(&ctrlHandlePtr->queueHdr));
	FsHandleUnlock(srvStreamPtr);
	FsFastWaitListNotify(&ctrlHandlePtr->readWaitList);
	FsHandleUnlock(cltHandlePtr->pdevHandlePtr);
	/*
	 * Set up the client list in case the client is remote.
	 */
	List_Init(&cltHandlePtr->clientList);
	(void)FsIOClientOpen(&cltHandlePtr->clientList, clientID, 0, FALSE);
	FsHandleRelease(ctrlHandlePtr, TRUE);
	/*
	 * Grab an extra reference to the server's handle so the
	 * server close routine can remove the handle and it won't
	 * go away until the client also closes.
	 */
	(void)FsHandleDup((FsHandleHeader *)cltHandlePtr->pdevHandlePtr);
	FsHandleUnlock(cltHandlePtr->pdevHandlePtr);
	/*
	 * Now that the request response stream is set up we do
	 * our first transaction with the server process to see if it
	 * will accept the open.  We unlock the handle and rely on the
	 * per-connection monitor lock instead.  This is important because a
	 * buggy pseudo-device server could be ignoring this connection
	 * request indefinitely, and leaving handles locked for long periods
	 * clogs up handle scavenging, and potentially recovery callbacks too.
	 */
	FsHandleUnlock(cltHandlePtr);
	status = FsPseudoStreamOpen(cltHandlePtr->pdevHandlePtr, *flagsPtr,
				    clientID, procID, uid);
    }
    if (status == SUCCESS) {
	*ioHandlePtrPtr = (FsHandleHeader *)cltHandlePtr;
    } else {
	FsHandleInvalidate((FsHandleHeader *) cltHandlePtr);
	FsHandleRemove(cltHandlePtr);
	(void)FsStreamClientClose(&cltStreamPtr->clientList, clientID);
	if (!found) {
	    /*
	     * Nuke shadow stream.
	     */
	    FsStreamDispose(cltStreamPtr);
	}
    }
exit:
    Mem_Free((Address)streamData);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPseudoStreamCltOpen --
 *
 *	Complete a remote client's stream to a pseudo-device.
 *	The client is on a different host than the server process.  This
 *	makes an RPC to the server's host to invoke FsPseudoStreamCltOpen.
 *	This host only keeps a FsRemoteIOHandle, and the FsRemoteIOClose
 *	routine is used to close it.
 * 
 * Results:
 *	SUCCESS unless the server process has died recently, then DEV_OFFLINE.
 *
 * Side effects:
 *	RPC to the server's host to invoke the regular setup routines.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsRmtPseudoStreamCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register FsFileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* IGNORED (== rpc_SpriteID) */
    ClientData		streamData;	/* NIL for us. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register ReturnStatus status;
    register FsPdevState *pdevStatePtr = (FsPdevState *)streamData;
    register FsRecoveryInfo *recovPtr;
    Proc_ControlBlock *procPtr;
    FsRemoteIOHandle *rmtHandlePtr;
    Boolean found;

    /*
     * Invoke via RPC FsPseudoStreamCltOpen.  Here we use the procID field
     * of the FsPdevState so that FsPseudoStreamCltOpen can pass them
     * to FsServerStreamCreate.
     */
    procPtr = Proc_GetEffectiveProc();
    pdevStatePtr->procID = procPtr->processID;
    pdevStatePtr->uid = procPtr->effectiveUserID;
    ioFileIDPtr->type = FS_LCL_PSEUDO_STREAM;
    status = FsDeviceRemoteOpen(ioFileIDPtr, *flagsPtr,	sizeof(FsPdevState),
				(ClientData)pdevStatePtr);
    if (status == SUCCESS) {
	/*
	 * Install a remote I/O handle and initialize its recovery state.
	 */
	ioFileIDPtr->type = FS_RMT_PSEUDO_STREAM;
	found = FsHandleInstall(ioFileIDPtr, sizeof(FsRemoteIOHandle), name,
		(FsHandleHeader **)&rmtHandlePtr);
	recovPtr = &rmtHandlePtr->recovery;
	if (!found) {
	    FsRecoveryInit(recovPtr);
	}
	recovPtr->use.ref++;
	if (*flagsPtr & FS_WRITE) {
	    recovPtr->use.write++;
	}
	*ioHandlePtrPtr = (FsHandleHeader *)rmtHandlePtr;
	FsHandleUnlock(rmtHandlePtr);
    }
    Mem_Free((Address)pdevStatePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamClose --
 *
 *	Close a pseudo stream that's been used by a client to talk to a server.
 *	This issues a close message to the server and then tears down the
 *	state used to implement the pseudo stream connection.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Other than the request-response to the server, this releases the
 *	pseudo stream's reference to the handle.  This may also have
 *	to contact a remote host to clean up references there, too.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Client pseudo-stream to close */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process, IGNORED */
    int			flags;		/* IGNORED */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register PdevClientIOHandle *cltHandlePtr =
	    (PdevClientIOHandle *)streamPtr->ioHandlePtr;
    Boolean cache = FALSE;

    DBG_PRINT( ("Client closing pdev %x,%x\n", 
		cltHandlePtr->hdr.fileID.major,
		cltHandlePtr->hdr.fileID.minor) );

    if (!FsIOClientClose(&cltHandlePtr->clientList, clientID, 0, &cache)) {
	/*
	 * Invalid client trying to close.
	 */
	Sys_Panic(SYS_WARNING, "FsPseudoStreamClose: client %d not found\n",
	    clientID);
	FsHandleUnlock(cltHandlePtr);
	return(GEN_INVALID_ARG);
    } else if (!List_IsEmpty(&cltHandlePtr->clientList)) {
	/*
	 * Still clients out there.
	 */
	FsHandleUnlock(cltHandlePtr);
    } else {
	/*
	 * No clients remaining so we can close down the connection.
	 * Notify the server that a client has gone away.  Then we get rid
	 * of our reference to the server's handle and nuke our own.
	 */
	FsPseudoStreamCloseInt(cltHandlePtr->pdevHandlePtr);
	FsHandleRelease(cltHandlePtr->pdevHandlePtr, FALSE);
	FsHandleRelease(cltHandlePtr, TRUE);
	FsHandleRemove(cltHandlePtr);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamMigStart --
 *
 *	Begin migration of a pseudo-stream client.  We leave the state
 *	alone because the client handle, which is here on the same host
 *	as the pseudo-device master, will still be needed after the
 *	client is remote.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Release the I/O handle.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* Buffer we fill in */
{
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamMigrate --
 *
 *	Migrate a pseudo-stream client.
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FS_LCL_PSEUDO_STREAM or FS_RMT_PSEUDO_STREAM.
 *	In the latter case FsRmtPseudoStreamMigrate is called to do all
 *	the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference FsPdevState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return FsPdevState for use by the MigEnd routine.
 
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr,
		      dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset */
    int		*sizePtr;	/* Return - sizeof(FsPdevState) */
    Address	*dataPtr;	/* Return - pointer to FsPdevState */
{
    PdevClientIOHandle			*cltHandlePtr;
    register Fs_Stream			*streamPtr;
    Boolean				found;
    Boolean				cache = FALSE;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The device was local, which is why we were called, but is
	 * now remote.
	 */
	migInfoPtr->ioFileID.type = FS_RMT_PSEUDO_STREAM;
	return(FsRmtPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr,
					offsetPtr, sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_LCL_PSEUDO_STREAM;
    cltHandlePtr = FsHandleFetchType(PdevClientIOHandle, &migInfoPtr->ioFileID);
    if (cltHandlePtr == (PdevClientIOHandle *)NIL) {
	Sys_Panic(SYS_FATAL, "FsPseudoStreamMigrate, no client handle <%d,%x,%x>\n",
		migInfoPtr->ioFileID.serverID,
		migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FAILURE);
    }
    DBG_PRINT( ("Migrating pdev %x,%x, ref %d.\n", 
		cltHandlePtr->hdr.fileID.major,
		cltHandlePtr->hdr.fileID.minor,
		cltHandlePtr->hdr.refCount) );
    /*
     * At the stream level, add the new client to the set of clients
     * for the stream, and check for any cross-network stream sharing.
     */
    streamPtr = FsStreamFind(&migInfoPtr->streamID,
			     (FsHandleHeader *)cltHandlePtr, migInfoPtr->flags,
			     (char *)NIL, &found);
    if ((streamPtr->flags & FS_RMT_SHARED) == 0) {
	/*
	 * We don't think the stream is being shared so we
	 * grab the offset from the client.
	 */
	streamPtr->offset = migInfoPtr->offset;
    }
    if ((migInfoPtr->flags & FS_RMT_SHARED) == 0) {
	/*
	 * The client doesn't perceive sharing of the stream so
	 * it must be its last reference so we do an I/O close.
	 */
	(void)FsStreamClientClose(&streamPtr->clientList,
				migInfoPtr->srcClientID);
    }
    if (FsStreamClientOpen(&streamPtr->clientList, dstClientID,
	    migInfoPtr->flags)) {
	/*
	 * We detected network sharing so we mark the stream.
	 */
	streamPtr->flags |= FS_RMT_SHARED;
	migInfoPtr->flags |= FS_RMT_SHARED;
    }
    FsHandleRelease(streamPtr, TRUE);
    /*
     * Move the client at the I/O handle level.  We are careful to only
     * close the srcClient if its migration state indicates it isn't
     * shared.  We are careful to only open the dstClient if it getting
     * the stream for the first time.
     */
    if ((migInfoPtr->flags & FS_RMT_SHARED) == 0) {
	found = FsIOClientClose(&cltHandlePtr->clientList,
		    migInfoPtr->srcClientID, migInfoPtr->flags, &cache);
	if (!found) {
	    Sys_Panic(SYS_WARNING,
		"FsPseudoStreamMigrate, IO Client %d not found\n",
		migInfoPtr->srcClientID);
	}
    }
    if (migInfoPtr->flags & FS_NEW_STREAM) {
	(void)FsIOClientOpen(&cltHandlePtr->clientList, dstClientID,
		migInfoPtr->flags, FALSE);
    }

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    *flagsPtr = streamPtr->flags;
    *offsetPtr = streamPtr->offset;
    /*
     * We don't need this reference on the I/O handle; there is no change.
     */
    FsHandleRelease(cltHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPseudoStreamMigrate --
 *
 *	Migrate a pseudo-stream client.
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FS_LCL_PSEUDO_STREAM or FS_RMT_PSEUDO_STREAM.
 *	In the former case FsPseudoStreamMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference FsPdevState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsRmtPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
			 sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - the new stream offset */
    int		*sizePtr;	/* Return - 0 */
    Address	*dataPtr;	/* Return - NIL */
{
    register ReturnStatus		status;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The device was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FS_LCL_PSEUDO_STREAM;
	return(FsPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr,
				     offsetPtr, sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_RMT_PSEUDO_STREAM;
    status = FsNotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
				0, (Address)NIL);
    DBG_PRINT( ("Migrating remote pdev %x,%x.\n", 
		migInfoPtr->ioFileID.major,
		migInfoPtr->ioFileID.minor) );
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsRmtDeviceMigrate, server error <%x>\n",
	    status);
    } else {
	*dataPtr = (Address)NIL;
	*sizePtr = 0;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamMigEnd --
 *
 *	Complete migration for a pseudo stream.  Complete setup of a 
 *	FS_RMT_DEVICE_STREAM after migration.
 *	The srvMigrate routine has done most all the work.
 *	We just grab a reference on the I/O handle for the stream.
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
ReturnStatus
FsPseudoStreamMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register PdevClientIOHandle *cltHandlePtr;

    cltHandlePtr = FsHandleFetchType(PdevClientIOHandle,
				     &migInfoPtr->ioFileID);
    if (cltHandlePtr == (PdevClientIOHandle *)NIL) {
	Sys_Panic(SYS_FATAL, "FsPseudoStreamMigEnd, no handle.\n");
	return(FAILURE);
    } else {
	/*
	 * Release this reference so the client handle always has
	 * just one reference.  Instead of refcounts, an empty
	 * client list indicates there are no more clients.
	 */
	FsHandleRelease(cltHandlePtr, TRUE);
	*hdrPtrPtr = (FsHandleHeader *)cltHandlePtr;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPseudoStreamVerify --
 *
 *	Verify that the remote client is known for the pdev, and return
 *	a locked pointer to the client I/O handle.
 *
 * Results:
 *	A pointer to the client I/O handle, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with FsHandleRelease.
 *
 *----------------------------------------------------------------------
 */

FsHandleHeader *
FsRmtPseudoStreamVerify(fileIDPtr, clientID, domainTypePtr)
    FsFileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_PSEUDO_DOMAIN */
{
    register PdevClientIOHandle	*cltHandlePtr;
    register FsClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = FS_LCL_PSEUDO_STREAM;
    cltHandlePtr = FsHandleFetchType(PdevClientIOHandle, fileIDPtr);
    if (cltHandlePtr != (PdevClientIOHandle *)NIL) {
	LIST_FORALL(&cltHandlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    FsHandleRelease(cltHandlePtr, TRUE);
	    cltHandlePtr = (PdevClientIOHandle *)NIL;
	}
    }
    if (!found) {
	Sys_Panic(SYS_WARNING,
	    "FsRmtPseudoDeviceVerify, client %d not known for pdev <%x,%x>\n",
	    clientID, fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_PSEUDO_DOMAIN;
    }
    return((FsHandleHeader *)cltHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamMigStart --
 *
 *	It's too painful to migrate the pseudo-device server.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsServerStreamMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* Buffer we fill in */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamMigEnd --
 *
 *	Complete migration for a server stream
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	The streams that compose the Server stream are also deencapsulated.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsServerStreamMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    return(GEN_NOT_IMPLEMENTED);
}
