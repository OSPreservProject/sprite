/* 
 * fsRmtFile.c --
 *
 *	Routines for operations on remote files.  The I/O operations
 *	check in the cache, and then use the raw remote I/O operations
 *	to transfer data to and from the remote file server.
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


#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsconsist.h>
#include <fsioFile.h>
#include <fslcl.h>
#include <fscache.h>
#include <fsdm.h>
#include <fsStat.h>
#include <fsprefix.h>
#include <fsNameOps.h>
#include <fsrmt.h>

#include <rpc.h>
#include <vm.h>

#include <stdio.h>

static ReturnStatus FsrmtFileBlockRead _ARGS_((Fs_HandleHeader *hdrPtr, 
		Fscache_Block *blockPtr, Sync_RemoteWaiter *waitPtr));
static ReturnStatus FsrmtFileBlockWrite _ARGS_((Fs_HandleHeader *hdrPtr, 
		Fscache_Block *blockPtr, int flags));
static ReturnStatus FsrmtFileBlockAllocate _ARGS_((Fs_HandleHeader *hdrPtr, 
		int offset, int numBytes, int flags, int *blockAddrPtr, 
		Boolean *newBlockPtr));
static ReturnStatus FsrmtFileTrunc _ARGS_((Fs_HandleHeader *hdrPtr, int size,
					  Boolean delete));
static Boolean FsrmtStartWriteBack _ARGS_((Fscache_Backend *backendPtr));

static void FsrmtReallocBlock _ARGS_((ClientData data, 
		Proc_CallInfo *callInfoPtr));

static Fscache_BackendRoutines  fsrmtBackendRoutines = {
	    FsrmtFileBlockAllocate,
	    FsrmtFileTrunc,
	    FsrmtFileBlockRead,
	    FsrmtFileBlockWrite,
	    FsrmtReallocBlock,
	    FsrmtStartWriteBack,

};
static 	Fscache_Backend *cacheBackendPtr = (Fscache_Backend *) NIL;

static Boolean FileMatch _ARGS_((Fscache_FileInfo *cacheInfoPtr, 
		ClientData cleintData));
static Boolean BlockMatch _ARGS_((Fscache_Block *blockPtr,
		ClientData cleintData));

static Sync_Lock rmtCleanerLock = Sync_LockInitStatic("Fs:rmtCleanerLock");
#define	LOCKPTR	&rmtCleanerLock

int	fsrmtBlockCleaners = 0;


/*
 *----------------------------------------------------------------------
 *
 * FsRmtFileHandleInit --
 *
 *	Initialize a handle for a remote file from the file state
 *	returned by the server.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Create and install a handle for the file.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsRmtFileHandleInit(fileIDPtr, fileStatePtr, openForWriting, name,
	newHandlePtrPtr)
    Fs_FileID		*fileIDPtr;
    Fsio_FileState		*fileStatePtr;
    Boolean		openForWriting;
    char		*name;
    Fsrmt_FileIOHandle	**newHandlePtrPtr;
{
    register Fsrmt_FileIOHandle *handlePtr;
    Boolean found;
    int	    size;

    /*
     * Since both Fsrmt_FileIOHandle and Fsio_FileIOHandle are so 
     * popular on the root file server we try to use the same memory
     * size for both.  We choose to allocate the large of the two.
     */
    size = sizeof(Fsrmt_FileIOHandle);
    if (size < sizeof(Fsio_FileIOHandle)) {
	size = sizeof(Fsio_FileIOHandle);
    }

    found = Fsutil_HandleInstall(fileIDPtr, size, name,
		    FALSE, (Fs_HandleHeader **)newHandlePtrPtr);
    handlePtr = *newHandlePtrPtr;
    if (found) {
	/*
	 * Update attributes cached in the handle, and verify the
	 * validity of any cached data blocks.
	 */
	if (Fscache_UpdateFile(&handlePtr->cacheInfo, openForWriting,
		    fileStatePtr->version, fileStatePtr->cacheable,
		    &fileStatePtr->attr)) {
	    Vm_FileChanged(&handlePtr->segPtr);
	}
	/* 
	 * Update the handle's open time stamp.  This is used to catch races
	 * between near-simultanous opens on a client and related cache
	 * consistency messasge.
	 */
	if (fileStatePtr->openTimeStamp > handlePtr->openTimeStamp) {
	    handlePtr->openTimeStamp = fileStatePtr->openTimeStamp;
	}
    } else {
	if (cacheBackendPtr == (Fscache_Backend *) NIL) {
	    cacheBackendPtr = 
		Fscache_RegisterBackend(&fsrmtBackendRoutines,(ClientData)0,0);
	}
	/*
	 * Initialize the new handle.  There is no cache validation to
	 * be done because blocks are only cached when a handle is present.
	 */
	Fscache_FileInfoInit(&handlePtr->cacheInfo, (Fs_HandleHeader *)handlePtr,
		fileStatePtr->version, fileStatePtr->cacheable,
		&fileStatePtr->attr, cacheBackendPtr);

	Fsutil_RecoveryInit(&handlePtr->rmt.recovery);
	Fscache_ReadAheadInit(&handlePtr->readAhead);

	handlePtr->flags = 0;
	handlePtr->openTimeStamp = fileStatePtr->openTimeStamp;
	handlePtr->segPtr = (Vm_Segment *)NIL;
	fs_Stats.object.rmtFiles++;
    }
    if (fileStatePtr->newUseFlags & FS_DIR) {
	handlePtr->cacheInfo.flags |= FSCACHE_IS_DIR;
    }
    free((Address)fileStatePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileIoOpen --
 *
 *      Set up a stream for a remote disk file.  This is called from Fs_Open
 *	to complete the opening of a stream. By this time any cache consistency
 *	actions have already been taken.
 *
 * Results:
 *	SUCCESS, until FsRmtFileHandleInit returns differently.
 *
 * Side effects:
 *	Installs the handle for the file and updates the use counts
 *	(ref, write, exec) due to this open.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    Fs_FileID		*ioFileIDPtr;	/* I/O fileID from the name server */
    int			*flagsPtr;	/* New ones from the server returned */
    int			clientID;	/* IGNORED */
    ClientData		streamData;	/* Reference to Fsio_FileState struct */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a handle set up for
					 * I/O to a file, NIL if failure. */
{
    ReturnStatus		status;
    Fsio_FileState			*fileStatePtr;
    register int 		flags;
    register Fsrmt_FileIOHandle *rmtHandlePtr;

    fileStatePtr = (Fsio_FileState *)streamData;
    flags = fileStatePtr->newUseFlags;

    status = FsRmtFileHandleInit(ioFileIDPtr, fileStatePtr,
		(flags & FS_WRITE), name, (Fsrmt_FileIOHandle **)ioHandlePtrPtr);
    if (status == SUCCESS) {
	rmtHandlePtr = (Fsrmt_FileIOHandle *)*ioHandlePtrPtr;
	/*
	 * Update our use information to reflect the open.
	 */
	rmtHandlePtr->rmt.recovery.use.ref++;
	if (flags & FS_WRITE) {
	    rmtHandlePtr->rmt.recovery.use.write++;
	}
	if (flags & FS_EXECUTE) {
	    rmtHandlePtr->rmt.recovery.use.exec++;
	}
	/*
	 * Note if we are a swap file in case of recovery later.
	 */
	rmtHandlePtr->flags |= (flags & FS_SWAP);
	*flagsPtr = flags;
	Fsutil_HandleUnlock(rmtHandlePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileReopen --
 *
 *	Reopen a remote file.  This sets up and conducts an 
 *	RPC_FS_REOPEN remote procedure call to re-open the remote file.
 *
 * Results:
 *	A  non-SUCCESS return code if the re-open was attempted and failed.
 *	FS_NO_HANDLE if we delete the handle.
 *
 * Side effects:
 *	If the reopen works we'll have updated cached attributes, version
 *	number, etc.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;
    int			clientID;		/* Should be rpc_SpriteID */
    ClientData		inData;			/* IGNORED */
    int			*outSizePtr;		/* IGNORED */
    ClientData		*outDataPtr;		/* IGNORED */
{
    register Fsrmt_FileIOHandle	*rmtHandlePtr;
    ReturnStatus		status;
    Fsio_FileReopenParams		reopenParams;
    Fsio_FileState			fileState;
    register int		numDirtyBlocks;
    int				outSize;

    rmtHandlePtr = (Fsrmt_FileIOHandle *)hdrPtr;
    numDirtyBlocks = Fscache_PreventWriteBacks(&rmtHandlePtr->cacheInfo);
    Fscache_AllowWriteBacks(&rmtHandlePtr->cacheInfo);
    /*
     * Optimize out re-opens for files that aren't being waited
     * on, that have no users, and that have no blocks in the cache.
     * I don't call OkToScavenge from fsutilHandleScavenge.c, since I think
     * it's okay to do this even if there's already an active scavenger,
     * because it will have locked the handle if it's working on it.
     */
    if (rmtHandlePtr->rmt.recovery.use.ref == 0 &&
	    Fscache_OkToScavenge(&rmtHandlePtr->cacheInfo) &&
	    !Fsutil_RecoveryNeeded(&rmtHandlePtr->rmt.recovery)) {
        Vm_FileChanged(&rmtHandlePtr->segPtr);
        Fsutil_RecoverySyncLockCleanup(&rmtHandlePtr->rmt.recovery);
        Fscache_InfoSyncLockCleanup(&rmtHandlePtr->cacheInfo);
        Fscache_ReadAheadSyncLockCleanup(&rmtHandlePtr->readAhead);
        Fsutil_HandleRemove(rmtHandlePtr);
        fs_Stats.object.rmtFiles--;
	fs_Stats.recovery.reopensAvoided++;
	return FS_NO_HANDLE;
    }
/* for debugging */
#define	FS_HANDLE_INVALID 0x20
    if (hdrPtr->flags & FS_HANDLE_INVALID) {
	printf("Attempting to recover invalid file handle <%d,0x%x,0x%x>\n",
		hdrPtr->fileID.serverID, hdrPtr->fileID.major,
		hdrPtr->fileID.minor);
	return SUCCESS;
    }
/* end for debugging */
    reopenParams.flags = rmtHandlePtr->flags;
    if (numDirtyBlocks > 0) {
	reopenParams.flags |= FSIO_HAVE_BLOCKS;
    }
    reopenParams.fileID = hdrPtr->fileID;
    reopenParams.fileID.type = FSIO_LCL_FILE_STREAM;
    reopenParams.prefixFileID.type = NIL;	/* not used */
    reopenParams.use = rmtHandlePtr->rmt.recovery.use;
    reopenParams.version = rmtHandlePtr->cacheInfo.version;

    /*
     * Contact the server to do the reopen.  We have to unlock the handle
     * here in case the server asks us to write-back or invalidate.
     */
    outSize = sizeof(Fsio_FileState);
    Fsutil_HandleUnlock(hdrPtr);
    status = FsrmtReopen(hdrPtr, sizeof(Fsio_FileReopenParams),
		(Address)&reopenParams, &outSize, (Address)&fileState);
    Fsutil_HandleLock(hdrPtr);
    if (status != SUCCESS) {
	if (numDirtyBlocks > 0) {
	    printf(
"Re-open failed <%x> with dirty blocks in cache, \"%s\" <%d,%d> server %d\n",
		status, Fsutil_HandleName(hdrPtr), hdrPtr->fileID.major,
		hdrPtr->fileID.minor, hdrPtr->fileID.serverID);
	}
	if (status != RPC_TIMEOUT &&
	    status != RPC_SERVICE_DISABLED) {
	    /*
	     * Nuke the cache because our caller will nuke the handle.
	     */
	    Fscache_FileInvalidate(&rmtHandlePtr->cacheInfo, 0, FSCACHE_LAST_BLOCK);
	}
    } else {
	/*
	 * Flush any lanquishing dirty blocks back to the server.
	 */
	if (numDirtyBlocks > 0) {
	    int skipped;
	    (void) Fscache_FileWriteBack(&rmtHandlePtr->cacheInfo, 0,
				FSCACHE_LAST_BLOCK, 0, &skipped);
	}
	/*
	 * Update the handle - take care of cache flushes, updating cached
	 * attributes, etc.
	 */
	if (Fscache_UpdateFile(&rmtHandlePtr->cacheInfo,
		    rmtHandlePtr->rmt.recovery.use.write,
		    fileState.version, fileState.cacheable,
		    &fileState.attr)) {
	    Vm_FileChanged(&rmtHandlePtr->segPtr);
	}

	rmtHandlePtr->openTimeStamp = fileState.openTimeStamp;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileClose -- 
 *
 *	Close time processing for remote files.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	This decrements the local use counts, then synchronizes
 *	with write-backs before telling the server about the close.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
FsrmtFileClose(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to remote file */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* Process ID of closer */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of closeData */
    ClientData		closeData;	/* NIL on entry. */
{
    register ReturnStatus status;
    register Fsrmt_FileIOHandle *handlePtr =
	    (Fsrmt_FileIOHandle *)streamPtr->ioHandlePtr;
    /*
     * Decrement local references.
     */

    handlePtr->rmt.recovery.use.ref--;
    if (flags & FS_WRITE) {
	handlePtr->rmt.recovery.use.write--;
    }
    if (flags & FS_EXECUTE) {
	handlePtr->rmt.recovery.use.exec--;
    }
    if (handlePtr->rmt.recovery.use.ref < 0 ||
	handlePtr->rmt.recovery.use.write < 0 ||
	handlePtr->rmt.recovery.use.exec < 0) {
	panic("FsrmtFileClose: <%d,%d> ref %d write %d exec %d\n",
	    handlePtr->rmt.hdr.fileID.major, handlePtr->rmt.hdr.fileID.minor,
	    handlePtr->rmt.recovery.use.ref,
	    handlePtr->rmt.recovery.use.write,
	    handlePtr->rmt.recovery.use.exec);
    }
    if (!Fsutil_HandleValid((Fs_HandleHeader *)handlePtr)) {
	status = FS_FILE_REMOVED;
    } else {
	if (handlePtr->rmt.recovery.use.ref == 0) {
	    int	numDirtyBlocks;
	    /*
	     * Synchronize closes and delayed writes.  We wait for writes
	     * in progress to complete. 
	     */
	    numDirtyBlocks =  Fscache_PreventWriteBacks(&handlePtr->cacheInfo);
	    if (numDirtyBlocks == 0) {
		/*
		 * Inform the server on the close requests the we don't
		 * have any dirty blocks for the file.
		 */
		flags |= FS_LAST_DIRTY_BLOCK;
	    }
	}
	status = Fsrmt_Close(streamPtr, clientID, procID, flags,
	    sizeof(Fscache_Attributes), (ClientData)&handlePtr->cacheInfo.attr);
    }

    if (status == FS_FILE_REMOVED && handlePtr->rmt.recovery.use.ref == 0) {
	/*
	 * Nuke our cache after we've finished with this (now bogus) handle.
	 * (Apparently we let the handle linger and get scavenged.  Could
	 * change things to remove the handle here.)
	 */
	Fscache_Trunc(&handlePtr->cacheInfo, 0, FSCACHE_TRUNC_DELETE);
    }
    Fscache_AllowWriteBacks(&handlePtr->cacheInfo);
    Fsutil_HandleRelease(handlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileScavenge --
 *
 *	Called peridocally to see if we still need the handle for
 *	the remote file.
 *
 * Results:
 *	TRUE if it removed the handle.
 *
 * Side effects:
 *	Either removes or unlocks the handle.
 *
 *----------------------------------------------------------------------
 */
Boolean
FsrmtFileScavenge(hdrPtr)
    Fs_HandleHeader *hdrPtr;
{
    register Fsrmt_FileIOHandle *handlePtr = (Fsrmt_FileIOHandle *)hdrPtr;
    if (handlePtr->rmt.recovery.use.ref == 0 &&
	Fscache_OkToScavenge(&handlePtr->cacheInfo) &&
	!Fsutil_RecoveryNeeded(&handlePtr->rmt.recovery)) {
	/*
	 * Remove handles for files with no users and no blocks in cache.
	 */
	Vm_FileChanged(&handlePtr->segPtr);
	Fsutil_RecoverySyncLockCleanup(&handlePtr->rmt.recovery);
	Fscache_InfoSyncLockCleanup(&handlePtr->cacheInfo);
	Fscache_ReadAheadSyncLockCleanup(&handlePtr->readAhead);
	Fsutil_HandleRemove(handlePtr);
	fs_Stats.object.rmtFiles--;
	return(TRUE);
    } else {
	Fsutil_HandleUnlock(handlePtr);
	return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileVerify --
 *
 *	Map from a remote client's fileID to a local fileID and
 *	verify that the remote client is known.
 *
 * Results:
 *	A pointer to the local handle for the file, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	Changes the client's fileID type to FSIO_LCL_FILE_STREAM and
 *	fetches the local handle.  The handle is returned locked.
 *
 *----------------------------------------------------------------------
 */

Fs_HandleHeader *
FsrmtFileVerify(fileIDPtr, clientID, domainTypePtr)
    Fs_FileID	*fileIDPtr;	/* Client's fileID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_LOCAL_DOMAIN */
{
    register Fsio_FileIOHandle *handlePtr;
    register Fsconsist_ClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = FSIO_LCL_FILE_STREAM;
    handlePtr = Fsutil_HandleFetchType(Fsio_FileIOHandle, fileIDPtr);
    if (handlePtr != (Fsio_FileIOHandle *)NIL) {
	LIST_FORALL(&handlePtr->consist.clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    printf(
		"FsrmtFileVerify: \"%s\" <%d,%d> client %d not found\n",
		Fsutil_HandleName(handlePtr), fileIDPtr->major, fileIDPtr->minor,
		clientID);
	    Fsutil_HandleRelease(handlePtr, TRUE);
	    handlePtr = (Fsio_FileIOHandle *)NIL;
	}
    } else {
	printf( "FsrmtFileVerify, no handle <%d,%d> client %d\n",
	    fileIDPtr->major, fileIDPtr->minor, clientID);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_LOCAL_DOMAIN;
    }
    return((Fs_HandleHeader *)handlePtr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsrmtFileMigClose --
 *
 *	Release recovery use counts on a remote file.  This is called when
 *	a stream to the file has migrated away.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Decrement use counts and release the handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileMigClose(hdrPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
{
    register Fsrmt_FileIOHandle *handlePtr = (Fsrmt_FileIOHandle *)hdrPtr;

    Fsutil_HandleLock(handlePtr);
    handlePtr->rmt.recovery.use.ref--;
    if (flags & FS_WRITE) {
	handlePtr->rmt.recovery.use.write--;
    }
    if (flags & FS_EXECUTE) {
	handlePtr->rmt.recovery.use.exec--;
    }
    Fsutil_HandleRelease(handlePtr, TRUE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsrmtFileMigOpen --
 *
 *	Complete the creation of a FSIO_RMT_FILE_STREAM after migration.
 *	
 * Results:
 *	SUCCESS (until FsRmtFileHandleInit returns differently)
 *
 * Side effects:
 *	If SUCCESS, adds a reference and useCounts to the I/O handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileMigOpen(migInfoPtr, size, data, hdrPtrPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* sizeof(Fsio_FileState), IGNORED */
    ClientData	data;		/* Ref. to Fsio_FileState. */
    Fs_HandleHeader **hdrPtrPtr;	/* Return - I/O handle for the file */
{
    ReturnStatus status;
    register Fs_FileID *fileIDPtr = &migInfoPtr->ioFileID;
    Fsrmt_FileIOHandle *handlePtr;

    status = FsrmtFileIoOpen(fileIDPtr, &migInfoPtr->flags, rpc_SpriteID,
		data, (char *)NIL, (Fs_HandleHeader **)&handlePtr);
    *hdrPtrPtr = (Fs_HandleHeader *)handlePtr;
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsrmtFileMigrate --
 *
 *	This causes a call to Fsio_FileMigrate on the server to shift
 *	client references.  We may be the server after migration, in which
 *	case the routine is called directly.  Otherwise an RPC is done
 *	to the server. A useful side-effect of this routine is
 *	to properly set the type in the ioFileID, either FSIO_LCL_FILE_STREAM
 *	or FSIO_RMT_FILE_STREAM.
 *
 * Results:
 *	The return code from Fsio_FileMigrate, or the RPC.
 *	Returns Fsio_FileState for use by the MigEnd routine.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - the correct stream offset */
    int		*sizePtr;	/* Return - sizeof(Fsio_FileState) */
    Address	*dataPtr;	/* Return - pointer to Fsio_FileState */
{
    register ReturnStatus		status;
    register Fsio_FileState		*fileStatePtr;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The file was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FSIO_LCL_FILE_STREAM;
	return(Fsio_FileMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
			    sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_RMT_FILE_STREAM;
    fileStatePtr = mnew(Fsio_FileState);
    status = Fsrmt_NotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
		sizeof(Fsio_FileState), (Address)fileStatePtr);
    if (status != SUCCESS) {
	printf( "FsrmtFileMigrate, server error <%x>\n",
	    status);
    } else {
	*dataPtr = (Address)fileStatePtr;
	*sizePtr = sizeof(Fsio_FileState);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileRead --
 *
 *	Read from a remote file.  This tries the cache first, and then
 *	goes remote if the file is not cacheable.  This also checks against
 *	remotely shared streams (due to migration), and bypasses the cache
 *	in that case.
 *
 * Results:
 *	The results of Fscache_Read, or of the RPC.
 *
 * Side effects:
 *	The *offsetPtr is updated to reflect the read.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsrmtFileRead(streamPtr, readPtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Stream to a remote file. */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    register Fsrmt_FileIOHandle *handlePtr =
	    (Fsrmt_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    if (readPtr->flags & FS_RMT_SHARED) {
	/*
	 * The stream is shared accross the network.  We have to go through
	 * to the file server to get the correct stream offset.
	 */
	status = FS_NOT_CACHEABLE;
    } else {
	status = Fscache_Read(&handlePtr->cacheInfo, readPtr->flags,
		    readPtr->buffer, readPtr->offset, &readPtr->length,
		    remoteWaitPtr);
	replyPtr->length = readPtr->length;
    }
    /*
     * If not-cacheable then go to the remote file server.  Otherwise
     * update the amount read in the reply because Fscache_Read uses
     * the old-style in/out length variable.
     */
    if (status != FS_NOT_CACHEABLE) {
	replyPtr->length = readPtr->length;
    } else {
	status = Fsrmt_Read(streamPtr, readPtr, remoteWaitPtr, replyPtr);
	if (status == SUCCESS) {
	    if (readPtr->flags & FS_RMT_SHARED) {
		fs_Stats.rmtIO.sharedStreamBytesRead += replyPtr->length;
	    } else {
		fs_Stats.rmtIO.uncacheableBytesRead += replyPtr->length;
		if (handlePtr->cacheInfo.flags & FSCACHE_IS_DIR) {
		    fs_Stats.rmtIO.uncacheableDirBytesRead += replyPtr->length;
		}
	    }
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileWrite --
 *
 *	Write to a remote file.  This checks for cachability and uses
 *	either the cache write routine or an RPC stub.
 *
 * Results:
 *	The results of Fscache_Write or the RPC.
 *
 * Side effects:
 *	A buffer may have to allocated when doing an uncached write.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsrmtFileWrite(streamPtr, writePtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to remote file. */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    register Fsrmt_FileIOHandle *handlePtr =
	    (Fsrmt_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    if (writePtr->flags & FS_RMT_SHARED) {
	status = FS_NOT_CACHEABLE;
    } else {
	Fscache_WaitForReadAhead(&handlePtr->readAhead);
	status = Fscache_Write(&handlePtr->cacheInfo, writePtr->flags,
			      writePtr->buffer, writePtr->offset,
		              &writePtr->length, remoteWaitPtr);
	writePtr->flags &= ~FS_SERVER_WRITE_THRU;
	Fscache_AllowReadAhead(&handlePtr->readAhead);
    }
    if (status != FS_NOT_CACHEABLE) {
	replyPtr->length = writePtr->length;
    } else {
	status = Fsrmt_Write(streamPtr, writePtr, remoteWaitPtr, replyPtr);
	if (status == SUCCESS) {
	    if (writePtr->flags & FS_RMT_SHARED) {
		fs_Stats.rmtIO.sharedStreamBytesWritten += replyPtr->length;
	    } else {
		fs_Stats.rmtIO.uncacheableBytesWritten += replyPtr->length;
	    }
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFilePageRead --
 *
 *	Do a page-in for a remote file.
 *
 * Results:
 *	The results of Fscache_Read, or of Fsrmt_Read.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsrmtFilePageRead(streamPtr, readPtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Stream to a remote file. */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    register Fsrmt_FileIOHandle *handlePtr =
	    (Fsrmt_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status = SUCCESS;
    int savedOffset = readPtr->offset;
    int savedLength = readPtr->length;
    Address savedBuffer = readPtr->buffer;
    int blockNum, firstBlock, lastBlock, size, toRead;
    Fscache_Block *blockPtr;
    Boolean found;

    /*
     * This should check if the file is cached if:
     * a) it is not a FS_SWAP file or
     * b) it is a shared file (which will be FS_SWAP)
     * but for now we'll just always check.  - Ken
     *
     * This used to be
     *		if ((readPtr->flags & FS_SWAP) == 0) {
     * The problem is that shared files are marked swap, and we don't want
     * to look in the cache for regular swap files, but we must for shared
     * files.  We need another flag and there isn't one.  If we know it's
     * a swap page, we should just do an Fsrmt_Read.
     */

    /*
     * This may be a CODE or HEAP page, so we check in our cache
     * to see if we have it there;  we may have just written it.
     * We are now, once again, attempting to special-case HEAP pages.
     * We want to keep the clean copies in the cache so they can
     * be re-read.
     */
    size = readPtr->length;
    firstBlock = (unsigned int) readPtr->offset / FS_BLOCK_SIZE;
    lastBlock = (unsigned int) (readPtr->offset + size - 1) /
		FS_BLOCK_SIZE;
    for (blockNum = firstBlock;
	 blockNum <= lastBlock; blockNum++) {
	toRead = size;
	if ((unsigned int) (readPtr->offset + size - 1)
		/ FS_BLOCK_SIZE > blockNum) {
	    toRead = (blockNum + 1) * FS_BLOCK_SIZE - readPtr->offset;
	}
	Fscache_FetchBlock(&handlePtr->cacheInfo, blockNum,
		FSCACHE_DATA_BLOCK, &blockPtr, &found);
	fs_Stats.blockCache.readAccesses++;
	if (found) {
            if (blockPtr->timeDirtied != 0) {
                fs_Stats.blockCache.readHitsOnDirtyBlock++;
            } else {
                fs_Stats.blockCache.readHitsOnCleanBlock++;
            }
            if (blockPtr->flags & FSCACHE_READ_AHEAD_BLOCK) {
                fs_Stats.blockCache.readAheadHits++;
            }
	    /*
	     * Update bytesRead after the successful FetchBlock.  The
	     * bytesRead counter counts all bytes read from the cache,
	     * including misses.
	     */
	    Fs_StatAdd(toRead, fs_Stats.blockCache.bytesRead,
		    fs_Stats.blockCache.bytesReadOverflow);
	    fs_Stats.rmtIO.hitsOnVMBlock++;
	    bcopy(blockPtr->blockAddr + (readPtr->offset &
		    FS_BLOCK_OFFSET_MASK), readPtr->buffer, toRead);
	    if (blockPtr->flags & FSCACHE_READ_AHEAD_BLOCK) {
		fs_Stats.blockCache.readAheadHits++;
	    }
	    /*
	     * Let heap pages sit in the cache.
	     */
	    if (readPtr->flags & FS_HEAP) {
		fs_Stats.rmtIO.hitsOnHeapBlock++;
		Fscache_UnlockBlock(blockPtr, 0, -1, 0,
			FSCACHE_CLEAR_READ_AHEAD);
	    } else {
		if (readPtr->flags & FS_SWAP) {
		    fs_Stats.rmtIO.hitsOnSwapPage++;
		} else if ((readPtr->flags & (FS_HEAP | FS_SWAP)) == 0) {
		    /* It's a code page */
		    fs_Stats.rmtIO.hitsOnCodePage++;
		}
		Fscache_UnlockBlock(blockPtr, 0, -1, 0,
			FSCACHE_CLEAR_READ_AHEAD | FSCACHE_BLOCK_UNNEEDED);
	    }
	} else {	/* Not found. */
	    fs_Stats.rmtIO.missesOnVMBlock++;
	    /*
	     * It's an initialized heap page.  Read it into the
	     * cache.
	     */
	    if ((readPtr->flags & FS_HEAP) &&
		    blockPtr != (Fscache_Block *) NIL) {
		Fscache_FileInfo	*cacheInfoPtr;

		fs_Stats.rmtIO.missesOnHeapBlock++;
		cacheInfoPtr = &handlePtr->cacheInfo;
		status = (cacheInfoPtr->backendPtr->ioProcs.blockRead)
			(cacheInfoPtr->hdrPtr, blockPtr, remoteWaitPtr);
		if (status == SUCCESS) {
		    /* Copy to vm block */
		    bcopy(blockPtr->blockAddr + (readPtr->offset &
			    FS_BLOCK_OFFSET_MASK), readPtr->buffer, toRead);
		    /*
		     * Update bytesRead after the read into a cache block.  The
		     * bytesRead counter counts all bytes read from the cache,
		     * including misses.  We don't update bytesRead for the
		     * non-heap block reads, since those won't be read into
		     * the cache.
		     */
		    Fs_StatAdd(toRead, fs_Stats.blockCache.bytesRead,
			    fs_Stats.blockCache.bytesReadOverflow);
		    fs_Stats.rmtIO.bytesReadForVM += toRead;
		    fs_Stats.rmtIO.bytesReadForHeap += toRead;
		    Fscache_UnlockBlock(blockPtr, 0, -1, 0,
			    FSCACHE_CLEAR_READ_AHEAD);
		} else {
		    fs_Stats.blockCache.domainReadFails++;
		}
	    }
	    if (status != SUCCESS || !(readPtr->flags & FS_HEAP) ||
		    blockPtr == (Fscache_Block *) NIL) {
		if (blockPtr != (Fscache_Block *)NIL) {
		    Fscache_UnlockBlock(blockPtr, 0, -1, 0,
			FSCACHE_DELETE_BLOCK);
		}
		if (readPtr->flags & FS_SWAP) {
		    fs_Stats.rmtIO.missesOnSwapPage++;
		} else if ((readPtr->flags & (FS_HEAP | FS_SWAP)) == 0) {
		    /* It's a code page. */
		    fs_Stats.rmtIO.missesOnCodePage++;
		}

		readPtr->length = toRead;
		status = Fsrmt_Read(streamPtr, readPtr, remoteWaitPtr,
			replyPtr);
		if (status != SUCCESS) {
		    break;
		}
		fs_Stats.rmtIO.bytesReadForVM += toRead;
		if (readPtr->flags & FS_HEAP) {
		    fs_Stats.rmtIO.bytesReadForHeapUncached += toRead;
		}
	    }
	}
	/*
	 * Successfully read one FS block, either remotely or from
	 * the cache.  Update pointers and loop.
	 */
	size -= toRead;
	readPtr->offset += toRead;
	readPtr->buffer += toRead;
    }
    if (status != SUCCESS) {
	readPtr->offset = savedOffset;
	readPtr->length = savedLength;
	readPtr->buffer = savedBuffer;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFilePageWrite --
 *
 *	Do a page-out for a remote file.
 *	NB: this knows that remote block allocation is a no-op, so
 *	there is no call to the allocation routine.
 *
 * Results:
 *	The results of Fsrmt_Write.
 *
 * Side effects:
 *	Statistics are taken.  (This could be replaced with Fsrmt_Write
 *	if these statistics aren't needed.)
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsrmtFilePageWrite(streamPtr, writePtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to remote file. */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    ReturnStatus status;

    status = Fsrmt_Write(streamPtr, writePtr, remoteWaitPtr, replyPtr);
    if (status == SUCCESS) {
	fs_Stats.rmtIO.bytesWrittenForVM += replyPtr->length;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileBlockRead --
 *
 *	Read in a cache block for a remote file.
 *
 * Results:
 *	The return code from the server.
 *
 * Side effects:
 *	This sets the blockPtr->blockSize to be the actual amount of
 *	data read into the block.  Any left over space is zero filled.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
FsrmtFileBlockRead(hdrPtr, blockPtr, waitPtr)
    Fs_HandleHeader *hdrPtr;	/* Pointer to handle for file to write to. */
    Fscache_Block *blockPtr;	/* Block to read in.  blockNum is the logical
				 * block and indicates the offset.  blockSize
				 * is set to FS_BLOCK_SIZE, but we reduce
				 * that if less gets read.  blockAddr is
				 * the memory area for the block. */
    Sync_RemoteWaiter *waitPtr;	/* For remote waiting if remote cache is full */
{
    Fs_Stream		dummyStream;
    Fs_IOParam		io;
    Fs_IOReply		reply;
    ReturnStatus	status;

    dummyStream.hdr.fileID.type = -1;
    dummyStream.ioHandlePtr = hdrPtr;

    io.buffer = blockPtr->blockAddr;
    io.length = FS_BLOCK_SIZE;
    io.offset = blockPtr->blockNum * FS_BLOCK_SIZE;
    io.flags = 0;

    status = Fsrmt_Read(&dummyStream, &io, waitPtr, &reply);
    blockPtr->blockSize = reply.length;
    if (blockPtr->blockSize < FS_BLOCK_SIZE) {
	/*
	 * We always must make sure that every cache block is filled
	 * with zeroes.  Since we didn't read a full block zero fill
	 * the rest.
	 */
	fs_Stats.blockCache.readZeroFills++;
	bzero(blockPtr->blockAddr + blockPtr->blockSize,
	    FS_BLOCK_SIZE - blockPtr->blockSize);
    }
    fs_Stats.rmtIO.bytesReadForCache += blockPtr->blockSize;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileBlockWrite --
 *
 *	Write out a cache block for a remote file.  This understands
 *	that the diskBlock is the same as the (logical) blockNum
 *	of the block.  It marks the write as coming from a client cache
 *	so the server doesn't update the modify time of the file.
 *
 *
 * Results:
 *	The return code from the RPC.
 *
 * Side effects:
 *	The write.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
FsrmtFileBlockWrite(hdrPtr, blockPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* I/O handle of file to write. */
    Fscache_Block *blockPtr;	/* The cache block to write. */
    int		flags;
{
    ReturnStatus	status;
    Fs_Stream		dummyStream;
    Fs_IOParam		io;
    Fs_IOReply		reply;

    /*
     *	The server recognizes the write RPC as coming from the
     *	cache (FS_CLIENT_CACHE_WRITE) and ignores the streamID.
     */
    dummyStream.hdr.fileID.type = -1;
    dummyStream.ioHandlePtr = hdrPtr;

    io.buffer = blockPtr->blockAddr;
    io.length = blockPtr->blockSize;
    io.offset = blockPtr->diskBlock * FS_BLOCK_SIZE; /* diskBlock == blockNum */
    io.flags = flags | FS_CLIENT_CACHE_WRITE;

    status = Fsrmt_Write(&dummyStream, &io, (Sync_RemoteWaiter *)NIL, &reply);
    if (status == SUCCESS) {
	fs_Stats.rmtIO.bytesWrittenFromCache += blockPtr->blockSize;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileBlockAllocate --
 *
 *	Allocate disk space for a remote file.  This always works, ie.
 *	the server is not contacted.  The offset is just mapped to a
 *	logical block number for the file.
 *
 *	IF THIS IS CHANGED TO ACTUALLY DO SOMETHING, then please add
 *	a call to this routine in FsrmtFilePageWrite.
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
static ReturnStatus
FsrmtFileBlockAllocate(hdrPtr, offset, numBytes, flags, blockAddrPtr, newBlockPtr)
    Fs_HandleHeader	*hdrPtr;	/* Local file handle. */
    int 		offset;		/* Offset to allocate at. */
    int 		numBytes;	/* Number of bytes to allocate. */
    int			flags;		/* FSCACHE_DONT_BLOCK */
    int			*blockAddrPtr; 	/* Disk address of block allocated. */
    Boolean		*newBlockPtr;	/* TRUE if there was no block allocated
					 * before. */
{
    *newBlockPtr = FALSE;
    *blockAddrPtr = offset / FS_BLOCK_SIZE;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileTrunc --
 *
 *	Truncate a remote file. Nothing need be done is the remote case
 *	since all the work is done on the file server.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static ReturnStatus
FsrmtFileTrunc(hdrPtr, size, delete)
    Fs_HandleHeader *hdrPtr;	/* I/O handle for the file. */
    int		    size;	/* Size to truncate to. */
    Boolean	    delete;	/* True if the file is being deleted. */
{
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileIOControl --
 *
 *	IOControls for remote regular files.  Note, no byte order
 *	checking/fixing is down here as this is always called directly from
 *	a process, never via RPC.
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
FsrmtFileIOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream *streamPtr;		/* Stream to remote file */
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register Fsrmt_FileIOHandle *handlePtr =
	    (Fsrmt_FileIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;

    Fsutil_HandleLock(handlePtr);
    switch(ioctlPtr->command) {
	case IOC_REPOSITION: {
	    if (streamPtr->flags & FS_RMT_SHARED) {
		status = Fsrmt_IOControl(streamPtr, ioctlPtr, replyPtr);
	    } else {
		status = SUCCESS;
	    }
	    break;
	}
	case IOC_GET_FLAGS:
	    if (ioctlPtr->outBufSize >= sizeof(int)) {
		*(int *)ioctlPtr->outBuffer = 0;
	    }
	    status = SUCCESS;
	    break;
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	    status = SUCCESS;
	    break;
	case IOC_TRUNCATE:
	    if ((streamPtr->flags & FS_WRITE) == 0) {
		status = FS_NO_ACCESS;
	    } else if (ioctlPtr->inBufSize >= sizeof(int)) {
		register int length = *(int *)ioctlPtr->inBuffer;
		Fscache_Trunc(&handlePtr->cacheInfo, length, 0);
		status = Fsrmt_IOControl(streamPtr, ioctlPtr, replyPtr);
	    } else {
		status = GEN_INVALID_ARG;
	    }
	    break;
	case IOC_MAP:
	    if (ioctlPtr->inBufSize >= sizeof(int)) {
		status = Fsrmt_IOControl(streamPtr, ioctlPtr, replyPtr);
	    } else {
		status = GEN_INVALID_ARG;
	    }
	    break;
	case IOC_LOCK:
	case IOC_UNLOCK:
	    status = Fsrmt_IOControl(streamPtr, ioctlPtr, replyPtr);
	    break;
	case IOC_NUM_READABLE: {
	    register int bytesAvailable;
	    register int streamOffset;

	    if (ioctlPtr->outBufSize < sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    streamOffset = *(int *)ioctlPtr->inBuffer;
	    bytesAvailable = handlePtr->cacheInfo.attr.lastByte + 1 -
				streamOffset;
	    *(int *)ioctlPtr->outBuffer = bytesAvailable;
	    status = SUCCESS;
	    break;
	}
	case IOC_SET_OWNER:
	case IOC_GET_OWNER:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_PREFIX:
	    status = SUCCESS;
	    break;
	case IOC_WRITE_BACK: {
	    /*
	     * Write out the cached data for the file.
	     */
	    Ioc_WriteBackArgs *argPtr = (Ioc_WriteBackArgs *)ioctlPtr->inBuffer;
	    Fscache_FileInfo *cacheInfoPtr = &handlePtr->cacheInfo;

	    if (ioctlPtr->inBufSize < sizeof(Ioc_WriteBackArgs)) {
		status = GEN_INVALID_ARG;
	    } else {
		int firstBlock, lastBlock;
		int blocksSkipped;
		int flags = 0;
		if (argPtr->shouldBlock) {
		    flags |= FSCACHE_FILE_WB_WAIT;
		}
		if (argPtr->firstByte > 0) {
		    firstBlock = argPtr->firstByte / FS_BLOCK_SIZE;
		} else {
		    firstBlock = 0;
		}
		if (argPtr->lastByte > 0) {
		    lastBlock = argPtr->lastByte / FS_BLOCK_SIZE;
		} else {
		    lastBlock = FSCACHE_LAST_BLOCK;
		}
		status = Fscache_FileWriteBack(cacheInfoPtr, firstBlock,
			lastBlock, flags, &blocksSkipped);
		if (status == SUCCESS) {
		    /*
		     * Perform the IOC_WRITE_BACK on the file server to 
		     * insure that the file is forced to disk.
		     */
		    status = Fsrmt_IOControl(streamPtr, ioctlPtr, replyPtr);
#define FILE_SERVER_IOC_WRITE_BACK_BROKEN
#ifdef FILE_SERVER_IOC_WRITE_BACK_BROKEN
		    if (status == GEN_INVALID_ARG) {
			/*
			 * Currently, IOC_WRITE_BACK don't work to machines
			 * with different byte orders. This has been fixed
			 * but until all file servers run the kernel we
			 * patch round the bug. 
			 * THIS CODE CAN BE REMOVED when all file servers
			 * are running a version > 1.075.
			 */
			status = SUCCESS;
		    }
#endif /* FILE_SERVER_IOC_WRITE_BACK_BROKEN */
		}

	    }
	    break;
	}
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
 * FsrmtFileGetIOAttr --
 *
 *	Update the attributes of a remotely served file from attributes
 *	that we have cached here.  The file server will have contacted
 *	other clients that may be caching the file, but we still need
 *	to check our own version of the access/modify times that we
 *	(might) have cached.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	Fills in the access time, modify time, and size from information
 *	we have cached here.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileGetIOAttr(fileIDPtr, clientID, attrPtr)
    Fs_FileID		*fileIDPtr;	/* ID on the file */
    int			clientID;	/* Host ID of calling process */
    Fs_Attributes	*attrPtr;	/* Attributes to update */
{
    Fsrmt_FileIOHandle *handlePtr;

    handlePtr = Fsutil_HandleFetchType(Fsrmt_FileIOHandle, fileIDPtr);
    if (handlePtr != (Fsrmt_FileIOHandle *)NIL) {
	Fscache_UpdateAttrFromCache(&handlePtr->cacheInfo, attrPtr);
	Fsutil_HandleRelease(handlePtr, TRUE);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtFileSetIOAttr --
 *
 *	Set the attributes of a remotely served file.  This routine is
 *	called after the file server has been contacted, so its only
 *	job is to update attributes we have cached here on the client.
 *
 * Results:
 *	SUCCESS or FS_STALE_HANDLE.
 *
 * Side effects:
 *	Updates the access time, modify time, and size that
 *	we have cached here.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtFileSetIOAttr(fileIDPtr, attrPtr, flags)
    Fs_FileID		*fileIDPtr;	/* ID on the file */
    Fs_Attributes	*attrPtr;	/* Attributes to update */
    int			flags;		/* What attrs to set */
{
    Fsrmt_FileIOHandle *handlePtr;

    handlePtr = Fsutil_HandleFetchType(Fsrmt_FileIOHandle, fileIDPtr);
    if (handlePtr != (Fsrmt_FileIOHandle *)NIL) {
	Fscache_UpdateCachedAttr(&handlePtr->cacheInfo, attrPtr, flags);
	Fsutil_HandleRelease(handlePtr, TRUE);
    }
    return(SUCCESS);
}

static void FsrmtCleanBlocks _ARGS_((ClientData	data,
				Proc_CallInfo *callInfoPtr));


/*
 *----------------------------------------------------------------------
 *
 * FsrmtStartWriteBack --
 *
 * 	Start a block cleaner process for the specified domain.
 *
 * Results:
 *	TRUE if a block cleaner was started.
 *
 * Side effects:
 *	Number of block cleaner processes may be incremented.
 *
 * ----------------------------------------------------------------------------
 */
static Boolean
FsrmtStartWriteBack(backendPtr)
    Fscache_Backend *backendPtr;	/* Backend to start writeback. */
{
    LOCK_MONITOR;

    if (fsrmtBlockCleaners < fscache_MaxBlockCleaners) {
	Proc_CallFunc(FsrmtCleanBlocks, (ClientData) backendPtr, 0);
	fsrmtBlockCleaners++;
	UNLOCK_MONITOR;
	return TRUE;
    }
    UNLOCK_MONITOR;
    return FALSE;
}


/*
 * ----------------------------------------------------------------------------
 *
 *	Functions to clean dirty blocks.
 *
 * ----------------------------------------------------------------------------
 */


/*
 * ----------------------------------------------------------------------------
 *
 * FsrmtCleanBlocks
 *
 *	Write all blocks on the dirty list to disk.  Called either from
 *	a block cleaner process or synchronously during system shutdown.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *     	The dirty list is emptied.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FsrmtCleanBlocks(data, callInfoPtr)
    ClientData		data;		/* Background flag.  If TRUE it means
					 * we are called from a block cleaner
					 * process.  Otherwise we being called
					 * synchrounously during a shutdown */
    Proc_CallInfo	*callInfoPtr;	/* Not Used. */
{
    Fscache_Block	*blockPtr;
    ReturnStatus		status;
    int				lastDirtyBlock;
    Fscache_FileInfo		*cacheInfoPtr;
    Fscache_Backend		*backendPtr;

    backendPtr = (Fscache_Backend *) data;
    cacheInfoPtr = Fscache_GetDirtyFile(backendPtr, TRUE, 
		FileMatch, (ClientData) 0);
    while (cacheInfoPtr != (Fscache_FileInfo *)NIL) {
	blockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, BlockMatch,
			(ClientData) 0,  &lastDirtyBlock);
	while (blockPtr != (Fscache_Block *) NIL) {
	    /*
	     * Write the block.
	     */
	    status = backendPtr->ioProcs.blockWrite
		    (cacheInfoPtr->hdrPtr, blockPtr, 
			lastDirtyBlock ? FS_LAST_DIRTY_BLOCK : 0);
	    Fscache_ReturnDirtyBlock( blockPtr, status);
	    blockPtr = Fscache_GetDirtyBlock(cacheInfoPtr, BlockMatch,
			(ClientData) 0, &lastDirtyBlock);
	}
	Fscache_ReturnDirtyFile(cacheInfoPtr, FALSE);
	cacheInfoPtr = Fscache_GetDirtyFile(backendPtr, TRUE,
			FileMatch, (ClientData) 0);
    }
    FscacheBackendIdle(backendPtr);
    LOCK_MONITOR;
    fsrmtBlockCleaners--;
    UNLOCK_MONITOR;
}



/*
 * ----------------------------------------------------------------------------
 *
 * FsrmtReallocBlock --
 *
 * 	Reallocate a block which got a write error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FsrmtReallocBlock(data, callInfoPtr)
    ClientData		data;			/* Block to move */
    Proc_CallInfo	*callInfoPtr;		/* Not used. */
{
    panic("FsrmtReallocBlock called\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * BlockMatch --
 *
 * 	Cache backend block type match.  Fsrmt doesn't care about the 
 *	order of blocks returned by GetDirtyBlocks.
 *
 * Results:
 *	TRUE.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Boolean
BlockMatch( blockPtr, cleintData)
    Fscache_Block *blockPtr;
    ClientData	cleintData;
{
    return TRUE;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FileMatch --
 *
 * 	Cache backend files match.  Fsrmt doesn't care about the 
 *	order of files returned by GetDirtyFiles.
 *
 * Results:
 *	TRUE.
 *
 * Side effects:
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Boolean
FileMatch( cacheInfoPtr, cleintData)
    Fscache_FileInfo *cacheInfoPtr;
    ClientData	cleintData;
{
    return TRUE;
}
