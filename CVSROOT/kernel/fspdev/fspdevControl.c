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

#include <sprite.h>
#include <fs.h>
#include <fsconsist.h>
#include <fsutil.h>
#include <fspdev.h>
#include <fspdevInt.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fsioLock.h>
#include <fsStat.h>
#include <proc.h>
#include <rpc.h>
#include <fsrecov.h>
#include <recov.h>

/*
 *----------------------------------------------------------------------------
 *
 * FspdevControlHandleInit --
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
Fspdev_ControlIOHandle *
FspdevControlHandleInit(fileIDPtr, name)
    Fs_FileID *fileIDPtr;
    char *name;
{
    register Boolean found;
    register Fspdev_ControlIOHandle *ctrlHandlePtr;
    Fs_HandleHeader *hdrPtr;

    found = Fsutil_HandleInstall(fileIDPtr, sizeof(Fspdev_ControlIOHandle), name,
			    FALSE, &hdrPtr);
    ctrlHandlePtr = (Fspdev_ControlIOHandle *)hdrPtr;
    if (!found) {
	ctrlHandlePtr->serverID = NIL;
	List_Init(&ctrlHandlePtr->queueHdr);
	ctrlHandlePtr->seed = 0;
	List_Init(&ctrlHandlePtr->readWaitList);
	Fsio_LockInit(&ctrlHandlePtr->lock);
	Fsutil_RecoveryInit(&ctrlHandlePtr->rmt.recovery);
	ctrlHandlePtr->accessTime = 0;
	ctrlHandlePtr->modifyTime = 0;
	ctrlHandlePtr->owner.id = (Proc_PID)NIL;
	ctrlHandlePtr->owner.procOrFamily = 0;
	ctrlHandlePtr->prefixPtr = (Fsprefix *)NIL;
	fs_Stats.object.controls++;
    }
    return(ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlIoOpen --
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
FspdevControlIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
		 ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* NIL. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a control stream, or NIL */
{
    register Fspdev_ControlIOHandle	*ctrlHandlePtr;

    ctrlHandlePtr = FspdevControlHandleInit(ioFileIDPtr, name);
    if (!List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	panic( "FsControlStreamCltOpen found control msgs\n");
    }
    ctrlHandlePtr->serverID = clientID;
    *ioHandlePtrPtr = (Fs_HandleHeader *)ctrlHandlePtr;
    /*
     * Can't migrate pseudo-device servers.
     */
    Proc_NeverMigrate(Proc_GetCurrentProc());
    Fsutil_HandleUnlock(ctrlHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlSelect --
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
FspdevControlSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    Fs_HandleHeader	*hdrPtr;	/* Handle on device to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    register Fspdev_ControlIOHandle *ctrlHandlePtr =
	    (Fspdev_ControlIOHandle *)hdrPtr;

    Fsutil_HandleLock(ctrlHandlePtr);
    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	if (waitPtr != (Sync_RemoteWaiter *)NIL) {
	    Fsutil_FastWaitListInsert(&ctrlHandlePtr->readWaitList, waitPtr);
	}
	*readPtr = 0;
    }
    *writePtr = *exceptPtr = 0;
    Fsutil_HandleUnlock(ctrlHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlRead --
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
ReturnStatus
FspdevControlRead(streamPtr, readPtr, waitPtr, replyPtr)
    Fs_Stream 		*streamPtr;	/* Control stream */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    ReturnStatus 		status;
    register Fspdev_ControlIOHandle *ctrlHandlePtr =
	    (Fspdev_ControlIOHandle *)streamPtr->ioHandlePtr;
    Pdev_Notify			notify;		/* Message returned to
						 * user-level server proc */

    Fsutil_HandleLock(ctrlHandlePtr);

    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	/*
	 * No control messages ready.
	 */
	Fsutil_FastWaitListInsert(&ctrlHandlePtr->readWaitList, waitPtr);
	replyPtr->length = 0;
	status = FS_WOULD_BLOCK;
    } else {
	register FspdevNotify *notifyPtr;		/* Internal message */
	notifyPtr = (FspdevNotify *)List_First(&ctrlHandlePtr->queueHdr);
	List_Remove((List_Links *)notifyPtr);
	notify.magic = PDEV_NOTIFY_MAGIC;
	status = Fs_GetStreamID(notifyPtr->streamPtr, &notify.newStreamID);
	if (status != SUCCESS) {
	    replyPtr->length = 0;
	} else {
	    if (readPtr->flags & FS_USER) {
		status = Vm_CopyOut(sizeof(notify), (Address) &notify,
				    readPtr->buffer);
		/*
		 * No need to close on error because the stream is already
		 * installed in the server process's state.  It'll be
		 * closed automatically when the server exits.
		 */
	    } else {
		bcopy((Address)&notify, readPtr->buffer, sizeof(notify));
	    }
	    replyPtr->length = sizeof(notify);
	}
	free((Address)notifyPtr);
    }
    Fsutil_HandleUnlock(ctrlHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlIOControl --
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
FspdevControlIOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream *streamPtr;		/* I/O handle */
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register Fspdev_ControlIOHandle *ctrlHandlePtr =
	    (Fspdev_ControlIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    if (ioctlPtr->format != mach_Format) {
	panic("FsControlIOControl: wrong format\n");
    }
    switch(ioctlPtr->command) {
	case IOC_PDEV_SIGNAL_OWNER:
	    status = FspdevSignalOwner(ctrlHandlePtr, ioctlPtr);
	    break;
	case IOC_REPOSITION:
	    status = SUCCESS;
	    break;
	case IOC_GET_FLAGS:
	    if (ioctlPtr->outBufSize >= sizeof(int)) {
		*(int *)ioctlPtr->outBuffer = 0;
		replyPtr->length = sizeof(int);	/* to quiet lint */
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
	    Fsutil_HandleLock(ctrlHandlePtr);
	    status = Fsio_IocLock(&ctrlHandlePtr->lock, ioctlPtr,
			    &streamPtr->hdr.fileID);
	    Fsutil_HandleUnlock(ctrlHandlePtr);
	    break;
	case IOC_NUM_READABLE: {
	    register int bytesAvailable;

	    if (ioctlPtr->outBufSize < sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    Fsutil_HandleLock(ctrlHandlePtr);
	    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
		bytesAvailable = 0;
	    } else {
		bytesAvailable = sizeof(Pdev_Notify);
	    }
	    Fsutil_HandleUnlock(ctrlHandlePtr);
	    status = SUCCESS;
	    *(int *)ioctlPtr->outBuffer = bytesAvailable;
	    break;
	}
	case IOC_SET_OWNER:
	case IOC_GET_OWNER:
	case IOC_MAP:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_PREFIX:
	    status = SUCCESS;
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
 * FspdevControlGetIOAttr --
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
FspdevControlGetIOAttr(fileIDPtr, clientID, attrPtr)
    register Fs_FileID		*fileIDPtr;	/* Identfies pdev connection */
    int				clientID;	/* Host ID of process asking
						 * for the attributes */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
{
    Fspdev_ControlIOHandle		*ctrlHandlePtr;

    ctrlHandlePtr = Fsutil_HandleFetchType(Fspdev_ControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr == (Fspdev_ControlIOHandle *)NIL) {
	printf( "FspdevControlGetIOAttr, no %s handle <%d,%x,%x> client %d\n",
	    Fsutil_FileTypeToString(fileIDPtr->type), fileIDPtr->serverID,
	    fileIDPtr->major, fileIDPtr->minor, clientID);
	return(FS_FILE_NOT_FOUND);
    }

    if (ctrlHandlePtr->accessTime > 0) {
	attrPtr->accessTime.seconds = ctrlHandlePtr->accessTime;
    }
    if (ctrlHandlePtr->modifyTime > 0) {
	attrPtr->dataModifyTime.seconds = ctrlHandlePtr->modifyTime;
    }

    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlSetIOAttr --
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
FspdevControlSetIOAttr(fileIDPtr, attrPtr, flags)
    register Fs_FileID		*fileIDPtr;	/* Identfies pdev connection */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
    int				flags;		/* Tells which attrs to set */
{
    Fspdev_ControlIOHandle		*ctrlHandlePtr;

    ctrlHandlePtr = Fsutil_HandleFetchType(Fspdev_ControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr == (Fspdev_ControlIOHandle *)NIL) {
	printf( "FspdevControlSetIOAttr, no handle <%d,%d,%x,%x>\n",
	    fileIDPtr->serverID, fileIDPtr->type,
	    fileIDPtr->major, fileIDPtr->minor);
	return(FS_FILE_NOT_FOUND);
    }
    if (flags & FS_SET_TIMES) {
	ctrlHandlePtr->accessTime = attrPtr->accessTime.seconds;
	ctrlHandlePtr->modifyTime = attrPtr->dataModifyTime.seconds;
    }
    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlVerify --
 *
 *	This is called during recovery.
 *	When the server at a remote site reopens its control stream it
 *	contacts the file server to re-establish itself as the server.
 *	This procedure is called from Fsio_StreamReopen to get the control
 *	handle associated with the top-level shadow stream here at the
 *	file server.
 *
 * Results:
 *	A pointer to the control I/O handle, or NIL if the server is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with Fsutil_HandleRelease.
 *
 *----------------------------------------------------------------------
 */

Fs_HandleHeader *
FspdevControlVerify(fileIDPtr, pdevServerHostID, domainTypePtr)
    Fs_FileID	*fileIDPtr;		/* control I/O file ID */
    int		pdevServerHostID;	/* Host ID of the client */
    int         *domainTypePtr; 	/* Return - FS_PSEUDO_DOMAIN */
{
    register Fspdev_ControlIOHandle	*ctrlHandlePtr;
    int serverID = -1;

    ctrlHandlePtr = Fsutil_HandleFetchType(Fspdev_ControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr != (Fspdev_ControlIOHandle *)NIL) {
	if (ctrlHandlePtr->serverID != pdevServerHostID) {
	    serverID = ctrlHandlePtr->serverID;
	    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
	    ctrlHandlePtr = (Fspdev_ControlIOHandle *)NIL;
	}
    }
    if (ctrlHandlePtr == (Fspdev_ControlIOHandle *)NIL) {
	printf("FspdevControlVerify, server mismatch (%d not %d) for %s <%x,%x>\n",
	    pdevServerHostID, serverID, Fsutil_FileTypeToString(fileIDPtr->type),
	    fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_PSEUDO_DOMAIN;
    }
    return((Fs_HandleHeader *)ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlReopen --
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
FspdevControlReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;
    int			clientID;		/* ID of pdev server's host */
    ClientData		inData;			/* FspdevControlReopenParams */
    int			*outSizePtr;		/* IGNORED */
    ClientData		*outDataPtr;		/* IGNORED */

{
    register Fspdev_ControlIOHandle *ctrlHandlePtr;
    register FspdevControlReopenParams *reopenParamsPtr;
    register ReturnStatus status = SUCCESS;
    Fsrecov_HandleState	recovInfo;

    if (hdrPtr != (Fs_HandleHeader *)NIL) {
	/*
	 * Called on the pdev server's host to contact the remote
	 * file server and re-establish state.
	 */
	Fspdev_ControlIOHandle *ctrlHandlePtr;
	FspdevControlReopenParams params;
	int outSize = 0;

	ctrlHandlePtr = (Fspdev_ControlIOHandle *)hdrPtr;
	reopenParamsPtr = &params;
	reopenParamsPtr->fileID = hdrPtr->fileID;
	reopenParamsPtr->serverID = ctrlHandlePtr->serverID;
	reopenParamsPtr->seed = ctrlHandlePtr->seed;
	status = FsrmtReopen(hdrPtr, sizeof(FspdevControlReopenParams),
		(Address)reopenParamsPtr, &outSize, (Address)NIL);
    } else {
	/*
	 * Called on the file server to re-establish a control handle
	 * that corresponds to a control handle on the pdev server's host.
	 */

	reopenParamsPtr = (FspdevControlReopenParams *)inData;
	ctrlHandlePtr = FspdevControlHandleInit(&reopenParamsPtr->fileID,
					    (char *)NIL);

	/*
	 * If we're the name server and we're not the client trying to
	 * reopen its control stream, then this is a control stream that
	 * we should have in the recovery box.  In this case, clientID is
	 * the ID of the machine running the pdev server.  The second case
	 * here (after the ||) is due to network partition and we still think
	 * we know who the pdev server machine should be.
	 */
	if (fsrecov_AlreadyInit && clientID != rpc_SpriteID) {
	    Fs_FileID	fid;

	    fid = reopenParamsPtr->fileID;
	    /* Get info from recov box. */
	    printf("FspdevControlReopen: checking control %d.%d.%d.%d\n",
		    fid.type, fid.serverID, fid.major, fid.minor);
	    if (Fsrecov_GetHandle(fid, clientID, &recovInfo, TRUE) != SUCCESS) {
		panic(
		"FspdevControlReopen: couldn't get recov info for handle.");
	    }
	    /* Test for sameness. */
	    if ((recovInfo.fileID.major != fid.major) ||
		    (recovInfo.fileID.minor != fid.minor)) {
		panic(
		"FspdevControlReopen: major or minor numbers disagree.");
	    }
	    if (recovInfo.info != reopenParamsPtr->serverID) {
		if (reopenParamsPtr->serverID == NIL) {
		    panic(
		    "FspdevControlReopen: serverID disagrees - now NIL.\n");
		    /* XXX Update handle here if I get rid of code below. */
		} else {
		    panic("FspdevControlReopen: serverID disagrees.");
		}
	    }
	    if (recovInfo.clientData != reopenParamsPtr->seed) {
		panic("FspdevControlReopen: seed disagrees.");
	    }
	    /*
	     * If we're supposed to have recovered everything from the
	     * recov box, then just return here.
	     */
	    if (fsrecov_FromBox) {
		return SUCCESS;
	    }
	}

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
		    Fsutil_FileTypeToString(ctrlHandlePtr->rmt.hdr.fileID.type),
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

	    if (fsrecov_AlreadyInit && clientID != rpc_SpriteID) {
		recovInfo.info = NIL;
		if (Fsrecov_UpdateHandle(reopenParamsPtr->fileID, clientID,
			&recovInfo) != SUCCESS) {
		    panic("FspdevControlReopen: couldn't update handle.");
		}
	    }
	}
	if (recov_Transparent && !fsrecov_AlreadyInit &&
		clientID != rpc_SpriteID) {
	    Fs_FileID	fid;

	    fid = reopenParamsPtr->fileID;
	    printf(
	    "FspdevControlReopen: installing control stream %d.%d.%d.%d\n",
		    fid.type, fid.serverID, fid.major, fid.minor);
	    if (Fsrecov_AddHandle((Fs_HandleHeader *) ctrlHandlePtr,
		    (Fs_FileID *) NIL,
		    clientID, ctrlHandlePtr->serverID == NIL ? NIL : 0,
		    ctrlHandlePtr->seed, TRUE) != SUCCESS) {
		/* We'll have to do better than this! */
		panic("FspdevControlReopen: couldn't add handle to recov box.");
	    }
	    /* Stream is added in stream reopen procedure. */
	}
	Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlClose --
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
FspdevControlClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Control stream */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register Fspdev_ControlIOHandle *ctrlHandlePtr =
	    (Fspdev_ControlIOHandle *)streamPtr->ioHandlePtr;
    register FspdevNotify *notifyPtr;
    int extra = 0;
    Fsrecov_HandleState	recovInfo;

    /*
     * Close any server streams that haven't been given to
     * the master process yet.
     */
    while (!List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	notifyPtr = (FspdevNotify *)List_First(&ctrlHandlePtr->queueHdr);
	List_Remove((List_Links *)notifyPtr);
	extra++;
	(void)Fs_Close(notifyPtr->streamPtr);
	free((Address)notifyPtr);
    }
    if (extra) {
	printf( "FspdevControlClose found %d left over messages\n",
			extra);
    }
    /*
     * Reset the pseudo-device server ID, both here and at the name server.
     */
    ctrlHandlePtr->serverID = NIL;
    if (ctrlHandlePtr->rmt.hdr.fileID.serverID != rpc_SpriteID) {
	(void)Fsrmt_Close(streamPtr, rpc_SpriteID, procID, 0, 0,
		(ClientData)NIL);
    } else {
	/*
	 * We're the name server and if we weren't the machine running the
	 * pdev server process, we must update recov box.
	 */
	if (recov_Transparent && clientID != rpc_SpriteID) {
	    if (fsrecov_DebugLevel <= 2) {
		printf("FspdevControlClose: closing ctrl handle %d.%d.%d.%d ",
			((Fs_HandleHeader *) ctrlHandlePtr)->fileID.type,
			((Fs_HandleHeader *) ctrlHandlePtr)->fileID.serverID,
			((Fs_HandleHeader *) ctrlHandlePtr)->fileID.major,
			((Fs_HandleHeader *) ctrlHandlePtr)->fileID.minor);
		printf("for client %d with serverID %d\n", clientID,
			ctrlHandlePtr->serverID);
	    }
	    /*
	     * XXX It seems like we should delete the ctrl handle in the
	     * recov box, but we don't since the close doesn't delete
	     * it from the handle table.  Is this a bug, or should I
	     * continue to mimic it?
	     */
#ifdef DONT_MIMIC_PDEV_BUG
	    if (Fsrecov_DeleteHandle((Fs_HandleHeader *) ctrlHandlePtr,
		    clientID, 0) != SUCCESS) {
		/* We'll have to do better than this! */
		panic(
		"FspdevControlClose: couldn't remove handle from recov box.");
	    }
#else
	    /* Update serverID to be NIL so it matches what's done above. */
	    if (Fsrecov_GetHandle(ctrlHandlePtr->rmt.hdr.fileID, clientID,
		    &recovInfo, FALSE) != SUCCESS) {
		panic(
		"FspdevControlClose: couldn't get recov info for handle.");
	    }
	    recovInfo.info = NIL;
	    if (Fsrecov_UpdateHandle(ctrlHandlePtr->rmt.hdr.fileID, clientID,
		    &recovInfo) != SUCCESS) {
		panic("FspdevControlClose: couldn't update handle.");
	    }
#endif DONT_MIMIC_PDEV_BUG
	    /* Still allow the stream to close since that's done elsewhere. */
	    if (Fsrecov_DeleteHandle((Fs_HandleHeader *) streamPtr, clientID,
		    streamPtr->flags) != SUCCESS) {
		panic(
		"FspdevControlClose: couldn't remove stream from recov box.");
	    }
	}
    }
    Fsutil_WaitListDelete(&ctrlHandlePtr->readWaitList);
    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlClientKill --
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
FspdevControlClientKill(hdrPtr, clientID)
    Fs_HandleHeader *hdrPtr;	/* File being killed */
    int		clientID;	/* Client ID to kill. */
{
    register Fspdev_ControlIOHandle *ctrlHandlePtr =
	    (Fspdev_ControlIOHandle *)hdrPtr;

    if (ctrlHandlePtr->serverID == clientID) {
	ctrlHandlePtr->serverID = NIL;
	if (ctrlHandlePtr->rmt.hdr.fileID.serverID == rpc_SpriteID) {
	    /*
	     * We're the file server and must update recov box if we
	     * weren't the machine running the pdev server process.
	     */
	    if (recov_Transparent && clientID != rpc_SpriteID) {
		if (fsrecov_DebugLevel <= 2) {
		    printf("FspdevControlClientKill: killing ctrl handle ");
		    printf("%d.%d.%d.%d for clientID %d\n",
			    ((Fs_HandleHeader *) ctrlHandlePtr)->fileID.type,
			    ((Fs_HandleHeader *) ctrlHandlePtr)->fileID.serverID
			    , ((Fs_HandleHeader *) ctrlHandlePtr)->fileID.major,
			    ((Fs_HandleHeader *) ctrlHandlePtr)->fileID.minor,
			    clientID);
		}
		if (Fsrecov_DeleteHandle((Fs_HandleHeader *) ctrlHandlePtr,
			clientID, 0) != SUCCESS) {
		    /* We'll have to do better than this! */
		    panic(
	    "FspdevControlClientKill: couldn't remove handle from recov box.");
		}
	    }
	}
	Fsutil_RecoverySyncLockCleanup(&ctrlHandlePtr->rmt.recovery);
	Fsutil_HandleRemove(ctrlHandlePtr);
	fs_Stats.object.controls--;
    } else {
        Fsutil_HandleUnlock(ctrlHandlePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevControlScavenge --
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
FspdevControlScavenge(hdrPtr)
    Fs_HandleHeader *hdrPtr;	/* File being encapsulated */
{
    register Fspdev_ControlIOHandle *ctrlHandlePtr = (Fspdev_ControlIOHandle *)hdrPtr;

    if (ctrlHandlePtr->serverID == NIL) {
	Fsutil_RecoverySyncLockCleanup(&ctrlHandlePtr->rmt.recovery);
	Fsutil_HandleRemove(ctrlHandlePtr);
	fs_Stats.object.controls--;
	return(TRUE);
    } else {
        Fsutil_HandleUnlock(ctrlHandlePtr);
	return(FALSE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fspdev_ControlRecovTestUseCount --
 *
 *	For recovery testing, return the use count on the stream's iohandle.
 *
 * Results:
 *	Use count.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Fspdev_ControlRecovTestUseCount(handlePtr)
    Fspdev_ControlIOHandle	 *handlePtr;
{
    return handlePtr->rmt.recovery.use.ref;
}


/*
 *----------------------------------------------------------------------
 *
 * Fspdev_ControlSetupHandle --
 *
 *	Given a pdev control stream recovery object, setup the necessary handle
 *	state for it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A handle is created in put in the handle table.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fspdev_ControlSetupHandle(recovInfoPtr)
    Fsrecov_HandleState	*recovInfoPtr;
{
    Fs_FileID			fileID;
    int				clientID;
    Fspdev_ControlIOHandle	*ctrlHandlePtr;

    if (!recov_Transparent) {
	panic("Fspdev_ControlSetupHandle: shouldn't have been called.");
    }
    clientID = recovInfoPtr->fileID.serverID;
    fileID = recovInfoPtr->fileID;
    fileID.serverID = rpc_SpriteID;
    ctrlHandlePtr = FspdevControlHandleInit(&fileID, (char *) NIL);
    ctrlHandlePtr->serverID = recovInfoPtr->info;
    ctrlHandlePtr->seed = recovInfoPtr->clientData;
    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevSetupControlReopen --
 *
 *	Set up the data for an RPC to reopen a control handle.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	Data structure set up.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FspdevSetupControlReopen(hdrPtr, paramsPtr)
    Fs_HandleHeader	*hdrPtr;
    Address		paramsPtr;
{
    Fspdev_ControlIOHandle *ctrlHandlePtr;
    FspdevControlReopenParams *reopenParamsPtr;

    if (hdrPtr != (Fs_HandleHeader *)NIL) {
	/*
	 * Called on the pdev server's host to contact the remote
	 * file server and re-establish state.
	 */
	ctrlHandlePtr = (Fspdev_ControlIOHandle *)hdrPtr;
	reopenParamsPtr = (FspdevControlReopenParams *) paramsPtr;
	reopenParamsPtr->fileID = hdrPtr->fileID;
	reopenParamsPtr->serverID = ctrlHandlePtr->serverID;
	reopenParamsPtr->seed = ctrlHandlePtr->seed;
    } else {
	panic("FspdevSetupControlReopen called on file server.");
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevFinishControlReopen --
 *
 *	Do post-processing for a device handle after bulk reopen.  There is
 *	none for a pseudo device.
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
FspdevFinishControlReopen(hdrPtr, statePtr, status)
    Fs_HandleHeader	*hdrPtr;
    Address		statePtr;
    ReturnStatus	status;
{
    return;
}
