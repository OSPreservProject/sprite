/* 
 * fsFile.c --
 *
 *	Routines for operations on files.  A file handle is identified
 *	by using the <major> field of the Fs_FileID for the domain index,
 *	and the <minor> field for the file number.
 *
 * Copyright 1987 Regents of the University of California
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
#include "fsutil.h"
#include "fsio.h"
#include "fslcl.h"
#include "fsNameOps.h"
#include "fscache.h"
#include "fsconsist.h"
#include "fsprefix.h"
#include "fsioLock.h"
#include "fsdm.h"
#include "fsStat.h"
#include "vm.h"
#include "rpc.h"
#include "recov.h"

void IncVersionNumber();

static Fscache_IOProcs  lclFileIOProcs = {
    Fsdm_BlockAllocate, 
    Fsio_FileBlockRead, 
    Fsio_FileBlockWrite,
    Fsio_FileBlockCopy
};


/*
 *----------------------------------------------------------------------
 *
 * Fsio_LocalFileHandleInit --
 *
 *	Initialize a handle for a local file from its descriptor on disk.
 *
 * Results:
 *	An error code from the read of the file descriptor off disk.
 *
 * Side effects:
 *	Create and install a handle for the file.  It is returned locked
 *	and with its reference count incremented if SUCCESS is returned.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_LocalFileHandleInit(fileIDPtr, name, newHandlePtrPtr)
    Fs_FileID	*fileIDPtr;
    char	*name;
    Fsio_FileIOHandle	**newHandlePtrPtr;
{
    register ReturnStatus status;
    register Fsio_FileIOHandle *handlePtr;
    register Fsdm_FileDescriptor *descPtr;
    register Fsdm_Domain *domainPtr;
    register Boolean found;

    found = Fsutil_HandleInstall(fileIDPtr, sizeof(Fsio_FileIOHandle), name,
		    (Fs_HandleHeader **)newHandlePtrPtr);
    if (found) {
	/*
	 * All set.
	 */
	if ((*newHandlePtrPtr)->descPtr == (Fsdm_FileDescriptor *)NIL) {
	    panic("Fsio_LocalFileHandleInit, found handle with no descPtr\n");
	}
	return(SUCCESS);
    }
    /*
     * Get a hold of the disk file descriptor.
     */
    handlePtr = *newHandlePtrPtr;
    domainPtr = Fsdm_DomainFetch(fileIDPtr->major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	Fsutil_HandleRelease(handlePtr, TRUE);
	Fsutil_HandleRemove(handlePtr);
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = (Fsdm_FileDescriptor *)malloc(sizeof(Fsdm_FileDescriptor));
    status = Fsdm_FileDescFetch(domainPtr, fileIDPtr->minor, descPtr);
    Fsdm_DomainRelease(fileIDPtr->major);

    if (status != SUCCESS) {
	printf( "Fsio_LocalFileHandleInit: Fsdm_FileDescFetch failed");
    } else if (!(descPtr->flags & FSDM_FD_ALLOC)) {
	status = FS_FILE_REMOVED;
    } else {
	Fscache_Attributes attr;

	handlePtr->descPtr = descPtr;
	handlePtr->flags = 0;
	/*
	 * The use counts are updated when an I/O stream is opened on the file
	 */
	handlePtr->use.ref = 0;
	handlePtr->use.write = 0;
	handlePtr->use.exec = 0;

	/*
	 * Copy attributes that are cached in the handle.
	 */
	attr.firstByte = descPtr->firstByte;
	attr.lastByte = descPtr->lastByte;
	attr.accessTime = descPtr->accessTime;
	attr.createTime = descPtr->createTime;
	attr.modifyTime = descPtr->dataModifyTime;
	attr.userType = descPtr->userType;
	attr.permissions = descPtr->permissions;
	attr.uid = descPtr->uid;
	attr.gid = descPtr->gid;

	Fscache_InfoInit(&handlePtr->cacheInfo, (Fs_HandleHeader *)handlePtr,
		descPtr->version, TRUE, &attr, &lclFileIOProcs);

	Fsconsist_Init(&handlePtr->consist, (Fs_HandleHeader *)handlePtr);
	Fsio_LockInit(&handlePtr->lock);
	Fscache_ReadAheadInit(&handlePtr->readAhead);

	handlePtr->segPtr = (Vm_Segment *)NIL;
    }
    if (status != SUCCESS) {
	Fsutil_HandleRelease(handlePtr, TRUE);
	Fsutil_HandleRemove(handlePtr);
	free((Address)descPtr);
	*newHandlePtrPtr = (Fsio_FileIOHandle *)NIL;
    } else {
	if (descPtr->fileType == FS_DIRECTORY) {
	    fs_Stats.object.directory++;
	} else {
	    fs_Stats.object.files++;
	}
	*newHandlePtrPtr = handlePtr;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileSyncLockCleanup --
 *
 *	This takes care of the dynamically allocated Sync_Lock's that
 *	are embedded in a Fsio_FileIOHandle.  This routine is
 *	called when the file handle is being removed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The locking statistics for this handle are combined with the
 *	summary statistics for the lock types in the handle
 *
 *----------------------------------------------------------------------
 */
void
Fsio_FileSyncLockCleanup(handlePtr)
    Fsio_FileIOHandle *handlePtr;
{
    Fsconsist_SyncLockCleanup(&handlePtr->consist);
    Fscache_InfoSyncLockCleanup(&handlePtr->cacheInfo);
    Fscache_ReadAheadSyncLockCleanup(&handlePtr->readAhead);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileNameOpen --
 *
 *	This is called in two cases after name lookup on the server.
 *	The first is when a client is opening the file from Fs_Open.
 *	The second is when a lookup is done when getting/setting the
 *	attributes of the files.  In the open case this routine has
 *	to set up Fsio_FileState that the client will use to complete
 *	the setup of its stream, and create a server-side stream.
 *	The handle should be locked upon entry, it remains locked upon return.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	The major side effect of this routine is to invoke cache consistency
 *	actions by other clients.  This also does conflict checking, like
 *	preventing writing if a file is being executed.  Lastly, a shadow
 *	stream is created here on the server to support migration.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_FileNameOpen(handlePtr, openArgsPtr, openResultsPtr)
     register Fsio_FileIOHandle *handlePtr;	/* A handle from FslclLookup.
					 * Should be LOCKED upon entry,
					 * Returned UNLOCKED. */
     Fs_OpenArgs		*openArgsPtr;	/* Standard open arguments */
     Fs_OpenResults	*openResultsPtr;/* For returning ioFileID, streamID,
					 * and Fsio_FileState */
{
    Fsio_FileState *fileStatePtr;
    ReturnStatus status;
    register useFlags = openArgsPtr->useFlags;
    register clientID = openArgsPtr->clientID;
    register Fs_Stream *streamPtr;

    if ((useFlags & FS_WRITE) &&
	(handlePtr->descPtr->fileType == FS_DIRECTORY)) {
	status = FS_IS_DIRECTORY;
	goto exit;
    }
    if (handlePtr->descPtr->fileType == FS_DIRECTORY) {
	/*
	 * Strip off execute permission that was used to allow access to
	 * a directory.
	 */
	useFlags &= ~FS_EXECUTE;
    }
    /*
     * Check against writing and executing at the same time.  Fs_Open already
     * checks that useFlags doesn't contain both execute and write bits.
     */
    if (((useFlags & FS_EXECUTE) && (handlePtr->use.write > 0)) ||
	((useFlags & (FS_WRITE|FS_CREATE)) && (handlePtr->use.exec > 0))) {
	status = FS_FILE_BUSY;
	goto exit;
    }
    /*
     * Add in read permission when executing a file so Fs_Read doesn't
     * foil page-ins later.
     */
    if ((useFlags & FS_EXECUTE) && (handlePtr->descPtr->fileType == FS_FILE)) {
	useFlags |= FS_READ;
    }
    /*
     * Set up the ioFileIDPtr so our caller can set/get attributes.
     */
    openResultsPtr->ioFileID = handlePtr->hdr.fileID;
    if (clientID != rpc_SpriteID) {
	openResultsPtr->ioFileID.type = FSIO_RMT_FILE_STREAM;
    }
    if (useFlags == 0) { 
	/*
	 * Only being called from the get/set attributes code.
	 * Setting up the ioFileID is all that is needed.
	 */
	status = SUCCESS;
    } else {
	/*
	 * Called during an open.  Update the summary use counts while
	 * we still have the handle locked.  Then unlock the handle and
	 * do consistency call-backs.  The handle is unlocked to allow
	 * servicing of RPCs which are side effects
	 * of the consistency requests (i.e. write-backs).
	 */
	handlePtr->use.ref++;
	if (useFlags & FS_WRITE) {
	    handlePtr->use.write++;
	    IncVersionNumber(handlePtr);
	}
	if (useFlags & FS_EXECUTE) {
	    handlePtr->use.exec++;
	}
	Fsutil_HandleUnlock(handlePtr);
	fileStatePtr = mnew(Fsio_FileState);
	status = Fsconsist_FileConsistency(handlePtr, clientID, useFlags,
		    &fileStatePtr->cacheable, &fileStatePtr->openTimeStamp);
	if (status == SUCCESS) {
	    /*
	     * Copy cached attributes into the returned file state.
	     */
	    Fscache_GetCachedAttr(&handlePtr->cacheInfo, &fileStatePtr->version,
			&fileStatePtr->attr);
	    /*
	     * Return new usage flags to the client.  This lets us strip
	     * off the execute use flag (above, for directories) so
	     * the client doesn't have to worry about it.
	     */
	    fileStatePtr->newUseFlags = useFlags;
	    openResultsPtr->streamData = (ClientData)fileStatePtr;
	    openResultsPtr->dataSize = sizeof(Fsio_FileState);

	    /*
	     * Now set up a shadow stream on here on the server so we
	     * can support shared offset even after migration.
	     * Note: prefix handles get opened, but the stream is not used,
	     * could dispose stream in FslclExport.
	     */

	    streamPtr = Fsio_StreamCreate(rpc_SpriteID, clientID,
		(Fs_HandleHeader *)handlePtr, useFlags, handlePtr->hdr.name);
	    openResultsPtr->streamID = streamPtr->hdr.fileID;
	    Fsutil_HandleRelease(streamPtr, TRUE);
	    return(SUCCESS);
	} else {
	    /*
	     * Consistency call-backs failed because the last writer
	     * could not write back its copy of the file. We garbage
	     * collect the client to retreat to a known bookkeeping point.
	     */
	    int ref, write, exec;
	    printf("Consistency failed %x on <%d,%d>\n", status,
		handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor);
	    Fsutil_HandleLock(handlePtr);
	    Fsconsist_Kill(&handlePtr->consist, clientID,
			  &ref, &write, &exec);
	    handlePtr->use.ref   -= ref;
	    handlePtr->use.write -= write;
	    handlePtr->use.exec  -= exec;
	    free((Address)fileStatePtr);
	}
    }
exit:
    Fsutil_HandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileReopen --
 *
 *      Reopen a file for use by a remote client. State is maintained in the
 *      handle's client list about this open and whether or not the client
 *      is caching.
 *
 * Results:
 *	A failure code if the client was caching dirty blocks but lost
 *	the race to re-open its file.  (i.e. another client already opened
 *	for writing.)
 *
 * Side effects:
 *	The client use state for the client is brought into agreement
 *	with what the client tells us.  We do cache consistency too.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;	/* IGNORED here on the server */
    int			clientID;	/* Client doing the reopen */
    ClientData		inData;		/* Fsio_FileReopenParams */
    int			*outSizePtr;	/* Size of returned data */
    ClientData		*outDataPtr;	/* Returned data */
{
    register Fsio_FileReopenParams *reopenParamsPtr; /* Parameters from RPC */
    register Fsio_FileState	*fileStatePtr;	/* Results for RPC */
    Fsio_FileIOHandle	    	*handlePtr;	/* Local handle for file */
    register ReturnStatus	status = SUCCESS; /* General return code */
    Fsdm_Domain			*domainPtr;

    *outDataPtr = (ClientData) NIL;
    *outSizePtr = 0;
    /*
     * Do initial setup for the reopen.  We make sure that the disk
     * for the file is still around first, mark the client
     * as doing recovery, and fetch a local handle for the file.
     * NAME note: we have no name for the file after a re-open.
     */
    reopenParamsPtr = (Fsio_FileReopenParams *) inData;
    domainPtr = Fsdm_DomainFetch(reopenParamsPtr->fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    status = Fsio_LocalFileHandleInit(&reopenParamsPtr->fileID, (char *)NIL,
	&handlePtr);
    if (status != SUCCESS) {
	goto reopenReturn;
    }
    /*
     * See if the client can still cache its dirty blocks.
     */
    if (reopenParamsPtr->flags & FSIO_HAVE_BLOCKS) {
	status = Fscache_CheckVersion(&handlePtr->cacheInfo,
				     reopenParamsPtr->version, clientID);
	if (status != SUCCESS) {
	    Fsutil_HandleRelease(handlePtr, TRUE);
	    goto reopenReturn;
	}
    }
    /*
     * Update global use counts and version number.
     */
    Fsconsist_ReopenClient(handlePtr, clientID, reopenParamsPtr->use,
			reopenParamsPtr->flags & FSIO_HAVE_BLOCKS);
    if (reopenParamsPtr->use.write > 0) {
	IncVersionNumber(handlePtr);
    }
    /*
     * Now unlock the handle and do cache consistency call-backs.
     */
    fileStatePtr = mnew(Fsio_FileState);
    fileStatePtr->cacheable = reopenParamsPtr->flags & FSIO_HAVE_BLOCKS;
    Fsutil_HandleUnlock(handlePtr);
    status = Fsconsist_ReopenConsistency(handlePtr, clientID, reopenParamsPtr->use,
		reopenParamsPtr->flags & FS_SWAP,
		&fileStatePtr->cacheable, &fileStatePtr->openTimeStamp);
    if (status != SUCCESS) {
	/*
	 * Consistency call-backs failed, probably due to disk-full.
	 * We kill the client here as it will invalidate its handle
	 * after this re-open fails.
	 */
	int ref, write, exec;
	Fsutil_HandleLock(handlePtr);
	Fsconsist_Kill(&handlePtr->consist, clientID, &ref, &write, &exec);
	handlePtr->use.ref   -= ref;
	handlePtr->use.write -= write;
	handlePtr->use.exec  -= exec;
	Fsutil_HandleUnlock(handlePtr);
	free((Address)fileStatePtr);
    } else {
	/*
	 * Successful re-open here on the server. Copy cached attributes
	 * into the returned file state.
	 */
	Fscache_GetCachedAttr(&handlePtr->cacheInfo, &fileStatePtr->version,
		    &fileStatePtr->attr);
	fileStatePtr->newUseFlags = 0;		/* Not used in re-open */
	*outDataPtr = (ClientData) fileStatePtr;
	*outSizePtr = sizeof(Fsio_FileState);
    }
    Fsutil_HandleRelease(handlePtr, FALSE);
reopenReturn:
    Fsdm_DomainRelease(reopenParamsPtr->fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileIoOpen --
 *
 *      Set up a stream for a local disk file.  This is called from Fs_Open to
 *	complete the opening of a stream.  By this time any cache consistency
 *	actions have already been taken, and local use counts have been
 *	incremented by Fsio_FileNameOpen.
 *
 * Results:
 *	SUCCESS, unless there was an error installing the handle.
 *
 * Side effects:
 *	Installs the handle for the file.  This increments its refererence
 *	count (different than the use count).
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    Fs_FileID		*ioFileIDPtr;	/* I/O fileID from the name server */
    int			*flagsPtr;	/* Return only.  The server returns
					 * a modified useFlags in Fsio_FileState */
    int			clientID;	/* IGNORED */
    ClientData		streamData;	/* Fsio_FileState. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a handle set up for
					 * I/O to a file, NIL if failure. */
{
    register ReturnStatus	status;

    status = Fsio_LocalFileHandleInit(ioFileIDPtr, name,
		(Fsio_FileIOHandle **)ioHandlePtrPtr);
    if (status == SUCCESS) {
	/*
	 * Return the new useFlags from the server.  It has stripped off
	 * execute permission for directories.
	 */
	*flagsPtr = ( (Fsio_FileState *)streamData )->newUseFlags;
	Fsutil_HandleUnlock(*ioHandlePtrPtr);
    }
    free((Address)streamData);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileClose --
 *
 *	Close time processing for local files.  We need to remove ourselves
 *	from the list of clients of the file, decrement use counts, and
 *	handle pending deletes.  This returns either with one reference
 *	to the handle released, or with the handle removed entirely.
 *
 * Results:
 *	SUCCESS, FS_FILE_REMOVED, or an error code from the disk operation
 *	on the file descriptor.
 *
 * Side effects:
 *	Attributes cached on clients are propogated to the local handle.
 *	Use counts in the client list are decremented.  The handle's
 *	use counts are decremented.  If the file has been deleted, then
 *	the file descriptor is so marked, other clients are told of
 *	the delete, and the handle is removed entirely.  Otherwise,
 *	a reference on the handle is released.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsio_FileClose(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to regular file */
    int			clientID;	/* Host ID of closer */
    Proc_PID		procID;		/* Process ID of closer */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of closeData */
    ClientData		closeData;	/* Ref. to Fscache_Attributes */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus		status;
    Boolean			wasCached = TRUE;

    /*
     * Update the client state to reflect the close by the client.
     */
    if (!Fsconsist_Close(&handlePtr->consist, clientID, flags, &wasCached)) {
	printf("Fsio_FileClose, client %d pid %x unknown for file <%d,%d>\n",
		  clientID, procID, handlePtr->hdr.fileID.major,
		  handlePtr->hdr.fileID.minor);
	Fsutil_HandleUnlock(handlePtr);
	return(FS_STALE_HANDLE);
    }
    if (wasCached && dataSize != 0) {
	/*
	 * Update the server's attributes from ones cached on the client.
	 */
	Fscache_UpdateAttrFromClient(clientID, &handlePtr->cacheInfo,
				(Fscache_Attributes *)closeData);
    }

    Fsio_LockClose(&handlePtr->lock, &streamPtr->hdr.fileID);

    /*
     * Update use counts and handle pending deletions.
     */
    status = Fsio_FileCloseInt(handlePtr, 1, (flags & FS_WRITE) != 0,
				     (flags & FS_EXECUTE) != 0,
				     clientID, TRUE);
    if (status == FS_FILE_REMOVED) {
	if (clientID == rpc_SpriteID) {
	    status = SUCCESS;
	}
    } else {
	/*
	 * Force the file to disk if we are told to do so by a client.
	 */
	if (flags & FS_WB_ON_LDB) {
	    int blocksSkipped;
	    status = Fscache_FileWriteBack(&handlePtr->cacheInfo, 0, 
				    FSCACHE_LAST_BLOCK,
				    FSCACHE_FILE_WB_WAIT | FSCACHE_WRITE_BACK_INDIRECT,
				    &blocksSkipped);
	    if (status != SUCCESS) {
		printf("Fsio_FileClose: write back <%d,%d> \"%s\" err <%x>\n",
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName(handlePtr), status);
	    }
	    status = Fsdm_FileDescWriteBack(handlePtr, TRUE);
	    if (status != SUCCESS) {
		printf("Fsio_FileClose: desc write <%d,%d> \"%s\" err <%x>\n",
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		    Fsutil_HandleName(handlePtr), status);
	    }
	} else {
	    status = SUCCESS;
	}
	Fsutil_HandleRelease(handlePtr, TRUE);
    }

    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_FileCloseInt --
 *
 *	Close a file, handling pending deletions.
 *	This is called from the regular close routine, from
 *	the file client-kill cleanup routine, and from the
 *	lookup routine that deletes file names.
 *
 * Results:
 *	SUCCESS or FS_FILE_REMOVED.
 *
 * Side effects:
 *	Adjusts use counts and does pending deletions.  If the file is
 *	deleted the handle can not be used anymore.  Otherwise it
 *	is left locked.
 *
 * ----------------------------------------------------------------------------
 *
 */
ReturnStatus
Fsio_FileCloseInt(handlePtr, ref, write, exec, clientID, callback)
    Fsio_FileIOHandle *handlePtr;	/* File to clean up */
    int ref;				/* Number of uses to remove */
    int write;				/* Number of writers to remove */
    int exec;				/* Number of executers to remove */
    int clientID;			/* Closing, or crashed, client */
    Boolean callback;			/* TRUE if we should call back to
					 * the client and tell it about
					 * the deletion. */
{
    register ReturnStatus status;
    /*
     * Update the global/summary use counts for the file.
     */
    handlePtr->use.ref -= ref;
    handlePtr->use.write -= write;
    handlePtr->use.exec -= exec;
    if (handlePtr->use.ref < 0 || handlePtr->use.write < 0 ||
	handlePtr->use.exec < 0) {
	panic("Fsio_FileCloseInt <%d,%d> use %d, write %d, exec %d\n",
	    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
	    handlePtr->use.ref, handlePtr->use.write, handlePtr->use.exec);
    }

    /*
     * Handle pending deletes
     *	1. Scan the client list and call-back to the last writer if
     *		it is not the client doing the close.
     *	2. Mark the disk descriptor as deleted,
     *	3. Remove the file handle.
     *	4. Return FS_FILE_REMOVED so clients know to nuke their cache.
     */
    if ((handlePtr->use.ref == 0) && (handlePtr->flags & FSIO_FILE_DELETED)) {
	if (handlePtr->descPtr->fileType == FS_DIRECTORY) {
	    fs_Stats.object.directory--;
	} else {
	    fs_Stats.object.files--;
	}
	if (callback) {
	    Fsconsist_ClientRemoveCallback(&handlePtr->consist, clientID);
	}
	(void)Fslcl_DeleteFileDesc(handlePtr);
	Fsio_FileSyncLockCleanup(handlePtr);
	if (callback) {
	    Fsutil_HandleRelease(handlePtr, TRUE);
	}
	Fsutil_HandleRemove(handlePtr);
	status = FS_FILE_REMOVED;
    } else {
	status = SUCCESS;
    }
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_FileClientKill --
 *
 *	Called when a client is assumed down.  This cleans up the
 *	cache consistency state associated with the client, and reflects
 *	these changes in uses (i.e. less writers) in the handle's global
 *	use counts.
 *	
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Removes the client list entry for the client and adjusts the
 *	use counts on the file.  This has to remove or unlock the handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
Fsio_FileClientKill(hdrPtr, clientID)
    Fs_HandleHeader	*hdrPtr;	/* File to clean up */
    int			clientID;	/* Host assumed down */
{
    Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
    int refs, writes, execs;
    register ReturnStatus status;

    Fsconsist_IOClientKill(&handlePtr->consist.clientList, clientID,
		    &refs, &writes, &execs);
    Fsio_LockClientKill(&handlePtr->lock, clientID);

    status = Fsio_FileCloseInt(handlePtr, refs, writes, execs, clientID, FALSE);
    if (status != FS_FILE_REMOVED) {
	Fsutil_HandleUnlock(handlePtr);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_FileScavenge --
 *
 *	Called periodically to see if this handle is still needed.
 *	
 *
 * Results:
 *	TRUE if it removed the handle.
 *
 * Side effects:
 *	Removes the handle if their are no references to it and no
 *	blocks in the cache for it.  Otherwise it unlocks the handle
 *	before returning.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
Boolean
Fsio_FileScavenge(hdrPtr)
    Fs_HandleHeader	*hdrPtr;	/* File to clean up */
{
    register Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
    register Fsdm_FileDescriptor *descPtr = handlePtr->descPtr;
    register Boolean noUsers;
    Fsdm_Domain *domainPtr;
    ReturnStatus status;

    /*
     * Write back the descriptor in case we decide below to remove the handle.
     */
    if (descPtr->flags & FSDM_FD_DIRTY) {
	descPtr->flags &= ~FSDM_FD_DIRTY;
	domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
	if (domainPtr == (Fsdm_Domain *)NIL ){
	    panic("Fsio_FileScavenge: Dirty descriptor in detached domain.\n");
	} else {
	    status = Fsdm_FileDescStore(domainPtr, handlePtr->hdr.fileID.minor, 
				      descPtr);
	    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
	    if (status != SUCCESS) {
		printf("Fsio_FileScavenge: Could not store file desc <%x>\n",
			status);
	    }
	}
    }
    /*
     * We can reclaim the handle if the following holds.
     *	1. There are no active users of the file.
     *  2. The file is not undergoing deletion
     *		(The deletion will handle removing the handle)
     *  3. There are no remote clients of the file.  In particular,
     *		the last writer might not be active, but we can't
     *		nuke the handle until after it writes back.
     */
    noUsers = (handlePtr->use.ref == 0) &&
	     ((handlePtr->flags & FSIO_FILE_DELETED) == 0) &&
	      (Fsconsist_NumClients(&handlePtr->consist) == 0);
    if (noUsers && handlePtr->descPtr->fileType == FS_DIRECTORY) {
	/*
	 * Flush unused directories, otherwise they linger for a long
	 * time.  They may still be in the name cache, in which case
	 * HandleAttemptRemove won't delete them.
	 */
	int blocksSkipped;
	status = Fscache_FileWriteBack(&handlePtr->cacheInfo,
		0, FSCACHE_LAST_BLOCK,
		FSCACHE_FILE_WB_WAIT | FSCACHE_WRITE_BACK_INDIRECT |
		FSCACHE_WRITE_BACK_AND_INVALIDATE, &blocksSkipped);
	noUsers = (status == SUCCESS) && (blocksSkipped == 0);
    }
    if (noUsers && Fscache_OkToScavenge(&handlePtr->cacheInfo)) {
	register Boolean isDir;
#ifdef CONSIST_DEBUG
	extern int fsTraceConsistMinor;
	if (fsTraceConsistMinor == handlePtr->hdr.fileID.minor) {
	    printf("Fsio_FileScavenge <%d,%d> nuked, lastwriter %d\n",
		handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		handlePtr->consist.lastWriter);
	}
#endif	CONSIST_DEBUG
	/*
	 * Remove handles for files with no users and no blocks in cache.
	 * We tell VM not to cache the segment associated with the file.
	 * The "attempt remove" call unlocks the handle and then frees its
	 * memory if there are no references to it lingering from the name
	 * hash table.
	 */
	Vm_FileChanged(&handlePtr->segPtr);
	isDir = (handlePtr->descPtr->fileType == FS_DIRECTORY);
	if (Fsutil_HandleAttemptRemove(handlePtr)) {
	    if (isDir) {
		fs_Stats.object.directory--;
		fs_Stats.object.dirFlushed++;
	    } else {
		fs_Stats.object.files--;
	    }
	    return(TRUE);
	} else {
	    return(FALSE);
	}
    } else {
	Fsutil_HandleUnlock(hdrPtr);
	return(FALSE);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_FileMigClose --
 *
 *	Initiate migration of a FSIO_LCL_FILE_STREAM.  There is no extra
 *	state needed than already put together by Fsio_EncapStream.  However,
 *	we do release a low-level reference on the handle which is
 *	re-obtained by FsFileDeencap.  Other than that, we leave the
 *	book-keeping alone, waiting to atomically switch references from
 *	one client to the other at de-encapsulation time.
 *	
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Release a reference on the handle header.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileMigClose(hdrPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
{
    panic( "Fsio_FileMigClose called\n");
    Fsutil_HandleRelease(hdrPtr, FALSE);
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_FileMigOpen --
 *
 *	Complete setup of a stream to a local file after migration to the
 *	file server.  Fsio_FileMigrate has done the work of shifting use
 *	counts at the stream and I/O handle level.  This routine has to
 *	increment the low level I/O handle reference count to reflect
 *	the existence of a new stream to the I/O handle.
 *
 * Results:
 *	SUCCESS or FS_FILE_NOT_FOUND if the I/O handle can't be set up.
 *
 * Side effects:
 *	Gains one reference to the I/O handle.  Frees the client data.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileMigOpen(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* sizeof(Fsio_FileState), IGNORED */
    ClientData	data;		/* referenced to Fsio_FileState */
    Fs_HandleHeader **hdrPtrPtr;	/* Return - I/O handle for the file */
{
    register ReturnStatus status;
    register Fsio_FileIOHandle *handlePtr;

    handlePtr = Fsutil_HandleFetchType(Fsio_FileIOHandle,
		&migInfoPtr->ioFileID);
    if (handlePtr == (Fsio_FileIOHandle *)NIL) {
	printf("Fsio_FileMigOpen, file <%d,%d> from client %d not found\n",
	    migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor,
	    migInfoPtr->srcClientID);
	status = FS_FILE_NOT_FOUND;
    } else {
	Fsutil_HandleUnlock(handlePtr);
	*hdrPtrPtr = (Fs_HandleHeader *)handlePtr;
	status = SUCCESS;
    }
    free((Address)data);
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_FileMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	Three things are done:  cache consistency actions are taken to
 *	reflect the movement of the client, file state is set up for use
 *	on the client in the MigEnd procedure, and cross-network stream
 *	sharing is detected.  A useful side-effect of this routine is
 *	to properly set the type in the ioFileID, either FSIO_LCL_FILE_STREAM
 *	or FSIO_RMT_FILE_STREAM.  In the latter case FsrmtFileMigrate
 *	is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up or if there
 *	is a cache consistency failure.  Otherwise SUCCESS is returned,
 *	*flagsPtr may have the FS_RMT_SHARED bit set, and *sizePtr
 *	and *dataPtr are set to reference Fsio_FileState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return Fsio_FileState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - correct stream offset */
    int		*sizePtr;	/* Return - sizeof(Fsio_FileState) */
    Address	*dataPtr;	/* Return - pointer to Fsio_FileState */
{
    register Fsio_FileIOHandle	*handlePtr;
    register Fsio_FileState		*fileStatePtr;
    register ReturnStatus		status;
    Boolean				closeSrcClient;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The file was local, which is why we were called, but is now remote.
	 */
	migInfoPtr->ioFileID.type = FSIO_RMT_FILE_STREAM;
	return(FsrmtFileMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_LCL_FILE_STREAM;
    handlePtr = Fsutil_HandleFetchType(Fsio_FileIOHandle, &migInfoPtr->ioFileID);
    if (handlePtr == (Fsio_FileIOHandle *)NIL) {
	panic("Fsio_FileMigrate, no I/O handle");
	status = FS_STALE_HANDLE;
    } else {

	/*
	 * At the stream level, add the new client to the set of clients
	 * for the stream, and check for any cross-network stream sharing.
	 * We only close the orignial client if the stream is unshared,
	 * i.e. there are no references left there.
	 */
	Fsio_StreamMigClient(migInfoPtr, dstClientID, (Fs_HandleHeader *)handlePtr,
			&closeSrcClient);

	/*
	 * Adjust use counts on the I/O handle to reflect any new sharing.
	 */
	Fsio_MigrateUseCounts(migInfoPtr->flags, closeSrcClient, &handlePtr->use);

	/*
	 * Update the client list, and take any required cache consistency
	 * actions. The handle returns unlocked from the consistency routine.
	 */
	fileStatePtr = mnew(Fsio_FileState);
	Fsutil_HandleUnlock(handlePtr);
	status = Fsconsist_MigrateConsistency(handlePtr, migInfoPtr->srcClientID,
		dstClientID, migInfoPtr->flags, closeSrcClient,
		&fileStatePtr->cacheable, &fileStatePtr->openTimeStamp);
	if (status == SUCCESS) {
	    Fscache_GetCachedAttr(&handlePtr->cacheInfo, &fileStatePtr->version,
				&fileStatePtr->attr);
	    *sizePtr = sizeof(Fsio_FileState);
	    *dataPtr = (Address)fileStatePtr;
	    *flagsPtr = fileStatePtr->newUseFlags = migInfoPtr->flags;
	    *offsetPtr = migInfoPtr->offset;
	} else {
	    free((Address)fileStatePtr);
	}
	/*
	 * We don't need this reference on the I/O handle, there is no change.
	 */
	Fsutil_HandleRelease(handlePtr, FALSE);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileRead --
 *
 *	Read from a file.  This is a thin layer on top of the cache
 *	read routine.
 *
 * Results:
 *	The results of Fscache_Read.
 *
 * Side effects:
 *	None, because Fscache_Read does most everything.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_FileRead(streamPtr, readPtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the file. */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    status = Fscache_Read(&handlePtr->cacheInfo, readPtr->flags, readPtr->buffer,
			    readPtr->offset, &readPtr->length, remoteWaitPtr);
    replyPtr->length = readPtr->length;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileWrite --
 *
 *	Write to a disk file.  This is a thin layer on top of the cache
 *	write routine.  Besides doing the write, this routine synchronizes
 *	with read ahead on the file.
 *
 * Results:
 *	The results of Fscache_Write.
 *
 * Side effects:
 *	The handle is locked during the I/O.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_FileWrite(streamPtr, writePtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the file. */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    /*
     * Get a reference to the domain so it can't be dismounted during the I/O.
     * Then synchronize with read ahead before actually doing the write.
     */
    if (Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE) == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    FscacheWaitForReadAhead(&handlePtr->readAhead);
    status = Fscache_Write(&handlePtr->cacheInfo, writePtr->flags,
			  writePtr->buffer, writePtr->offset,
			  &writePtr->length, remoteWaitPtr);
    replyPtr->length = writePtr->length;
    if (status == SUCCESS && (fsutil_WriteThrough || fsutil_WriteBackASAP)) {
	/*
	 * When in write-through or asap mode we have to force the descriptor
	 * to disk on every write.
	 */
	status = Fsdm_FileDescWriteBack(handlePtr, FALSE);
    }

    FscacheAllowReadAhead(&handlePtr->readAhead);
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileBlockRead --
 *
 *	Read a block from the local disk.  This is called when the cache
 *	needs data, and from the paging routines.  Synchronization with
 *	other I/O should is done at a higher level, ie. the cache.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the bufSize parameter.  *readCountPtr is filled with the number
 *	of bytes actually read.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileBlockRead(hdrPtr, flags, buffer, offsetPtr,  lenPtr, remoteWaitPtr)
    Fs_HandleHeader	*hdrPtr;	/* Handle on a local file. */
    int			flags;		/* IGNORED */
    register Address	buffer;		/* Where to read into. */
    int 		*offsetPtr;	/* Byte offset at which to read.  Return
					 * value isn't used, but by-reference
					 * passing is done to be compatible
					 * with Fsrmt_Read */
    int			*lenPtr;	/* Return,  bytes read. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* IGNORED */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)hdrPtr;
    register	Fsdm_Domain	 *domainPtr;
    register	Fsdm_FileDescriptor *descPtr;
    register			 offset = *offsetPtr;
    register int		 numBytes;
    ReturnStatus		 status;
    Fsdm_BlockIndexInfo		 indexInfo;

    numBytes = *lenPtr;
    if ((offset & FS_BLOCK_OFFSET_MASK) != 0) {
	panic("Fsio_FileBlockRead: Non-block aligned offset\n");
    }
    if (numBytes > FS_BLOCK_SIZE) {
	panic("Fsio_FileBlockRead: Reading more than block\n");
    }

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *) NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    if (handlePtr->hdr.fileID.minor == 0) {
	/*
	 * If is a physical block address then read it in directly.
	 */
	status = Fsio_DeviceBlockIO(FS_READ, &domainPtr->headerPtr->device,
			   offset / FS_FRAGMENT_SIZE, FS_FRAGMENTS_PER_BLOCK, 
			   buffer);
	fs_Stats.gen.physBytesRead += FS_BLOCK_SIZE;
    } else {
	/*
	 * Is a logical file read. Round the size down to the actual
	 * last byte in the file.
	 */

	descPtr = handlePtr->descPtr;
	if (offset > descPtr->lastByte) {
	    *lenPtr = 0;
	    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
	    return(SUCCESS);
	} else if (offset + numBytes - 1 > descPtr->lastByte) {
	    numBytes = descPtr->lastByte - offset + 1;
	}

	status = Fsdm_GetFirstIndex(handlePtr, offset / FS_BLOCK_SIZE, 
				 &indexInfo, 0);
	if (status != SUCCESS) {
	    printf("Fsio_FileRead: Could not setup indexing\n");
	    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
	    return(status);
	}

	if (indexInfo.blockAddrPtr != (int *) NIL &&
	    *indexInfo.blockAddrPtr != FSDM_NIL_INDEX) {
	    /*
	     * Read in the block.
	     */
	    status = Fsio_DeviceBlockIO(FS_READ, &domainPtr->headerPtr->device,
		      *indexInfo.blockAddrPtr +
		      domainPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
		      (numBytes - 1) / FS_FRAGMENT_SIZE + 1, buffer);
	} else {
	    /*
	     * Zero fill the block.  We're in a 'hole' in the file.
	     */
	    bzero(buffer, numBytes);
	}
	Fsdm_EndIndex(handlePtr, &indexInfo, FALSE);
	Fs_StatAdd(numBytes, fs_Stats.gen.fileBytesRead,
		   fs_Stats.gen.fileReadOverflow);
#ifndef CLEAN
	if (fsdmKeepTypeInfo) {
	    int fileType;
	
	    fileType = Fsdm_FindFileType(&handlePtr->cacheInfo);
	    fs_TypeStats.diskBytes[FS_STAT_READ][fileType] += numBytes;
	}
#endif CLEAN
    }

    if (status == SUCCESS) {
	*lenPtr = numBytes;
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileBlockWrite --
 *
 *	Write a block given a disk block number to a normal file, directory, 
 *	or symbolic link.
 *
 * Results:
 *	The return code from the driver, or FS_DOMAIN_UNAVAILABLE if
 *	the domain has been un-attached.
 *
 * Side effects:
 *	The device write.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileBlockWrite(hdrPtr, blockAddr, numBytes, buffer, flags)
    Fs_HandleHeader *hdrPtr;	/* Pointer to handle for file to write to. */
    int 	blockAddr;	/* Disk address. For regular files this
				 * counts from the beginning of the data blocks.
				 * For raw I/O they count from the start
				 * of the disk partition. */
    int		numBytes;	/* Number of bytes to write. */
    register Address buffer;	/* Where to read bytes from. */
    int		flags;		/* IGNORED */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)hdrPtr;
    register	Fsdm_Domain	 *domainPtr;
    ReturnStatus		status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    if (handlePtr->hdr.fileID.minor == 0 || blockAddr < 0) {
	/*
	 * The block number is a raw block number counting from the
	 * beginning of the domain.
	 * Descriptor blocks are indicated by a handle with a 0 file number 
	 * and indirect a negative block number (indirect blocks).
	 */
	if (blockAddr < 0) {
	    blockAddr = -blockAddr;
	}
	fs_Stats.gen.physBytesWritten += numBytes;
	status = Fsio_DeviceBlockIO(FS_WRITE, &domainPtr->headerPtr->device,
			 blockAddr, FS_FRAGMENTS_PER_BLOCK, buffer);
    } else {
	/*
	 * The block number is relative to the start of the data blocks.
	 */
	Fs_StatAdd(numBytes, fs_Stats.gen.fileBytesWritten,
		   fs_Stats.gen.fileWriteOverflow);
#ifndef CLEAN
	if (fsdmKeepTypeInfo) {
	    int fileType;
	
	    fileType = Fsdm_FindFileType(&handlePtr->cacheInfo);
	    fs_TypeStats.diskBytes[FS_STAT_WRITE][fileType] += numBytes;
	}
#endif CLEAN
	status = Fsio_DeviceBlockIO(FS_WRITE, &domainPtr->headerPtr->device,
	       blockAddr + 
	       domainPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
	       (numBytes - 1) / FS_FRAGMENT_SIZE + 1, buffer);
    }
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileBlockCopy --
 *
 *	Copy the block from the source swap file to the destination swap file.
 *
 *	NOTE: This routine does not call the routine that puts swap file blocks
 *	      on the front of the free list.  This is because the general
 *	      mode of doing things is to fork which copies the swap file and
 *	      then exec which removes it.  Thus we want the swap file to be
 *	      in the cache for the copy and we don't have to put the 
 *	      destination files blocks on front of the lru list because it
 *	      is going to get removed real soon anyway.
 *
 * Results:
 *	Error code if couldn't allocate disk space or didn't read a full
 *	block.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileBlockCopy(srcHdrPtr, dstHdrPtr, blockNum)
    Fs_HandleHeader	*srcHdrPtr;	/* File to copy block from.  */
    Fs_HandleHeader	*dstHdrPtr;	/* File to copy block to.  */
    int			blockNum;	/* Block to copy. */
{
    int			offset;
    ReturnStatus	status;
    Fscache_Block	*cacheBlockPtr;
    int			numBytes;
    register Fsio_FileIOHandle *srcHandlePtr =
	    (Fsio_FileIOHandle *)srcHdrPtr;
    register Fsio_FileIOHandle *dstHandlePtr =
	    (Fsio_FileIOHandle *)dstHdrPtr;

    /*
     * Look in the cache for the source block.
     */
    status = Fscache_BlockRead(&srcHandlePtr->cacheInfo, blockNum,
		    &cacheBlockPtr, &numBytes, FSCACHE_DATA_BLOCK, FALSE);
    if (status != SUCCESS) {
	return(status);
    }
    if (numBytes != FS_BLOCK_SIZE) {
	if (numBytes != 0) {
	    Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
	}
	return(VM_SHORT_READ);
    }
    /*
     * Write to the destination block.
     */
    numBytes = FS_BLOCK_SIZE;
    offset = blockNum * FS_BLOCK_SIZE;
    status = Fscache_Write(&dstHandlePtr->cacheInfo, FALSE,
		    cacheBlockPtr->blockAddr, offset, &numBytes,
		    (Sync_RemoteWaiter *) NIL);
    if (status == SUCCESS && numBytes != FS_BLOCK_SIZE) {

	status = VM_SHORT_WRITE;
    }

    Fscache_UnlockBlock(cacheBlockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);

    srcHandlePtr->cacheInfo.attr.accessTime = fsutil_TimeInSeconds;
    dstHandlePtr->cacheInfo.attr.modifyTime = fsutil_TimeInSeconds;

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileIOControl --
 *
 *	IOControls for regular files.  The handle should be locked up entry.
 *	This handles byte swapping of its input and output buffers if
 *	the clients byte ordering/padding is different.
 *
 * Results:
 *	An error code from the command.
 *
 * Side effects:
 *	Command dependent.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Fsio_FileIOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream *streamPtr;		/* Stream to local file */
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register Fsio_FileIOHandle *handlePtr =
	    (Fsio_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status = SUCCESS;

    Fsutil_HandleLock(handlePtr);
    switch(ioctlPtr->command) {
	case IOC_REPOSITION:
	    break;
	case IOC_GET_FLAGS:
	    if ((ioctlPtr->outBufSize >= sizeof(int)) &&
		(ioctlPtr->outBuffer != (Address)NIL)) {
		*(int *)ioctlPtr->outBuffer = 0;
	    }
	    break;
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	    break;
	case IOC_TRUNCATE: {
	    int length;

	    if (ioctlPtr->inBufSize < sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else if (ioctlPtr->format != mach_Format) {
		int outSize = sizeof(int);
		int inSize = sizeof(int);
		int fmtStatus;
		fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize, 
				ioctlPtr->inBuffer, mach_Format, &outSize,
				(Address) &length);
		if (fmtStatus != 0) {
		    printf("Format of ioctl failed <0x%x>\n", fmtStatus);
		    status = GEN_INVALID_ARG;
		}
		if (outSize != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else {
		length = *(int *)ioctlPtr->inBuffer;
	    }
	    if (status == SUCCESS) {
		if (length < 0) {
		    status = GEN_INVALID_ARG;
		} else {
		    status = Fsio_FileTrunc(handlePtr, length, 0);
		}
	    }
	    break;
	}
	case IOC_LOCK:
	case IOC_UNLOCK:
	    status = Fsio_IocLock(&handlePtr->lock, ioctlPtr,
				&streamPtr->hdr.fileID);
	    break;
	case IOC_NUM_READABLE: {
	    /*
	     * Return the number of bytes available to read.  The top-level
	     * IOControl routine has put the current stream offset in inBuffer.
	     */
	    int bytesAvailable;
	    int streamOffset;
	    int size;

	    if (ioctlPtr->inBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else if (ioctlPtr->format != mach_Format) {
		int fmtStatus;
		int inSize;
		inSize = ioctlPtr->inBufSize;
		fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize, 
				ioctlPtr->inBuffer, mach_Format, &size,
				(Address) &streamOffset);
		if (fmtStatus != 0) {
		    printf("Format of ioctl failed <0x%x>\n", fmtStatus);
		    status = GEN_INVALID_ARG;
		}
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else {
		streamOffset = *(int *)ioctlPtr->inBuffer;
	    }
	    if (status == SUCCESS) {
		bytesAvailable = handlePtr->cacheInfo.attr.lastByte + 1 -
				streamOffset;
		if (ioctlPtr->outBufSize != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		} else if (ioctlPtr->format != mach_Format) {
		    int fmtStatus;
		    int	inSize;
		    inSize = sizeof(int);
		    fmtStatus = Fmt_Convert("w", mach_Format, &inSize, 
				    (Address) &bytesAvailable, ioctlPtr->format,
				    &size, ioctlPtr->outBuffer);
		    if (fmtStatus != 0) {
			printf("Format of ioctl failed <0x%x>\n", fmtStatus);
			status = GEN_INVALID_ARG;
		    }
		    if (size != sizeof(int)) {
			status = GEN_INVALID_ARG;
		    }
		} else {
		    *(int *)ioctlPtr->outBuffer = bytesAvailable;
		}
	    }
	    break;
	}
	case IOC_SET_OWNER:
	case IOC_GET_OWNER:
	case IOC_MAP:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_PREFIX:
	    break;
	default:
	    status = GEN_INVALID_ARG;
	    break;
    }
    Fsutil_HandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileTrunc --
 *
 *	Shorten a file to length bytes.  This calls routines to update
 *	the cacheInfo and the fileDescriptor.
 *
 * Results:
 *	Error status from Fsdm_FileDescTrunc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsio_FileTrunc(handlePtr, size, flags)
    Fsio_FileIOHandle	*handlePtr;	/* File to truncate. */
    int			size;		/* Size to truncate the file to. */
    int			flags;		/* FSCACHE_TRUNC_DELETE */
{
    ReturnStatus status;
    Fscache_Trunc(&handlePtr->cacheInfo, size, flags);
    status = Fsdm_FileDescTrunc(handlePtr, size);
    if ((flags & FSCACHE_TRUNC_DELETE) && handlePtr->cacheInfo.blocksInCache > 0) {
	panic("Fsio_FileTrunc (delete) %d blocks left over\n",
		    handlePtr->cacheInfo.blocksInCache);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_FileSelect --
 *
 *	Always returns that file is readable and writable.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_FileSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    Fs_HandleHeader *hdrPtr;	/* The handle of the file */
    Sync_RemoteWaiter *waitPtr;	/* Process info for waiting */
    int		*readPtr;	/* Read bit */
    int		*writePtr;	/* Write bit */
    int		*exceptPtr;	/* Exception bit */
{
    *exceptPtr = 0;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------------
 *
 * IncVersionNumber --
 *
 *	Increment the version number on file.  This is done when a file
 *	is opened for writing, and the version number is used by clients
 *	to verify their caches.  This must be called with the handle locked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Version number incremented and the descriptor is pushed to disk.
 *
 *----------------------------------------------------------------------------
 *
 */
void
IncVersionNumber(handlePtr)
    Fsio_FileIOHandle	*handlePtr;
{
    Fsdm_FileDescriptor	*descPtr;
    Fsdm_Domain		*domainPtr;

    descPtr = handlePtr->descPtr;
    descPtr->version++;
    handlePtr->cacheInfo.version = descPtr->version;
    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	printf( "FsIncVersionNumber: Domain gone.\n");
    } else {
	(void)Fsdm_FileDescStore(domainPtr, handlePtr->hdr.fileID.minor,
			descPtr);
	Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    }
    Vm_FileChanged(&handlePtr->segPtr);
}
