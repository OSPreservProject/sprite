/* 
 * fsFile.c --
 *
 *	Routines for operations on files.  A file handle is identified
 *	by using the <major> field of the FsFileID for the domain index,
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
#include "fsInt.h"
#include "fsFile.h"
#include "fsDevice.h"
#include "fsLocalDomain.h"
#include "fsMigrate.h"
#include "fsOpTable.h"
#include "fsBlockCache.h"
#include "fsCacheOps.h"
#include "fsConsist.h"
#include "fsStream.h"
#include "fsPrefix.h"
#include "fsLock.h"
#include "fsDisk.h"
#include "fsStat.h"
#include "vm.h"
#include "rpc.h"
#include "recov.h"

void IncVersionNumber();


/*
 *----------------------------------------------------------------------
 *
 * FsLocalFileHandleInit --
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
FsLocalFileHandleInit(fileIDPtr, name, newHandlePtrPtr)
    FsFileID	*fileIDPtr;
    char	*name;
    FsLocalFileIOHandle	**newHandlePtrPtr;
{
    register ReturnStatus status;
    register FsLocalFileIOHandle *handlePtr;
    register FsFileDescriptor *descPtr;
    register FsDomain *domainPtr;
    register Boolean found;

    found = FsHandleInstall(fileIDPtr, sizeof(FsLocalFileIOHandle), name,
		    (FsHandleHeader **)newHandlePtrPtr);
    if (found) {
	/*
	 * All set.
	 */
	return(SUCCESS);
    }
    /*
     * Get a hold of the disk file descriptor.
     */
    handlePtr = *newHandlePtrPtr;
    domainPtr = FsDomainFetch(fileIDPtr->major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	FsHandleRelease(handlePtr, TRUE);
	FsHandleRemove(handlePtr);
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = (FsFileDescriptor *)Mem_Alloc(sizeof(FsFileDescriptor));
    status = FsFetchFileDesc(domainPtr, fileIDPtr->minor, descPtr);
    FsDomainRelease(fileIDPtr->major);

    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsLocalFileHandleInit: FsFetchFileDesc failed");
    } else if (!(descPtr->flags & FS_FD_ALLOC)) {
	status = FS_FILE_REMOVED;
    } else {
	FsCachedAttributes attr;

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

	FsCacheInfoInit(&handlePtr->cacheInfo, (FsHandleHeader *)handlePtr,
		descPtr->version, TRUE, &attr);

	FsConsistInit(&handlePtr->consist, (FsHandleHeader *)handlePtr);
	FsLockInit(&handlePtr->lock);
	FsReadAheadInit(&handlePtr->readAhead);

	handlePtr->segPtr = (Vm_Segment *)NIL;
    }
    if (status != SUCCESS) {
	FsHandleRelease(handlePtr, TRUE);
	FsHandleRemove(handlePtr);
	Mem_Free((Address)descPtr);
	*newHandlePtrPtr = (FsLocalFileIOHandle *)NIL;
    } else {
	*newHandlePtrPtr = handlePtr;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileSrvOpen --
 *
 *	This is called in two cases after name lookup on the server.
 *	The first is when a client is opening the file from Fs_Open.
 *	The second is when a lookup is done when getting/setting the
 *	attributes of the files.  In the open case this routine has
 *	to set up FsFileState that the client will use to complete
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
FsFileSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
	dataSizePtr, clientDataPtr)
     register FsLocalFileIOHandle *handlePtr;	/* A handle from FsLocalLookup.
					 * Should be LOCKED upon entry,
					 * Returned UNLOCKED. */
     int		clientID;	/* Host ID of client doing the open */
     register int	useFlags;	/* FS_READ | FS_WRITE | FS_EXECUTE */
     FsFileID		*ioFileIDPtr;	/* Return - same as handle file ID */
     FsFileID		*streamIDPtr;	/* Return ID of stream to the file. 
					 * NIL during set/get attributes */
     int		*dataSizePtr;	/* Return - sizeof(FsFileState) */
     ClientData		*clientDataPtr;	/* Return - a reference to FsFileState
					 * used by FsCltFileOpen.  Nothing is
					 * returned during set/get attrs */
{
    FsFileState *fileStatePtr;
    ReturnStatus status;

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
    *ioFileIDPtr = handlePtr->hdr.fileID;
    if (clientID != rpc_SpriteID) {
	ioFileIDPtr->type = FS_RMT_FILE_STREAM;
    }
    if (clientDataPtr == (ClientData *)NIL) { 
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
	FsHandleUnlock(handlePtr);
	fileStatePtr = Mem_New(FsFileState);
	status = FsFileConsistency(handlePtr, clientID, useFlags,
		    &fileStatePtr->cacheable, &fileStatePtr->openTimeStamp);
	if (status == SUCCESS) {
	    /*
	     * Copy cached attributes into the returned file state.
	     */
	    FsGetCachedAttr(&handlePtr->cacheInfo, &fileStatePtr->version,
			&fileStatePtr->attr);
	    /*
	     * Return new usage flags to the client.  This lets us strip
	     * off the execute use flag (above, for directories) so
	     * the client doesn't have to worry about it.
	     */
	    fileStatePtr->newUseFlags = useFlags;
	    *clientDataPtr = (ClientData)fileStatePtr;
	    *dataSizePtr = sizeof(FsFileState);

	    /*
	     * Now set up a shadow stream on here on the server so we
	     * can support shared offset even after migration.
	     * Note: prefix handles get opened, but no stream is set
	     * up for them as there is never an offset for them, hence
	     * this check against a NIL streamID pointer.
	     */
	    if (streamIDPtr != (FsFileID *)NIL) {
		register Fs_Stream *streamPtr;

		streamPtr = FsStreamNew(rpc_SpriteID,
		    (FsHandleHeader *)handlePtr, useFlags, handlePtr->hdr.name);
		(void)FsStreamClientOpen(&streamPtr->clientList, clientID,
					 useFlags);
		*streamIDPtr = streamPtr->hdr.fileID;
		FsHandleRelease(streamPtr, TRUE);
	    }
	    return(SUCCESS);
	} else {
	    /*
	     * Consistency call-backs failed because the last writer
	     * could not write back its copy of the file. We garbage
	     * collect the client to retreat to a known bookkeeping point.
	     */
	    int ref, write, exec;
	    Sys_Panic(SYS_WARNING, "Consistency failed %x on <%d,%d>\n",
		status,
		handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor);
	    FsHandleLock(handlePtr);
	    FsConsistKill(&handlePtr->consist, clientID, &ref, &write, &exec);
	    handlePtr->use.ref   -= ref;
	    handlePtr->use.write -= write;
	    handlePtr->use.exec  -= exec;
	    Mem_Free((Address)fileStatePtr);
	}
    }
exit:
    FsHandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileReopen --
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
FsFileReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;	/* IGNORED here on the server */
    int			clientID;	/* Client doing the reopen */
    ClientData		inData;		/* FsFileReopenParams */
    int			*outSizePtr;	/* Size of returned data */
    ClientData		*outDataPtr;	/* Returned data */
{
    register FsFileReopenParams *reopenParamsPtr; /* Parameters from RPC */
    register FsFileState	*fileStatePtr;	/* Results for RPC */
    FsLocalFileIOHandle	    	*handlePtr;	/* Local handle for file */
    register ReturnStatus	status = SUCCESS; /* General return code */
    FsDomain			*domainPtr;

    *outDataPtr = (ClientData) NIL;
    *outSizePtr = 0;
    /*
     * Do initial setup for the reopen.  We make sure that the disk
     * for the file is still around first, mark the client
     * as doing recovery, and fetch a local handle for the file.
     * NAME note: we have no name for the file after a re-open.
     */
    reopenParamsPtr = (FsFileReopenParams *) inData;
    domainPtr = FsDomainFetch(reopenParamsPtr->fileID.major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    status = FsLocalFileHandleInit(&reopenParamsPtr->fileID, (char *)NIL,
	&handlePtr);
    if (status != SUCCESS) {
	goto reopenReturn;
    }
    /*
     * See if the client can still cache its dirty blocks.
     */
    if (reopenParamsPtr->haveDirtyBlocks) {
	status = FsCacheCheckVersion(&handlePtr->cacheInfo,
				     reopenParamsPtr->version, clientID);
	if (status != SUCCESS) {
	    FsHandleRelease(handlePtr, TRUE);
	    goto reopenReturn;
	}
    }
    /*
     * Update global use counts and version number.
     */
    FsReopenClient(handlePtr, clientID, reopenParamsPtr->use,
			    reopenParamsPtr->haveDirtyBlocks);
    if (reopenParamsPtr->use.write > 0) {
	IncVersionNumber(handlePtr);
    }
    /*
     * Now unlock the handle and do cache consistency call-backs.
     */
    fileStatePtr = Mem_New(FsFileState);
    fileStatePtr->cacheable = reopenParamsPtr->haveDirtyBlocks;
    FsHandleUnlock(handlePtr);
    status = FsReopenConsistency(handlePtr, clientID, reopenParamsPtr->use,
		&fileStatePtr->cacheable, &fileStatePtr->openTimeStamp);
    if (status != SUCCESS) {
	/*
	 * Consistency call-backs failed, probably due to disk-full.
	 * We kill the client here as it will invalidate its handle
	 * after this re-open fails.
	 */
	int ref, write, exec;
	FsHandleLock(handlePtr);
	FsConsistKill(&handlePtr->consist, clientID, &ref, &write, &exec);
	handlePtr->use.ref   -= ref;
	handlePtr->use.write -= write;
	handlePtr->use.exec  -= exec;
	FsHandleUnlock(handlePtr);
	Mem_Free((Address)fileStatePtr);
    } else {
	/*
	 * Successful re-open here on the server. Copy cached attributes
	 * into the returned file state.
	 */
	FsGetCachedAttr(&handlePtr->cacheInfo, &fileStatePtr->version,
		    &fileStatePtr->attr);
	fileStatePtr->newUseFlags = 0;		/* Not used in re-open */
	*outDataPtr = (ClientData) fileStatePtr;
	*outSizePtr = sizeof(FsFileState);
    }
    FsHandleRelease(handlePtr, FALSE);
reopenReturn:
    FsDomainRelease(reopenParamsPtr->fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileCltOpen --
 *
 *      Set up a stream for a local disk file.  This is called from Fs_Open to
 *	complete the opening of a stream.  By this time any cache consistency
 *	actions have already been taken, and local use counts have been
 *	incremented by FsFileSrvOpen.
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
FsFileCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    FsFileID		*ioFileIDPtr;	/* I/O fileID from the name server */
    int			*flagsPtr;	/* Return only.  The server returns
					 * a modified useFlags in FsFileState */
    int			clientID;	/* IGNORED */
    ClientData		streamData;	/* FsFileState. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a handle set up for
					 * I/O to a file, NIL if failure. */
{
    register ReturnStatus	status;

    status = FsLocalFileHandleInit(ioFileIDPtr, name,
		(FsLocalFileIOHandle **)ioHandlePtrPtr);
    if (status == SUCCESS) {
	/*
	 * Return the new useFlags from the server.  It has stripped off
	 * execute permission for directories.
	 */
	*flagsPtr = ( (FsFileState *)streamData )->newUseFlags;
	FsHandleUnlock(*ioHandlePtrPtr);
    }
    Mem_Free((Address)streamData);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileClose --
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
FsFileClose(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to regular file */
    int			clientID;	/* Host ID of closer */
    Proc_PID		procID;		/* Process ID of closer */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of closeData */
    ClientData		closeData;	/* Ref. to FsCachedAttributes */
{
    register FsLocalFileIOHandle *handlePtr =
	    (FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus		status;
    Boolean			wasCached = TRUE;

    /*
     * Update the client state to reflect the close by the client.
     */
    if (!FsConsistClose(&handlePtr->consist, clientID, flags, &wasCached)) {
	Sys_Panic(SYS_WARNING,
		  "FsFileClose, client %d unknown for file <%d,%d>\n",
		  clientID, handlePtr->hdr.fileID.major,
		  handlePtr->hdr.fileID.minor);
	FsHandleUnlock(handlePtr);
	return(FS_STALE_HANDLE);
    }
    if (wasCached && dataSize != 0) {
	/*
	 * Update the server's attributes from ones cached on the client.
	 */
	FsUpdateAttrFromClient(&handlePtr->cacheInfo,
				(FsCachedAttributes *)closeData);
    }

    /*
     * THE REST OF THIS ROUTINE LOOKS Almost LIKE FsFileClientKill.
     * The main difference is that we have a low-level reference to release.
     */

    FsLockClose(&handlePtr->lock, procID, &streamPtr->hdr.fileID);
    /*
     * Update the global/summary use counts for the file.
     */
    handlePtr->use.ref--;
    if (flags & FS_WRITE) {
	handlePtr->use.write--;
    }
    if (flags & FS_EXECUTE) {
	handlePtr->use.exec--;
    }
    if (handlePtr->use.ref < 0 || handlePtr->use.write < 0 ||
	handlePtr->use.exec < 0) {
	Sys_Panic(SYS_FATAL, "FsFileClose <%d,%d> use %d, write %d, exec %d\n",
	    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
	    handlePtr->use.ref, handlePtr->use.write, handlePtr->use.exec);
    }
    /*
     * Force the file to disk if we are told to do so by a client.
     */
    if (flags & FS_WB_ON_LDB) {
	int blocksSkipped;
	/*
	 * Force this files blocks to disk.
	 */
	(void)FsCacheFileWriteBack(&handlePtr->cacheInfo, 0, 
				FS_LAST_BLOCK,
				FS_FILE_WB_WAIT | FS_FILE_WB_INDIRECT,
				&blocksSkipped);
	FsWriteBackDesc(handlePtr, TRUE);
    }

    /*
     * Handle pending deletes
     *	1. We scan the client list and call-back to the last writer if
     *		it is not the client doing the close.
     *	2. We mark the disk descriptor as deleted,
     *	3. We return FS_FILE_REMOVED to the calling client so it knows
     *		to nuke its cache.
     */
    if ((handlePtr->use.ref == 0) && (handlePtr->flags & FS_FILE_DELETED)) {
	FsClientRemoveCallback(&handlePtr->consist, clientID);
	status = FsDeleteFileDesc(handlePtr);
	FsHandleRelease(handlePtr, TRUE);
	FsHandleRemove(handlePtr);
	if (clientID != rpc_SpriteID && status == SUCCESS) {
	    status = FS_FILE_REMOVED;
	}
    } else {
	FsHandleRelease(handlePtr, TRUE);
	status = SUCCESS;
    }

    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsFileClientKill --
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
FsFileClientKill(hdrPtr, clientID)
    FsHandleHeader	*hdrPtr;	/* File to clean up */
    int			clientID;	/* Host assumed down */
{
    FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;
    int refs, writes, execs;

    FsIOClientKill(&handlePtr->consist.clientList, clientID,
		    &refs, &writes, &execs);
    /*
     * THIS LOOKS MUCH LIKE THE LAST PART OF FsFileClose.
     * MAINTAIN BOTH ROUTINES PLEASE.  The difference between them is
     * that here we are removing the client from the client list, and
     * we have no low-level reference count on the handle to release.
     */
    FsLockClientKill(&handlePtr->lock, clientID);

    handlePtr->use.ref -= refs;
    handlePtr->use.write -= writes;
    handlePtr->use.exec -= execs;
    if (handlePtr->use.ref < 0 || handlePtr->use.write < 0 ||
	handlePtr->use.exec < 0) {
	Sys_Panic(SYS_FATAL, "FsFileClientKill <%d,%d> use %d, write %d, exec %d\n",
	    hdrPtr->fileID.major, hdrPtr->fileID.minor,
	    handlePtr->use.ref, handlePtr->use.write, handlePtr->use.exec);
    }
    /*
     * Handle pending deletes.  We mark the disk descriptor as deleted
     * and tell other clients about the remove.
     */
    if ((handlePtr->use.ref == 0) && (handlePtr->flags & FS_FILE_DELETED)) {
	FsClientRemoveCallback(&handlePtr->consist, clientID);
	(void)FsDeleteFileDesc(handlePtr);
	FsHandleRemove(handlePtr);
    } else {
	FsHandleUnlock(handlePtr);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsFileScavenge --
 *
 *	Called periodically to see if this handle is still needed.
 *	
 *
 * Results:
 *	SUCCESS.
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
void
FsFileScavenge(hdrPtr)
    FsHandleHeader	*hdrPtr;	/* File to clean up */
{
    register FsLocalFileIOHandle *handlePtr = (FsLocalFileIOHandle *)hdrPtr;
    register FsFileDescriptor *descPtr = handlePtr->descPtr;
    FsDomain *domainPtr;
    ReturnStatus status;

    /*
     * Write back the descriptor in case we decide below to remove the handle.
     */
    if (descPtr->flags & FS_FD_DIRTY) {
	descPtr->flags &= ~FS_FD_DIRTY;
	domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
	if (domainPtr == (FsDomain *)NIL ){
	    Sys_Panic(SYS_FATAL,
		"FsFileScavenge: Dirty descriptor in detached domain.\n");
	} else {
	    status = FsStoreFileDesc(domainPtr, handlePtr->hdr.fileID.minor, 
				      descPtr);
	    FsDomainRelease(handlePtr->hdr.fileID.major);
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING,
		"FsFileScavenge: Could not store file desc <%x>\n",
			status);
	    }
	}
    }

    if (handlePtr->use.ref == 0 &&
	handlePtr->cacheInfo.blocksInCache == 0 &&
	FsConsistClients(&handlePtr->consist) == 0) {
#ifdef CONSIST_DEBUG
	extern int fsTraceConsistMinor;
	if (fsTraceConsistMinor == handlePtr->hdr.fileID.minor) {
	    Sys_Printf("FsFileScavenge <%d,%d> nuked, lastwriter %d\n",
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
	FsHandleAttemptRemove(handlePtr);
    } else {
	FsHandleUnlock(hdrPtr);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsFileMigStart --
 *
 *	Initiate migration of a FS_LCL_FILE_STREAM.  There is no extra
 *	state needed than already put together by Fs_EncapStream.  However,
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
FsFileMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* migration data, NOT_USED */
{
    if ((flags & FS_RMT_SHARED) == 0) {
	FsHandleRelease(hdrPtr, FALSE);
    }
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsFileMigEnd --
 *
 *	Complete setup of a stream to a local file after migration.  The
 *	srvMigrate routine has done most of the work.  We just need to
 *	get a reference on the I/O handle for the stream.
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
FsFileMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* sizeof(FsFileState), IGNORED */
    ClientData	data;		/* referenced to FsFileState */
    FsHandleHeader **hdrPtrPtr;	/* Return - I/O handle for the file */
{
    register ReturnStatus status;
    register FsLocalFileIOHandle *handlePtr;

    handlePtr = FsHandleFetchType(FsLocalFileIOHandle,
		&migInfoPtr->ioFileID);
    if (handlePtr == (FsLocalFileIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING, 
	    "FsFileMigEnd, file <%d,%d> from client %d not found\n",
	    migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor,
	    migInfoPtr->srcClientID);
	status = FS_FILE_NOT_FOUND;
    } else {
	FsHandleUnlock(handlePtr);
	*hdrPtrPtr = (FsHandleHeader *)handlePtr;
	status = SUCCESS;
    }
    Mem_Free((Address)data);
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsFileMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	Three things are done:  cache consistency actions are taken to
 *	reflect the movement of the client, file state is set up for use
 *	on the client in the MigEnd procedure, and cross-network stream
 *	sharing is detected.  A useful side-effect of this routine is
 *	to properly set the type in the ioFileID, either FS_LCL_FILE_STREAM
 *	or FS_RMT_FILE_STREAM.  In the latter case FsRmtFileMigrate
 *	is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up or if there
 *	is a cache consistency failure.  Otherwise SUCCESS is returned,
 *	*flagsPtr may have the FS_RMT_SHARED bit set, and *sizePtr
 *	and *dataPtr are set to reference FsFileState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return FsFileState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsFileMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - correct stream offset */
    int		*sizePtr;	/* Return - sizeof(FsFileState) */
    Address	*dataPtr;	/* Return - pointer to FsFileState */
{
    register FsLocalFileIOHandle	*handlePtr;
    register FsFileState		*fileStatePtr;
    register ReturnStatus		status;
    register Fs_Stream			*streamPtr;
    Boolean				found;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The file was local, which is why we were called, but is now remote.
	 */
	migInfoPtr->ioFileID.type = FS_RMT_FILE_STREAM;
	return(FsRmtFileMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_LCL_FILE_STREAM;
    handlePtr = FsHandleFetchType(FsLocalFileIOHandle, &migInfoPtr->ioFileID);
    if (handlePtr == (FsLocalFileIOHandle *)NIL) {
	Sys_Panic(SYS_FATAL, "FsFileMigrate, no I/O handle");
	status = FS_STALE_HANDLE;
    } else {
	/*
	 * At the stream level, add the new client to the set of clients
	 * for the stream, and check for any cross-network stream sharing.
	 * We only close the orignial client if the stream is unshared,
	 * i.e. there are no references left there.
	 */
	streamPtr = FsStreamFind(&migInfoPtr->streamID,
		    (FsHandleHeader *)handlePtr, migInfoPtr->flags,
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
	 * Adjust use counts on the I/O handle to reflect any new sharing.
	 */
	 if ((migInfoPtr->flags & FS_NEW_STREAM) &&
	     (migInfoPtr->flags & FS_RMT_SHARED)) {
	    /*
	     * The stream is becoming shared across the network so
	     * we need to increment the use counts on the I/O handle
	     * to reflect the additional client stream.
	     */
	    handlePtr->use.ref++;
	    if (migInfoPtr->flags & FS_WRITE) {
		handlePtr->use.write++;
	    }
	    if (migInfoPtr->flags & FS_EXECUTE) {
		handlePtr->use.exec++;
	    }
	} else if ((migInfoPtr->flags & (FS_NEW_STREAM|FS_RMT_SHARED)) == 0) {
	    /*
	     * The stream is no longer shared, and it is not new on the
	     * target client, so we have to decrement the use counts
	     * to reflect the fact that the original client's stream is not
	     * referencing the I/O handle.
	     */
	    handlePtr->use.ref--;
	    if (migInfoPtr->flags & FS_WRITE) {
		handlePtr->use.write--;
	    }
	    if (migInfoPtr->flags & FS_EXECUTE) {
		handlePtr->use.exec--;
	    }
	}
	/*
	 * Update the client list, and take any required cache consistency
	 * actions. The handle returns unlocked from the consistency routine.
	 */
	fileStatePtr = Mem_New(FsFileState);
	FsHandleUnlock(handlePtr);
	status = FsMigrateConsistency(handlePtr, migInfoPtr->srcClientID,
		dstClientID, migInfoPtr->flags,
		&fileStatePtr->cacheable, &fileStatePtr->openTimeStamp);
	if (status == SUCCESS) {
	    FsGetCachedAttr(&handlePtr->cacheInfo, &fileStatePtr->version,
				&fileStatePtr->attr);
	    *sizePtr = sizeof(FsFileState);
	    *dataPtr = (Address)fileStatePtr;
	    *flagsPtr = fileStatePtr->newUseFlags = streamPtr->flags;
	    *offsetPtr = streamPtr->offset;
	} else {
	    Mem_Free((Address)fileStatePtr);
	}
	/*
	 * We don't need this reference on the I/O handle, there is no change.
	 */
	FsHandleRelease(handlePtr, FALSE);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileRead --
 *
 *	Read from a file.  This is a thin layer on top of the cache
 *	read routine.
 *
 * Results:
 *	The results of FsCacheRead.
 *
 * Side effects:
 *	None, because FsCacheRead does most everything.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsFileRead(streamPtr, flags, buffer, offsetPtr,  lenPtr, remoteWaitPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the file. */
    int			flags;		/* Usage flags. */
    register Address	buffer;		/* Where to read into. */
    int 		*offsetPtr;	/* In/Out byte offset */
    int			*lenPtr;	/* In/Out bytes to read. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
{
    register FsLocalFileIOHandle *handlePtr =
	    (FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    status = FsCacheRead(&handlePtr->cacheInfo, flags, buffer, *offsetPtr,
			 lenPtr, remoteWaitPtr);
    *offsetPtr += *lenPtr;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileWrite --
 *
 *	Write to a disk file.  This is a thin layer on top of the cache
 *	write routine.  Besides doing the write, this routine synchronizes
 *	with read ahead on the file.
 *
 * Results:
 *	The results of FsCacheWrite.
 *
 * Side effects:
 *	The handle is locked during the I/O.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsFileWrite(streamPtr, flags, buffer, offsetPtr,  lenPtr, remoteWaitPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the file. */
    int			flags;		/* Usage flags. */
    register Address	buffer;		/* Where to read into. */
    int 		*offsetPtr;	/* In/Out byte offset */
    int			*lenPtr;	/* In/Out bytes to read. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
{
    register FsLocalFileIOHandle *handlePtr =
	    (FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    /*
     * Get a reference to the domain so it can't be dismounted during the I/O.
     * Then synchronize with read ahead before actually doing the write.
     */
    if (FsDomainFetch(handlePtr->hdr.fileID.major, FALSE) == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    FsWaitForReadAhead(&handlePtr->readAhead);
    status = FsCacheWrite(&handlePtr->cacheInfo, flags, buffer, *offsetPtr,
			  lenPtr, remoteWaitPtr);
    if (status == SUCCESS && (fsWriteThrough || fsWriteBackASAP)) {
	/*
	 * When in write-through or asap mode we have to force the descriptor
	 * to disk on every write.
	 */
	FsWriteBackDesc(handlePtr, FALSE);
    }

    *offsetPtr += *lenPtr;
    FsAllowReadAhead(&handlePtr->readAhead);
    FsDomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileBlockRead --
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
FsFileBlockRead(hdrPtr, flags, buffer, offsetPtr,  lenPtr, remoteWaitPtr)
    FsHandleHeader	*hdrPtr;	/* Handle on a local file. */
    int			flags;		/* IGNORED */
    register Address	buffer;		/* Where to read into. */
    int 		*offsetPtr;	/* Byte offset at which to read.  Return
					 * value isn't used, but by-reference
					 * passing is done to be compatible
					 * with FsSpriteRead */
    int			*lenPtr;	/* Return,  bytes read. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* IGNORED */
{
    register FsLocalFileIOHandle *handlePtr =
	    (FsLocalFileIOHandle *)hdrPtr;
    register	FsDomain	 *domainPtr;
    register	FsFileDescriptor *descPtr;
    register			 offset = *offsetPtr;
    register int		 numBytes;
    ReturnStatus		 status;
    FsBlockIndexInfo		 indexInfo;

    numBytes = *lenPtr;
    if ((offset & FS_BLOCK_OFFSET_MASK) != 0) {
	Sys_Panic(SYS_FATAL, "FsFileBlockRead: Non-block aligned offset\n");
    }
    if (numBytes > FS_BLOCK_SIZE) {
	Sys_Panic(SYS_FATAL, "FsFileBlockRead: Reading more than block\n");
    }

    domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (FsDomain *) NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }

    if (handlePtr->hdr.fileID.minor == 0) {
	/*
	 * If is a physical block address then read it in directly.
	 */
	status = FsDeviceBlockIO(FS_READ, &domainPtr->headerPtr->device,
			   offset / FS_FRAGMENT_SIZE, FS_FRAGMENTS_PER_BLOCK, 
			   buffer);
	fsStats.gen.physBytesRead += FS_BLOCK_SIZE;
    } else {
	/*
	 * Is a logical file read. Round the size down to the actual
	 * last byte in the file.
	 */

	descPtr = handlePtr->descPtr;
	if (offset > descPtr->lastByte) {
	    *lenPtr = 0;
	    FsDomainRelease(handlePtr->hdr.fileID.major);
	    return(SUCCESS);
	} else if (offset + numBytes - 1 > descPtr->lastByte) {
	    numBytes = descPtr->lastByte - offset + 1;
	}

	status = FsGetFirstIndex(handlePtr, offset / FS_BLOCK_SIZE, 
				 &indexInfo, 0);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "FsFileRead: Could not setup indexing\n");
	    FsDomainRelease(handlePtr->hdr.fileID.major);
	    return(status);
	}

	if (indexInfo.blockAddrPtr != (int *) NIL &&
	    *indexInfo.blockAddrPtr != FS_NIL_INDEX) {
	    /*
	     * Read in the block.
	     */
	    status = FsDeviceBlockIO(FS_READ, &domainPtr->headerPtr->device,
		      *indexInfo.blockAddrPtr +
		      domainPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
		      (numBytes - 1) / FS_FRAGMENT_SIZE + 1, buffer);
	} else {
	    /*
	     * Zero fill the block.  We're in a 'hole' in the file.
	     */
	    Byte_Zero(numBytes, buffer);
	}
	FsEndIndex(handlePtr, &indexInfo, FALSE);
	FsStat_Add(numBytes, fsStats.gen.fileBytesRead,
		   fsStats.gen.fileReadOverflow);
#ifndef CLEAN
	if (fsKeepTypeInfo) {
	    int fileType;
	
	    fileType = FsFindFileType(&handlePtr->cacheInfo);
	    fsStats.type.diskBytes[FS_STAT_READ][fileType] += numBytes;
	}
#endif CLEAN
    }

    if (status == SUCCESS) {
	*lenPtr = numBytes;
    }
    FsDomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileBlockWrite --
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
FsFileBlockWrite(hdrPtr, blockAddr, numBytes, buffer, flags)
    FsHandleHeader *hdrPtr;	/* Pointer to handle for file to write to. */
    int 	blockAddr;	/* Disk address. For regular files this
				 * counts from the beginning of the data blocks.
				 * For raw I/O they count from the start
				 * of the disk partition. */
    int		numBytes;	/* Number of bytes to write. */
    register Address buffer;	/* Where to read bytes from. */
    int		flags;		/* IGNORED */
{
    register FsLocalFileIOHandle *handlePtr =
	    (FsLocalFileIOHandle *)hdrPtr;
    register	FsDomain	 *domainPtr;
    ReturnStatus		status;

    domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, TRUE);
    if (domainPtr == (FsDomain *)NIL) {
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
	fsStats.gen.physBytesWritten += numBytes;
	status = FsDeviceBlockIO(FS_WRITE, &domainPtr->headerPtr->device,
			 blockAddr, FS_FRAGMENTS_PER_BLOCK, buffer);
    } else {
	/*
	 * The block number is relative to the start of the data blocks.
	 */
	FsStat_Add(numBytes, fsStats.gen.fileBytesWritten,
		   fsStats.gen.fileWriteOverflow);
#ifndef CLEAN
	if (fsKeepTypeInfo) {
	    int fileType;
	
	    fileType = FsFindFileType(&handlePtr->cacheInfo);
	    fsStats.type.diskBytes[FS_STAT_WRITE][fileType] += numBytes;
	}
#endif CLEAN
	status = FsDeviceBlockIO(FS_WRITE, &domainPtr->headerPtr->device,
	       blockAddr + 
	       domainPtr->headerPtr->dataOffset * FS_FRAGMENTS_PER_BLOCK,
	       (numBytes - 1) / FS_FRAGMENT_SIZE + 1, buffer);
    }
    FsDomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileBlockCopy --
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
FsFileBlockCopy(srcHdrPtr, dstHdrPtr, blockNum)
    FsHandleHeader	*srcHdrPtr;	/* File to copy block from.  */
    FsHandleHeader	*dstHdrPtr;	/* File to copy block to.  */
    int			blockNum;	/* Block to copy. */
{
    int			offset;
    ReturnStatus	status;
    FsCacheBlock	*cacheBlockPtr;
    int			numBytes;
    register FsLocalFileIOHandle *srcHandlePtr =
	    (FsLocalFileIOHandle *)srcHdrPtr;
    register FsLocalFileIOHandle *dstHandlePtr =
	    (FsLocalFileIOHandle *)dstHdrPtr;

    /*
     * Look in the cache for the source block.
     */
    status = FsCacheBlockRead(&srcHandlePtr->cacheInfo, blockNum,
		    &cacheBlockPtr, &numBytes, FS_DATA_CACHE_BLOCK, FALSE);
    if (status != SUCCESS) {
	return(status);
    }
    if (numBytes != FS_BLOCK_SIZE) {
	if (numBytes != 0) {
	    FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);
	}
	return(VM_SHORT_READ);
    }
    /*
     * Write to the destination block.
     */
    numBytes = FS_BLOCK_SIZE;
    offset = blockNum * FS_BLOCK_SIZE;
    status = FsCacheWrite(&dstHandlePtr->cacheInfo, FALSE,
		    cacheBlockPtr->blockAddr, offset, &numBytes,
		    (Sync_RemoteWaiter *) NIL);
    if (status == SUCCESS && numBytes != FS_BLOCK_SIZE) {

	status = VM_SHORT_WRITE;
    }

    FsCacheUnlockBlock(cacheBlockPtr, 0, -1, 0, FS_CLEAR_READ_AHEAD);

    srcHandlePtr->cacheInfo.attr.accessTime = fsTimeInSeconds;
    dstHandlePtr->cacheInfo.attr.modifyTime = fsTimeInSeconds;

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileIOControl --
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
FsFileIOControl(streamPtr, command, byteOrder, inBufSize, inBuffer, outBufSize, outBuffer)
    Fs_Stream *streamPtr;		/* Stream to local file */
    int command;			/* File specific I/O control */
    int byteOrder;			/* Client's byte order */
    int inBufSize;			/* Size of inBuffer */
    Address inBuffer;			/* Buffer of input arguments */
    int outBufSize;			/* Size of outBuffer */
    Address outBuffer;			/* Buffer for return parameters */

{
    register FsLocalFileIOHandle *handlePtr =
	    (FsLocalFileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status = SUCCESS;

    FsHandleLock(handlePtr);
    switch(command) {
	case IOC_REPOSITION:
	    break;
	case IOC_GET_FLAGS:
	    if ((outBufSize >= sizeof(int)) && (outBuffer != (Address)NIL)) {
		*(int *)outBuffer = 0;
	    }
	    break;
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	    break;
	case IOC_TRUNCATE: {
	    int length;

	    if (byteOrder != mach_ByteOrder) {
		int size = sizeof(int);
		Swap_Buffer(inBuffer, sizeof(int), byteOrder, mach_ByteOrder,
			    "w", (Address)&length, &size);
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else if (inBufSize < sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		length = *(int *)inBuffer;
	    }
	    if (status == SUCCESS) {
		status = FsFileTrunc(handlePtr, length, 0);
	    }
	    break;
	}
	case IOC_LOCK:
	case IOC_UNLOCK:
	    status = FsIocLock(&handlePtr->lock, command, byteOrder, inBuffer,
				inBufSize, &streamPtr->hdr.fileID);
	    break;
	case IOC_NUM_READABLE: {
	    /*
	     * Return the number of bytes available to read.  The top-level
	     * IOControl routine has put the current stream offset in inBuffer.
	     */
	    int bytesAvailable;
	    int streamOffset;
	    int size;

	    if (byteOrder != mach_ByteOrder) {
		size = sizeof(int);
		Swap_Buffer(inBuffer, inBufSize, byteOrder, mach_ByteOrder, "w",
			    (Address)&streamOffset, &size);
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else if (inBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		streamOffset = *(int *)inBuffer;
	    }
	    if (status == SUCCESS) {
		bytesAvailable = handlePtr->cacheInfo.attr.lastByte + 1 -
				streamOffset;
		if (byteOrder != mach_ByteOrder) {
		    Swap_Buffer((Address)&bytesAvailable, sizeof(int),
			mach_ByteOrder, byteOrder, "w", outBuffer, &size);
		    if (size != sizeof(int)) {
			status = GEN_INVALID_ARG;
		    }
		} else if (outBufSize != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		} else {
		    *(int *)outBuffer = bytesAvailable;
		}
	    }
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
    FsHandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileTrunc --
 *
 *	Shorten a file to length bytes.  This calls routines to update
 *	the cacheInfo and the fileDescriptor.
 *
 * Results:
 *	Error status from FsDescTrunc.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsFileTrunc(handlePtr, size, flags)
    FsLocalFileIOHandle	*handlePtr;	/* File to truncate. */
    int			size;		/* Size to truncate the file to. */
    int			flags;		/* FS_TRUNC_DELETE */
{
    ReturnStatus status;
    FsCacheTrunc(&handlePtr->cacheInfo, size, flags);
    status = FsDescTrunc(handlePtr, size);
    if ((flags & FS_TRUNC_DELETE) && handlePtr->cacheInfo.blocksInCache > 0) {
	Sys_Panic(SYS_FATAL, "FsFileTrunc (delete) %d blocks left over\n",
		    handlePtr->cacheInfo.blocksInCache);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileSelect --
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
FsFileSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader *hdrPtr;	/* The handle of the file */
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
    FsLocalFileIOHandle	*handlePtr;
{
    FsFileDescriptor	*descPtr;
    FsDomain		*domainPtr;

    descPtr = handlePtr->descPtr;
    descPtr->version++;
    handlePtr->cacheInfo.version = descPtr->version;
    domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	Sys_Panic(SYS_WARNING, "FsIncVersionNumber: Domain gone.\n");
    } else {
	(void)FsStoreFileDesc(domainPtr, handlePtr->hdr.fileID.minor,
			descPtr);
	FsDomainRelease(handlePtr->hdr.fileID.major);
    }
    Vm_FileChanged(&handlePtr->segPtr);
}
