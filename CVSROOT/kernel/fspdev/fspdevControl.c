/* 
 * fsPdev.c --  
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
#include "fsPdev.h"
#include "fsOpTable.h"
#include "fsStream.h"
#include "fsClient.h"
#include "fsLock.h"
#include "fsRecovery.h"
#include "fsStat.h"
#include "proc.h"
#include "rpc.h"

/*
 *----------------------------------------------------------------------------
 *
 * FsControlHandleInit --
 *
 *	Fetch and initialize a control handle for a pseudo-device.
 *
 * Results:
 *	A pointer to the control stream I/O handle.  The found parameter is
 *	set to TRUE if the handle was already found, FALSE if we created it.
 *
 * Side effects:
 *	Initializes and installs the control handle.
 *
 *----------------------------------------------------------------------------
 *
 */
PdevControlIOHandle *
FsControlHandleInit(fileIDPtr, name)
    Fs_FileID *fileIDPtr;
    char *name;
{
    register Boolean found;
    register PdevControlIOHandle *ctrlHandlePtr;
    FsHandleHeader *hdrPtr;

    found = FsHandleInstall(fileIDPtr, sizeof(PdevControlIOHandle), name,
			    &hdrPtr);
    ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;
    if (!found) {
	ctrlHandlePtr->serverID = NIL;
	List_Init(&ctrlHandlePtr->queueHdr);
	ctrlHandlePtr->seed = 0;
	List_Init(&ctrlHandlePtr->readWaitList);
	FsLockInit(&ctrlHandlePtr->lock);
	FsRecoveryInit(&ctrlHandlePtr->rmt.recovery);
	ctrlHandlePtr->accessTime = 0;
	ctrlHandlePtr->modifyTime = 0;
	ctrlHandlePtr->owner.id = (Proc_PID)NIL;
	ctrlHandlePtr->owner.procOrFamily = 0;
	ctrlHandlePtr->prefixPtr = (FsPrefix *)NIL;
	fsStats.object.controls++;
    }
    return(ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlCltOpen --
 *
 *	Complete setup of the server's control stream.  Called from
 *	Fs_Open on the host running the server.  We mark the Control
 *	I/O handle as having a server (us).
 * 
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Installs the Control I/O handle and keeps a reference to it.
 *	Marks the process as not suitable for migration.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
		 ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* NIL. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a control stream, or NIL */
{
    register PdevControlIOHandle	*ctrlHandlePtr;

    ctrlHandlePtr = FsControlHandleInit(ioFileIDPtr, name);
    if (!List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	panic( "FsControlStreamCltOpen found control msgs\n");
    }
    ctrlHandlePtr->serverID = clientID;
    *ioHandlePtrPtr = (FsHandleHeader *)ctrlHandlePtr;
    /*
     * Can't migrate pseudo-device servers.
     */
    Proc_NeverMigrate(Proc_GetCurrentProc());
    FsHandleUnlock(ctrlHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlSelect --
 *
 *	Select on the server's control stream.  This returns readable
 *	if there are control messages in the queue.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Puts the caller on the handle's read wait list if the control
 *	stream isn't selectable.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsControlSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle on device to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)hdrPtr;

    FsHandleLock(ctrlHandlePtr);
    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	if (waitPtr != (Sync_RemoteWaiter *)NIL) {
	    FsFastWaitListInsert(&ctrlHandlePtr->readWaitList, waitPtr);
	}
	*readPtr = 0;
    }
    *writePtr = *exceptPtr = 0;
    FsHandleUnlock(ctrlHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlRead --
 *
 *	Read from the server's control stream.  The server learns of new
 *	clients by reading this stream.  Internally the stream is a list
 *	of addresses of streams created for the server.  This routine maps
 *	those addresses to streamIDs for the user level server process and
 *	returns them to the reader.
 *
 * Results:
 *	SUCCESS, FS_WOULD_BLOCK,
 *	or an error code from setting up a new stream ID.
 *
 * Side effects:
 *	The server's list of stream ptrs in the process table is updated.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream 	*streamPtr;	/* Control stream */
    int		flags;		/* FS_USER is checked */
    Address 	buffer;		/* Where to read into. */
    int		*offsetPtr;	/* IGNORED */
    int 	*lenPtr;	/* In/Out length parameter */
    Sync_RemoteWaiter *waitPtr;	/* Info for waiting */
{
    ReturnStatus 		status = SUCCESS;
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)streamPtr->ioHandlePtr;
    Pdev_Notify			notify;		/* Message returned */

    FsHandleLock(ctrlHandlePtr);

    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	/*
	 * No control messages ready.
	 */
	FsFastWaitListInsert(&ctrlHandlePtr->readWaitList, waitPtr);
	*lenPtr = 0;
	status = FS_WOULD_BLOCK;
    } else {
	register PdevNotify *notifyPtr;
	notifyPtr = (PdevNotify *)List_First(&ctrlHandlePtr->queueHdr);
	List_Remove((List_Links *)notifyPtr);
	notify.magic = PDEV_NOTIFY_MAGIC;
	status = FsGetStreamID(notifyPtr->streamPtr, &notify.newStreamID);
	if (status != SUCCESS) {
	    *lenPtr = 0;
	} else {
	    if (flags & FS_USER) {
		status = Vm_CopyOut(sizeof(notify), (Address) &notify, buffer);
		/*
		 * No need to close on error because the stream is already
		 * installed in the server process's state.  It'll be
		 * closed automatically when the server exits.
		 */
	    } else {
		bcopy((Address)&notify, buffer, sizeof(notify));
	    }
	    *lenPtr = sizeof(notify);
	}
	free((Address)notifyPtr);
    }
    FsHandleUnlock(ctrlHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlIOControl --
 *
 *	IOControls for the control stream.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Command dependent.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsControlIOControl(streamPtr, command, byteOrder, inBufPtr, outBufPtr)
    Fs_Stream *streamPtr;		/* I/O handle */
    int command;			/* File specific I/O control */
    int byteOrder;			/* Client byte order, should be same */
    Fs_Buffer *inBufPtr;		/* Command inputs */
    Fs_Buffer *outBufPtr;		/* Buffer for return parameters */

{
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    if (byteOrder != mach_ByteOrder) {
	panic( "FsControlIOControl: wrong byte order\n");
    }
    switch(command) {
	case IOC_PDEV_SIGNAL_OWNER:
	    status = FsPdevSignalOwner(ctrlHandlePtr, inBufPtr);
	    break;
	case IOC_REPOSITION:
	    status = SUCCESS;
	    break;
	case IOC_GET_FLAGS:
	    if ((outBufPtr->size >= sizeof(int)) &&
		(outBufPtr->addr != (Address)NIL)) {
		*(int *)outBufPtr->addr = 0;
	    }
	    status = SUCCESS;
	    break;
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	    status = SUCCESS;
	    break;
	case IOC_TRUNCATE:
	    status = SUCCESS;
	    break;
	case IOC_LOCK:
	case IOC_UNLOCK:
	    FsHandleLock(ctrlHandlePtr);
	    status = FsIocLock(&ctrlHandlePtr->lock, command, byteOrder,
			       inBufPtr, &streamPtr->hdr.fileID);
	    FsHandleUnlock(ctrlHandlePtr);
	    break;
	case IOC_NUM_READABLE: {
	    register int bytesAvailable;

	    if (outBufPtr->size < sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    FsHandleLock(ctrlHandlePtr);
	    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
		bytesAvailable = 0;
	    } else {
		bytesAvailable = sizeof(Pdev_Notify);
	    }
	    FsHandleUnlock(ctrlHandlePtr);
	    status = SUCCESS;
	    *(int *)outBufPtr->addr = bytesAvailable;
	    break;
	}
	case IOC_SET_OWNER:
	case IOC_GET_OWNER:
	case IOC_MAP:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
	default:
	    status = GEN_INVALID_ARG;
	    break;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlGetIOAttr --
 *
 *	Called from Fs_GetAttrStream to get the I/O attributes of a
 *	pseudo-device.  The access and modify times of the pseudo-device
 *	are obtained from the internal pdev state.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsControlGetIOAttr(fileIDPtr, clientID, attrPtr)
    register Fs_FileID		*fileIDPtr;	/* Identfies pdev connection */
    int				clientID;	/* Host ID of process asking
						 * for the attributes */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
{
    PdevControlIOHandle		*ctrlHandlePtr;

    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr == (PdevControlIOHandle *)NIL) {
	printf( "FsControlGetIOAttr, no %s handle <%d,%x,%x> client %d\n",
	    FsFileTypeToString(fileIDPtr->type), fileIDPtr->serverID,
	    fileIDPtr->major, fileIDPtr->minor, clientID);
	return(FS_FILE_NOT_FOUND);
    }

    if (ctrlHandlePtr->accessTime > 0) {
	attrPtr->accessTime.seconds = ctrlHandlePtr->accessTime;
    }
    if (ctrlHandlePtr->modifyTime > 0) {
	attrPtr->dataModifyTime.seconds = ctrlHandlePtr->modifyTime;
    }

    FsHandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlSetIOAttr --
 *
 *	Set the IO attributes of a pseudo-device.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Updates the access and modify times kept in the pdev state.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsControlSetIOAttr(fileIDPtr, attrPtr, flags)
    register Fs_FileID		*fileIDPtr;	/* Identfies pdev connection */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
    int				flags;		/* Tells which attrs to set */
{
    PdevControlIOHandle		*ctrlHandlePtr;

    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr == (PdevControlIOHandle *)NIL) {
	printf( "FsControlSetIOAttr, no handle <%d,%d,%x,%x>\n",
	    fileIDPtr->serverID, fileIDPtr->type,
	    fileIDPtr->major, fileIDPtr->minor);
	return(FS_FILE_NOT_FOUND);
    }
    if (flags & FS_SET_TIMES) {
	ctrlHandlePtr->accessTime = attrPtr->accessTime.seconds;
	ctrlHandlePtr->modifyTime = attrPtr->dataModifyTime.seconds;
    }
    FsHandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlVerify --
 *
 *	This is called during recovery.
 *	When the server at a remote site reopens its control stream it
 *	contacts the file server to re-establish itself as the server.
 *	This procedure is called from FsStreamReopen to get the control
 *	handle associated with the top-level shadow stream here at the
 *	file server.
 *
 * Results:
 *	A pointer to the control I/O handle, or NIL if the server is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with FsHandleRelease.
 *
 *----------------------------------------------------------------------
 */

FsHandleHeader *
FsControlVerify(fileIDPtr, pdevServerHostID)
    Fs_FileID	*fileIDPtr;		/* control I/O file ID */
    int		pdevServerHostID;	/* Host ID of the client */
{
    register PdevControlIOHandle	*ctrlHandlePtr;
    int serverID = -1;

    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr != (PdevControlIOHandle *)NIL) {
	if (ctrlHandlePtr->serverID != pdevServerHostID) {
	    serverID = ctrlHandlePtr->serverID;
	    FsHandleRelease(ctrlHandlePtr, TRUE);
	    ctrlHandlePtr = (PdevControlIOHandle *)NIL;
	}
    }
    if (ctrlHandlePtr == (PdevControlIOHandle *)NIL) {
	printf("FsControlVerify, server mismatch (%d not %d) for %s <%x,%x>\n",
	    pdevServerHostID, serverID, FsFileTypeToString(fileIDPtr->type),
	    fileIDPtr->major, fileIDPtr->minor);
    }
    return((FsHandleHeader *)ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlReopen --
 *
 *	Reopen a control stream.  A control handle is kept on both the
 *	file server as well as the pseudo-device server's host.  If the
 *	file server reboots a reopen has to be done in order to set
 *	the serverID field on the file server so subsequent client opens work.
 *	Thus this is called on a remote client to contact the file server,
 *	and then on the file server from the RPC stub.
 *
 * Results:
 *	SUCCESS if there is no conflict with the server reopening.
 *
 * Side effects:
 *	On the file server the serverID field is set.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;
    int			clientID;		/* ID of pdev server's host */
    ClientData		inData;			/* PdevControlReopenParams */
    int			*outSizePtr;		/* IGNORED */
    ClientData		*outDataPtr;		/* IGNORED */

{
    register PdevControlIOHandle *ctrlHandlePtr;
    register PdevControlReopenParams *reopenParamsPtr;
    register ReturnStatus status = SUCCESS;

    if (hdrPtr != (FsHandleHeader *)NIL) {
	/*
	 * Called on the pdev server's host to contact the remote
	 * file server and re-establish state.
	 */
	PdevControlIOHandle *ctrlHandlePtr;
	PdevControlReopenParams params;
	int outSize = 0;

	ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;
	reopenParamsPtr = &params;
	reopenParamsPtr->fileID = hdrPtr->fileID;
	reopenParamsPtr->serverID = ctrlHandlePtr->serverID;
	reopenParamsPtr->seed = ctrlHandlePtr->seed;
	status = FsSpriteReopen(hdrPtr, sizeof(PdevControlReopenParams),
		(Address)reopenParamsPtr, &outSize, (Address)NIL);
    } else {
	/*
	 * Called on the file server to re-establish a control handle
	 * that corresponds to a control handle on the pdev server's host.
	 */

	reopenParamsPtr = (PdevControlReopenParams *)inData;
	ctrlHandlePtr = FsControlHandleInit(&reopenParamsPtr->fileID,
					    (char *)NIL);
	if (reopenParamsPtr->serverID != NIL) {
	    /*
	     * The remote host thinks it is running the pdev server process.
	     */
	    if (ctrlHandlePtr->serverID == NIL) {
		ctrlHandlePtr->serverID = reopenParamsPtr->serverID;
		ctrlHandlePtr->seed = reopenParamsPtr->seed;
	    } else if (ctrlHandlePtr->serverID != clientID) {
		printf(
		    "PdevControlReopen conflict, %d lost to %d, %s <%x,%x>\n",
		    clientID, ctrlHandlePtr->serverID,
		    FsFileTypeToString(ctrlHandlePtr->rmt.hdr.fileID.type),
		    ctrlHandlePtr->rmt.hdr.fileID.major,
		    ctrlHandlePtr->rmt.hdr.fileID.minor);
		status = FS_FILE_BUSY;
	    }
	} else if (ctrlHandlePtr->serverID == clientID) {
	    /*
	     * The pdev server closed while we were down or unable
	     * to communicate.
	     */
	    ctrlHandlePtr->serverID = NIL;
	}
	FsHandleRelease(ctrlHandlePtr, TRUE);
     }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlClose --
 *
 *	Close a server process's control stream.  After this the pseudo-device
 *	is no longer active and client operations will fail.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Reset the control handle's serverID.
 *	Clears out the state for the control message queue.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Control stream */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)streamPtr->ioHandlePtr;
    register PdevNotify *notifyPtr;
    int extra = 0;

    /*
     * Close any server streams that haven't been given to
     * the master process yet.
     */
    while (!List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	notifyPtr = (PdevNotify *)List_First(&ctrlHandlePtr->queueHdr);
	List_Remove((List_Links *)notifyPtr);
	extra++;
	(void)Fs_Close(notifyPtr->streamPtr);
	free((Address)notifyPtr);
    }
    if (extra) {
	printf( "FsControlClose found %d left over messages\n",
			extra);
    }
    /*
     * Reset the pseudo-device server ID, both here and at the name server.
     */
    ctrlHandlePtr->serverID = NIL;
    if (ctrlHandlePtr->rmt.hdr.fileID.serverID != rpc_SpriteID) {
	(void)FsRemoteClose(streamPtr, rpc_SpriteID, procID, 0, 0,
		(ClientData)NIL);
    }
    FsWaitListDelete(&ctrlHandlePtr->readWaitList);
    FsHandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlClientKill --
 *
 *	See if a crashed client was running a pseudo-device master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the serverID field if it matches the crashed host's ID.
 *	This unlocks the handle before returning.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
FsControlClientKill(hdrPtr, clientID)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
{
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)hdrPtr;

    if (ctrlHandlePtr->serverID == clientID) {
	ctrlHandlePtr->serverID = NIL;
	FsHandleRemove(ctrlHandlePtr);
	fsStats.object.controls--;
    } else {
        FsHandleUnlock(ctrlHandlePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlScavenge --
 *
 *	See if this control stream handle is still needed.
 *
 * Results:
 *	TRUE if the handle was removed.
 *
 * Side effects:
 *	Will remove the handle if there is no server.
 *
 *----------------------------------------------------------------------
 */
Boolean
FsControlScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
{
    register PdevControlIOHandle *ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;

    if (ctrlHandlePtr->serverID == NIL) {
	FsHandleRemove(ctrlHandlePtr);
	fsStats.object.controls--;
	return(TRUE);
    } else {
        FsHandleUnlock(ctrlHandlePtr);
	return(FALSE);
    }
}
