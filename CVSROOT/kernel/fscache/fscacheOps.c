/* 
 * fsCacheOps.c --
 *
 *	Cache routines that are monitored with the lock in the per-file
 *	cacheInfo structure.  This includes high level read and write
 *	routines, and routines that fiddle with the attributes that
 *	are cached.
 *	
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
#include "fscache.h"
#include "fsconsist.h"
#include "fsNameOps.h"
#include "fsdm.h"
#include "fsStat.h"
#include "fsutilTrace.h"
#include "fslcl.h"
#include "fsBlockCache.h"
#include "vm.h"
#include "spriteTime.h"
#include "timer.h"
#include "sys.h"
#include "rpc.h"

#define	BLOCK_ALIGNED(offset) (((offset) & ~FS_BLOCK_OFFSET_MASK) == (offset))

/*
 * Cache I/O is serialized using a monitor lock on the cache state for
 * each file.
 */
#define LOCKPTR (&cacheInfoPtr->lock)


/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_UpdateFile --
 *
 * 	This routine updates the cacheInfo for a file based on info returned
 *	from the server at open time.  It checks version numbers and the
 *	cachability of the file, and will do any required invalidations.
 *
 * Results:
 *	True if the file in the cache is different than before.  This is
 *	used to tell VM that the cached segment representing the file
 *	is no longer valid.
 *
 * Side effects:
 *	Updates the version number.  May invalidate old blocks.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
Fscache_UpdateFile(cacheInfoPtr, openForWriting, version, cacheable, attrPtr)
    Fscache_FileInfo *cacheInfoPtr;	/* Cache state of file to update. */
    Boolean		openForWriting;	/* TRUE if opening for writing */
    int			version;	/* Version number used to verify
					 * our cached data blocks. */
    Boolean		cacheable;	/* TRUE if server says we can cache. */
    Fscache_Attributes	*attrPtr;	/* Attributes from server */
{
    register Boolean outOfDate;
    Boolean changed = FALSE;
#ifdef CONSIST_DEBUG
    extern int fsTraceConsistMinor;
#endif
    LOCK_MONITOR;

    /* 
     * See if we have a good cached copy.  Note: if we open for writing
     * the returned version number will be one greater than ours
     * if no-one else has modified the file since we cached it.
     */

    if (openForWriting) {
	outOfDate = (cacheInfoPtr->version < version - 1);
    } else {
	outOfDate = (cacheInfoPtr->version < version);
    }
#ifdef CONSIST_DEBUG
    if (fsTraceConsistMinor == cacheInfoPtr->hdrPtr->fileID.minor) {
	printf("Fscache_UpdateFile: <%d,%d> version %d->%d, %s, %s\n",
		    cacheInfoPtr->hdrPtr->fileID.major,
		    cacheInfoPtr->hdrPtr->fileID.minor,
		    cacheInfoPtr->version, version,
		    (cacheable ? "cacheable" : "not-cacheable"),
		    (outOfDate ? "out of date" : "not out of date"));
    }
#endif /* CONSIST_DEBUG */
    if (version > cacheInfoPtr->version) {
	/*
	 * Update the version of the handle, ie. we just opened for writing
	 * or had an outOfDate version.
	 */
	cacheInfoPtr->version = version;
	changed = TRUE;
    }
    if (cacheInfoPtr->flags & FSCACHE_FILE_GONE) {
	/*
	 * Reset the deleted bit.  By this time any deletion
	 * actions will have completed.
	 */
	cacheInfoPtr->flags &= ~FSCACHE_FILE_GONE;
    }

    /*
     * If we were caching and the file is no longer cacheable, or our copy
     * is outOfDate, we need to invalidate our cache.  Note that we do
     * the cache invalidate with our previous notion of the size.
     */
    if (!cacheable || outOfDate) {
	fs_Stats.handle.cacheFlushes++;
	Fscache_FileInvalidate(cacheInfoPtr, 0, FSCACHE_LAST_BLOCK);
    }
    if (outOfDate) {
	fs_Stats.handle.versionMismatch++;
	cacheInfoPtr->attr = *attrPtr;
    }

    /*
     * Propogate the cachability of the handle.
     */
    if (cacheable) {
	if (cacheInfoPtr->flags & FSCACHE_FILE_NOT_CACHEABLE) {
	    /*
	     * The handle wasn't client cacheable before.  In this
	     * case mark the handle as cacheable and update the cached
	     * attributes since they could have changed 
	     * while we didn't have the file cached.
	     */
	    cacheInfoPtr->attr = *attrPtr;
	    cacheInfoPtr->flags &= ~FSCACHE_FILE_NOT_CACHEABLE;
	}
    } else {
	cacheInfoPtr->flags |= FSCACHE_FILE_NOT_CACHEABLE;
    }

    /* 
     * Update the handle's access time.  The access time doesn't get
     * pushed out to all clients concurrently read-sharing a file,
     * so we grab it here if we are out-of-date.
     */
    if (attrPtr->accessTime > cacheInfoPtr->attr.accessTime) {
	cacheInfoPtr->attr.accessTime = attrPtr->accessTime;
    }
    UNLOCK_MONITOR;
    return (changed);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_UpdateAttrFromClient --
 *
 * 	This is used on the server to update the attributes of a handle
 *	that has been cached on a client.  Called at close time with
 *	the attributes the client sends over with the close RPC.
 *	The client's times are checked against the servers to ensure
 *	that they only increase and never move backwards.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the access and modify time if they are greater than
 *	the local attribute.  Updates the first and last byte indexes.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_UpdateAttrFromClient(clientID, cacheInfoPtr, attrPtr)
    int clientID;				/* Client, for warning msg */
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state to update. */
    register Fscache_Attributes	*attrPtr;	/* Attributes from client */
{
    LOCK_MONITOR;

    if (attrPtr->modifyTime > cacheInfoPtr->attr.modifyTime) {
	cacheInfoPtr->attr.modifyTime = attrPtr->modifyTime;
    }
    if (attrPtr->accessTime > cacheInfoPtr->attr.accessTime) {
	cacheInfoPtr->attr.accessTime = attrPtr->accessTime;
    }
    if (cacheInfoPtr->attr.lastByte > attrPtr->lastByte) {
	printf(
	"Fscache_UpdateAttrFromClient %d: \"%s\" <%d,%d> short size %d not %d\n",
		clientID,
		Fsutil_HandleName(cacheInfoPtr->hdrPtr),
		cacheInfoPtr->hdrPtr->fileID.major,
		cacheInfoPtr->hdrPtr->fileID.minor,
		attrPtr->lastByte, cacheInfoPtr->attr.lastByte);
    } else {
	cacheInfoPtr->attr.lastByte = attrPtr->lastByte;
    }
    cacheInfoPtr->attr.firstByte = attrPtr->firstByte;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_UpdateDirSize --
 *
 * 	This is used on the server to update the size of a directory
 *	when it grows due to additions.  Note that directories don't
 *	get smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the last byte index.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_UpdateDirSize(cacheInfoPtr, newLastByte)
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state to update. */
    register int newLastByte;			/* New last byte index */
{
    LOCK_MONITOR;
    if (newLastByte > cacheInfoPtr->attr.lastByte) {
	cacheInfoPtr->attr.lastByte = newLastByte;
    }
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_UpdateAttrFromCache --
 *
 * 	This is used on a client to update the given attributes from
 *	information it has cached.  A client caches the access time,
 *	modify time, and size of a file.  When the server returns attributes,
 *	this routine is called to complete them from info cached here
 *	on the client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the access and modify time of *attrPtr if they are less than
 *	the cached value.  Always updates the first and last byte indexes
 *	with the cached value.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_UpdateAttrFromCache(cacheInfoPtr, attrPtr)
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state to check. */
    register Fs_Attributes	*attrPtr;	/* Attributes from server */
{
    LOCK_MONITOR;
    if ((cacheInfoPtr->version == attrPtr->version) &&
	(cacheInfoPtr->flags & FSCACHE_FILE_NOT_CACHEABLE) == 0) {
	if (cacheInfoPtr->attr.accessTime > attrPtr->accessTime.seconds) {
	    attrPtr->accessTime.seconds = cacheInfoPtr->attr.accessTime;
	}
	if (cacheInfoPtr->attr.modifyTime > attrPtr->dataModifyTime.seconds) {
	    attrPtr->dataModifyTime.seconds = cacheInfoPtr->attr.modifyTime;
	}
	attrPtr->size = cacheInfoPtr->attr.lastByte + 1;
	if (cacheInfoPtr->attr.firstByte > 0) {
	    attrPtr->size -= cacheInfoPtr->attr.firstByte;
	}
    }
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_GetCachedAttr --
 *
 * 	This is used on a server to fill out the attributes returned to
 *	a client for caching there.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_GetCachedAttr(cacheInfoPtr, versionPtr, attrPtr)
    register Fscache_FileInfo	*cacheInfoPtr;	/* Cache state to update. */
    register int		*versionPtr;	/* Version number of file */
    register Fscache_Attributes	*attrPtr;	/* New attributes from server */
{
    LOCK_MONITOR;
    *versionPtr = cacheInfoPtr->version;
    *attrPtr = cacheInfoPtr->attr;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_UpdateCachedAttr --
 *
 * 	This is called during an Fs_SetAttributes call to update attributes
 *	that are cached in a handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the times, permissions, file type, and ownership info,
 *	depending on the flags argument.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
Fscache_UpdateCachedAttr(cacheInfoPtr, attrPtr, flags)
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state to update. */
    register Fs_Attributes   *attrPtr;		/* New attributes */
    register int	     flags;		/* What attrs to update */
{
    LOCK_MONITOR;
    if (flags & FS_SET_TIMES) {
	cacheInfoPtr->attr.accessTime = attrPtr->accessTime.seconds;
	cacheInfoPtr->attr.modifyTime = attrPtr->dataModifyTime.seconds;
	cacheInfoPtr->attr.createTime = attrPtr->createTime.seconds;
    }
    if (flags & FS_SET_MODE) {
	cacheInfoPtr->attr.permissions = attrPtr->permissions;
    }
    if (flags & FS_SET_OWNER) {
	if (attrPtr->uid >= 0) {
	    cacheInfoPtr->attr.uid = attrPtr->uid;
	}
	if (attrPtr->gid >= 0) {
	    cacheInfoPtr->attr.gid = attrPtr->gid;
	}
    }
    if ((flags & FS_SET_FILE_TYPE) &&
	attrPtr->userType != FS_USER_TYPE_UNDEFINED) {
	cacheInfoPtr->attr.userType = attrPtr->userType;
    }
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_CheckVersion --
 *
 * 	This is used during recovery on a server to see if the client
 *	has an ok version of the file.
 *
 * Results:
 *	FS_VERSION_MISMATCH if the client's version number is out-of-date.
 *
 * Side effects:
 *	None, except for a print statement.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY ReturnStatus
Fscache_CheckVersion(cacheInfoPtr, version, clientID)
    register Fscache_FileInfo	*cacheInfoPtr;	/* Cache state to check. */
    int				version;
    int				clientID;
{
    ReturnStatus status = SUCCESS;
    LOCK_MONITOR;
    if (version != cacheInfoPtr->version) {
	printf(
    "Version mismatch: clt %d, srv %d, file \"%s\" <%d,%d>, from client %d\n",
	    version, cacheInfoPtr->version, Fsutil_HandleName(cacheInfoPtr->hdrPtr),
	    cacheInfoPtr->hdrPtr->fileID.major,
	    cacheInfoPtr->hdrPtr->fileID.minor, clientID);
	status = FS_VERSION_MISMATCH;
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_OkToScavenge --
 *
 * 	This is called at handle scavenge time to see if it is ok to
 *	scavenge the handle.  This calls a routine in fsBlockCache.c
 *	which gets the global cache monitor lock because the blocksInCache
 *	attribute and FS_FILE_ON_DIRTY list flags is modified under that lock.
 *
 * Results:
 *	TRUE if there is no information in the cache for the file.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
Fscache_OkToScavenge(cacheInfoPtr)
    register Fscache_FileInfo	*cacheInfoPtr;	/* Cache state to check. */
{
    register Boolean ok;
    LOCK_MONITOR;
    ok = FscacheBlockOkToScavenge(cacheInfoPtr);
    UNLOCK_MONITOR;
    return(ok);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fscache_Consist --
 *
 * 	This is called from ProcessConsist to take a cache consistency
 *	action in response to a callback from the file server.
 *
 * Results:
 *	The return status from the block cache operations, plus the
 *	values of the attributes we have cached here on the client.
 *
 * Side effects:
 *	Will writeback and/or invalidate the cache according to the
 *	request by the server.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY ReturnStatus
Fscache_Consist(cacheInfoPtr, flags, cachedAttrPtr)
    register Fscache_FileInfo	*cacheInfoPtr;
    int				flags;
    Fscache_Attributes		*cachedAttrPtr;
{
    ReturnStatus status;
    int firstBlock;
    int numSkipped;
    int mig;

    LOCK_MONITOR;

    if (cacheInfoPtr->attr.firstByte == -1) {
	firstBlock = 0;
    } else {
	firstBlock = cacheInfoPtr->attr.firstByte / FS_BLOCK_SIZE;
    }
    status = SUCCESS;
    mig = (flags & FSCONSIST_MIGRATION) ? FSCACHE_WB_MIGRATION : 0;
    switch (flags & ~(FSCONSIST_DEBUG|FSCONSIST_MIGRATION)) {
	case FSCONSIST_WRITE_BACK_BLOCKS:
	    status = Fscache_FileWriteBack(cacheInfoPtr, firstBlock,
			FSCACHE_LAST_BLOCK, FSCACHE_FILE_WB_WAIT | mig,
					   &numSkipped);
	    break;
	case FSCONSIST_CANT_CACHE_NAMED_PIPE:
	case FSCONSIST_INVALIDATE_BLOCKS:
	    Fscache_FileInvalidate(cacheInfoPtr, firstBlock,
				   FSCACHE_LAST_BLOCK);
	    cacheInfoPtr->flags |= FSCACHE_FILE_NOT_CACHEABLE;
	    break;
	case FSCONSIST_INVALIDATE_BLOCKS | FSCONSIST_WRITE_BACK_BLOCKS:
	    status = Fscache_FileWriteBack(cacheInfoPtr, firstBlock,
					   FSCACHE_LAST_BLOCK,
					   FSCACHE_WRITE_BACK_AND_INVALIDATE |
					   FSCACHE_FILE_WB_WAIT | mig,
					   &numSkipped);
	    cacheInfoPtr->flags |= FSCACHE_FILE_NOT_CACHEABLE;
	    break;
	case FSCONSIST_DELETE_FILE:
	    cacheInfoPtr->flags |= FSCACHE_FILE_GONE;
	    Fscache_FileInvalidate(cacheInfoPtr, firstBlock, FSCACHE_LAST_BLOCK);
	    cacheInfoPtr->flags |= FSCACHE_FILE_NOT_CACHEABLE;
	    break;
	case FSCONSIST_WRITE_BACK_ATTRS:
	    break;
	default:
	    printf("Fscache_Consist: Bad consistency action %x\n", flags);
	    status = FS_INVALID_ARG;
	    break;
    }
    *cachedAttrPtr = cacheInfoPtr->attr;

    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_Read --
 *
 *	Read from the block cache.
 *
 * Results:
 *	SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *	Fill in the buffer and the cache with data from the stream.
 *	The cache state for the file is locked to serialize I/O.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fscache_Read(cacheInfoPtr, flags, buffer, offset, lenPtr, remoteWaitPtr)
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state for file. */
    int			flags;		/* FS_USER | FS_CONSUME */
    register Address	buffer;		/* Buffer to fill with file data */
    int 		offset;		/* Byte offset */
    int 		*lenPtr;	/* In/Out byte count */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
{
    register int 	size;		/* Amount left to read */
    register int 	blockNum;	/* Current block being read */
    register int 	toRead;		/* Amount to read each iteration */
    ReturnStatus	status = SUCCESS; /* Return from I/O operations */
    int			firstBlock;	/* First block of the read */
    int			lastBlock;	/* Last block of the read */
    Boolean		found;		/* For fetching blocks from cache */
    Fscache_Block	*blockPtr;	/* For fetching blocks from cache */
    int			dontBlock;	/* FSCACHE_DONT_BLOCK */

    /*
     * Serialiaze access to the cache for this file.
     */
    LOCK_MONITOR;

    if (cacheInfoPtr->flags & FSCACHE_FILE_NOT_CACHEABLE) {
	status = FS_NOT_CACHEABLE;
	goto exit;
    }
    /*
     * Determine the offset at which to read, consuming reads always
     * start at the beginning of the data.
     */
    if (flags & FS_CONSUME) {
	if (cacheInfoPtr->attr.firstByte == -1) {
	    *lenPtr = 0;
	    status = SUCCESS;
	    goto exit;
	} else {
	    offset = cacheInfoPtr->attr.firstByte;
	}
    }
    if (offset > cacheInfoPtr->attr.lastByte) {
	*lenPtr = 0;
	status = SUCCESS;
	goto exit;
    }

    if ((remoteWaitPtr != (Sync_RemoteWaiter *)NIL) &&
	(remoteWaitPtr->pid != (Proc_PID)NIL)) {
	dontBlock = FSCACHE_DONT_BLOCK;
    } else {
	dontBlock = 0;
    }
    /*
     * Fetch blocks one at a time.
     */
    size = *lenPtr;
    firstBlock = (unsigned int) offset / FS_BLOCK_SIZE; 
    lastBlock = (unsigned int) (offset + size - 1) / FS_BLOCK_SIZE;

    for (blockNum = firstBlock; 
	 blockNum <= lastBlock && offset <= cacheInfoPtr->attr.lastByte; 
	 blockNum++, size -= toRead, buffer += toRead, offset += toRead) {

	/*
	 * Initiate read ahead on the next block.
	 */
	FscacheReadAhead(cacheInfoPtr, blockNum + 1);

	/*
	 * Determine the number of bytes to transfer out of the cache block
	 * in this go around.
	 */
	toRead = size;
	if ((unsigned int) (offset + size - 1) / FS_BLOCK_SIZE > blockNum) {
	    toRead = (blockNum + 1) * FS_BLOCK_SIZE - offset;
	}
	if (toRead > cacheInfoPtr->attr.lastByte - offset + 1) {
	    toRead = cacheInfoPtr->attr.lastByte - offset + 1;
	}

	/* 
	 * Get the block from the cache.  If the block isn't in the cache
	 * then read data into it.
	 */
	Fscache_FetchBlock(cacheInfoPtr, blockNum,
		FSCACHE_DATA_BLOCK|dontBlock, &blockPtr, &found);
	if (blockPtr == (Fscache_Block *)NIL) {
	    /*
	     * Cache is full.
	     */
	    status = FS_WOULD_BLOCK;
	    Fsutil_WaitListInsert(fscacheFullWaitList, remoteWaitPtr);
	    break;
	}
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
	} else {
	    /*
	     * We didn't find the block in the cache so we have to read it in.
	     * Fscache_FetchBlock has set the blockNum and blockAddr (buffer).
	     * blockSize gets set as a side-effect of the block read routine.
	     */
	    status = (cacheInfoPtr->ioProcsPtr->blockRead)
			(cacheInfoPtr->hdrPtr, blockPtr, remoteWaitPtr);
#ifdef lint
	    status = Fsio_FileBlockRead(cacheInfoPtr->hdrPtr,
			blockPtr, remoteWaitPtr);
	    status = FsrmtFileBlockRead(cacheInfoPtr->hdrPtr,
			blockPtr, remoteWaitPtr);
#endif /* lint */
	    if (status != SUCCESS) {
		fs_Stats.blockCache.domainReadFails++;
		Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
		break;
	    }
        }

	/*
	 * Copy the bytes out of the cache block.
	 */
	if (flags & FS_USER) {
	    if (Vm_CopyOut(toRead, 
		          blockPtr->blockAddr + (offset & FS_BLOCK_OFFSET_MASK),
			  buffer) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
		Fscache_UnlockBlock(blockPtr, 0, -1, 0,
			FSCACHE_CLEAR_READ_AHEAD);
		break;
	    }
	} else {
	    bcopy(blockPtr->blockAddr + (offset & FS_BLOCK_OFFSET_MASK),
		      buffer, toRead);
	}
	Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_CLEAR_READ_AHEAD);
    }
    *lenPtr -= size;
    Fs_StatAdd(*lenPtr, fs_Stats.blockCache.bytesRead,
	       fs_Stats.blockCache.bytesReadOverflow);
#ifndef CLEAN
    if (fsdmKeepTypeInfo) {
	int fileType;

	fileType = Fsdm_FindFileType(cacheInfoPtr);
	fs_TypeStats.cacheBytes[FS_STAT_READ][fileType] += *lenPtr;
    }
#endif CLEAN

    /*
     * Consume data if the flags indicate a consuming stream.
     * This doesn't update the modify time of the file.
     */
    if (flags & FS_CONSUME) {
	int length;
	length = cacheInfoPtr->attr.lastByte - (int) offset + 1;
	(void)(*fsio_StreamOpTable[cacheInfoPtr->hdrPtr->fileID.type].ioControl)
		(cacheInfoPtr->hdrPtr, IOC_TRUNCATE, mach_Format,
			sizeof(length), (Address) &length, 0, (Address) NIL); 
    }
exit:
    if ((status == SUCCESS) ||
	(status == FS_WOULD_BLOCK && (*lenPtr > 0))) {
	cacheInfoPtr->attr.accessTime = fsutil_TimeInSeconds;
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_Write --
 *
 *	Write to the cache.  Called from the file write and named pipe
 *	write routines.
 * 
 * Results:
 *	A return status, SUCCESS if successful.
 *
 * Side effects:
 *	Data is put into the cache.  Blocks may be read if only partial
 *	blocks are being written.  The cache state is locked during the I/O
 *	to serialize cache access.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fscache_Write(cacheInfoPtr, flags, buffer, offset, lenPtr, remoteWaitPtr)
    register Fscache_FileInfo *cacheInfoPtr;	/* Cache state for the file. */
    int			flags;		/* FS_USER | FS_APPEND | 
					 * FS_SERVER_WRITE_THRU */
    register Address 	buffer;		/* Buffer to write from */
    int 		offset;		/* Byte offset */
    int 		*lenPtr;	/* In/Out byte count */
    Sync_RemoteWaiter 	*remoteWaitPtr;	/* Process info for remote waiting */
{
    register int 	size;		/* The current size left to write */
    register int 	toWrite;	/* Amount to write each iteration */
    int			toAlloc;	/* Amount of disk space to allocate.*/
    register int 	blockNum;	/* The current block being written */
    Fscache_Block 	*blockPtr;	/* From fetching cached blocks */
    int			blockSize;	/* The number of bytes in the current
					 * block. */
    ReturnStatus 	status = SUCCESS; /* Return from I/O operations */
    int			firstBlock;	/* The first block of the write */
    int			lastBlock;	/* The last block to write */
    Boolean		found;		/* From fetching cached blocks */
    int			oldOffset;	/* Initial value of the offset */
    int			lastFileBlock;	/* Last block in the file */
    int			blockAddr;	/* For allocating blocks */
    Boolean		newBlock;	/* A brand new block was allocated. */
    int			bytesToFree; 	/* number of bytes overwritten
					 * in file */
    int			modTime;	/* File modify time. */
    Boolean		dontBlock;	/* TRUE if lower levels shouldn't block
					 * because we can block up higher */

    /*
     * Serialize access to the cache for this file.
     */
    LOCK_MONITOR;

    if (cacheInfoPtr->flags & FSCACHE_FILE_NOT_CACHEABLE) {
	/*
	 * Not cached.  The flag is checked here under monitor lock.
	 */
	status = FS_NOT_CACHEABLE;
	goto exit;
    }
    if (cacheInfoPtr->flags & FSCACHE_FILE_GONE) {
	/*
	 * A delayed write is arriving as the file is being deleted.
	 */
	printf( "Write to deleted file #%d\n",
		    cacheInfoPtr->hdrPtr->fileID.minor);
	status = FS_FILE_REMOVED;
	*lenPtr = 0;
	goto exit;
    }
    /*
     * Determine where to start writing.
     */
    if (flags & FS_APPEND) {
	offset = cacheInfoPtr->attr.lastByte + 1;
    }
    oldOffset = offset;
    size = *lenPtr;
    *lenPtr = 0;
    bytesToFree = 0;

    /*
     * Determine the range of blocks to write and where the current last block
     * in the file is.
     */
    firstBlock = (unsigned int) offset / FS_BLOCK_SIZE; 
    lastBlock = (unsigned int) (offset + size - 1) / FS_BLOCK_SIZE;
    if (cacheInfoPtr->attr.lastByte == -1) {
	lastFileBlock = -1;
    } else {
	lastFileBlock = ((unsigned int) cacheInfoPtr->attr.lastByte) /
				FS_BLOCK_SIZE;
    }
    /*
     * If we have a handle on a process then our caller can block it.
     * Otherwise we have to block at a low level in FetchBlock.
     */
    if ((remoteWaitPtr != (Sync_RemoteWaiter *)NIL) &&
	(remoteWaitPtr->pid != (Proc_PID)NIL)) {
	dontBlock = FSCACHE_DONT_BLOCK;
    } else {
	dontBlock = 0;
    }

    /*
     * Put the data into the cache a block at a time.
     */
    for (blockNum = firstBlock; 
	 blockNum <= lastBlock; 
	 blockNum++, size -= toWrite, buffer += toWrite, offset += toWrite) {

	/*
	 * Determine the number of bytes to write into the cache block
	 * this go around and the number of bytes to allocate on disk.
	 */
	if ((unsigned int) (offset + size - 1) / FS_BLOCK_SIZE > blockNum) {
	    /*
	     * Writing will go into the next block.  In this case fill up the 
	     * rest of the current block and allocate space for the rest of
	     * the current block.
	     */
	    toWrite = FS_BLOCK_SIZE - (offset & FS_BLOCK_OFFSET_MASK);
	    toAlloc = toWrite;
	} else {
	    /*
	     * The write ends in this block.
	     * There are two cases for disk allocation:
	     *   1) We are writing before the last byte in the file.  In this
	     *      case allocate up to the last byte or the end of the
	     *      current block whichever comes first.
	     *   2) We are after the last byte in the file.  In this case just
	     *	    allocate space for the newly written bytes.
	     */
	    toWrite = size;
	    if (offset + toWrite - 1 < cacheInfoPtr->attr.lastByte) {
		if (cacheInfoPtr->attr.lastByte >=
			    (blockNum + 1) * FS_BLOCK_SIZE) {
		    toAlloc = FS_BLOCK_SIZE - (offset & FS_BLOCK_OFFSET_MASK);
		} else {
		    toAlloc = cacheInfoPtr->attr.lastByte - offset + 1;
		}
	    } else {
		toAlloc = toWrite;
	    }
	}

	/*
	 * Allocate space behind the cache block.
	 */
	status = (cacheInfoPtr->ioProcsPtr->allocate)(cacheInfoPtr->hdrPtr,
		    offset, toAlloc, dontBlock, &blockAddr, &newBlock);
#ifdef lint
	status = Fsdm_BlockAllocate(cacheInfoPtr->hdrPtr,
		    offset, toAlloc, dontBlock, &blockAddr, &newBlock);
	status = FsrmtBlockAllocate(cacheInfoPtr->hdrPtr,
		    offset, toAlloc, dontBlock, &blockAddr, &newBlock);
#endif /* lint */

	if (blockAddr == FSDM_NIL_INDEX) {
	    if (status == SUCCESS) {
		status = FS_NO_DISK_SPACE;
	    }
	    blockPtr = (Fscache_Block *)NIL;
	} else {
	    fs_Stats.blockCache.writeAccesses++;
	    Fscache_FetchBlock(cacheInfoPtr, blockNum, 
		 (int)(FSCACHE_IO_IN_PROGRESS | FSCACHE_DATA_BLOCK | dontBlock),
		 &blockPtr, &found);
	    if (blockPtr == (Fscache_Block *)NIL) {
		status = FS_WOULD_BLOCK;
	    }
	}
	if (blockPtr == (Fscache_Block *)NIL) {
	    if (status == FS_NO_DISK_SPACE) {
		/*
		 * Limit the number of "Alloc failed" messages to 1 per file.
		 * Servers with hard-copy consoles like this feature.
		 */
		if ((cacheInfoPtr->flags & FSCACHE_ALLOC_FAILED) == 0) {
		    cacheInfoPtr->flags |= FSCACHE_ALLOC_FAILED;
		    printf("Fscache_Write: Alloc failed <%d,%d> \"%s\" %s\n",
			cacheInfoPtr->hdrPtr->fileID.major,
			cacheInfoPtr->hdrPtr->fileID.major,
			Fsutil_HandleName(cacheInfoPtr->hdrPtr),
			"DISK FULL");
		}
	    }
	    break;
	} else {
	    cacheInfoPtr->flags &= ~FSCACHE_ALLOC_FAILED;
	}


	if (toWrite == FS_BLOCK_SIZE) {
	    if (found) {
		fs_Stats.blockCache.overWrites++;
		bytesToFree += FS_BLOCK_SIZE;
	    }
	} else {
	    /*
	     * Check if are writing into the middle of the file and are not 
	     * overwriting the block.  If so have to read the block in if
	     * it is not in the cache.
	     */
	    if (blockNum <= lastFileBlock && !newBlock) {
		if (found) {
		    fs_Stats.blockCache.partialWriteHits++;
		    if (blockPtr->flags & FSCACHE_READ_AHEAD_BLOCK) {
			fs_Stats.blockCache.readAheadHits++;
		    }
		} else {
		    fs_Stats.blockCache.partialWriteMisses++;
		    status = (cacheInfoPtr->ioProcsPtr->blockRead)
			(cacheInfoPtr->hdrPtr, blockPtr, remoteWaitPtr);
#ifdef lint
		    status = Fsio_FileBlockRead(cacheInfoPtr->hdrPtr, 
				blockPtr, remoteWaitPtr);
		    status = FsrmtFileBlockRead(cacheInfoPtr->hdrPtr,
				blockPtr, remoteWaitPtr);
#endif /* lint */
		    if (status != SUCCESS) {
			fs_Stats.blockCache.domainReadFails++;
			Fscache_UnlockBlock(blockPtr, 0, -1, 0,
			    FSCACHE_DELETE_BLOCK);
			break;
		    }
		}
		bytesToFree += toWrite;
	    } else {
		/*
		 * We are writing to the end of the file or the block
		 * that we are writing to is brand new.  In this case zero
		 * fill the block if it isn't in the cache yet.
		 */
		if (!found) {
		    fs_Stats.blockCache.writeZeroFills2++;
		    bzero(blockPtr->blockAddr, FS_BLOCK_SIZE);
		}
	    }
	}

	/*
	 * Copy the bytes into the block.
	 */
	if (flags & FS_USER) {
	    if (Vm_CopyIn(toWrite, buffer, blockPtr->blockAddr + 
				(offset & FS_BLOCK_OFFSET_MASK)) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
		Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
		break;
	    }
	} else {
	    bcopy(buffer, blockPtr->blockAddr + (offset & FS_BLOCK_OFFSET_MASK), toWrite);
	}

	/*
	 * If the block is write-thru then write the data through to the 
	 * server and then check the block back in as clean.
	 * THIS IS A GORY HACK that limits the use of FS_SERVER_WRITE_THRU
	 * to the client.
	 */
	if (flags & FS_SERVER_WRITE_THRU) {
	    Fs_Stream	dummyStream;
	    Fs_IOParam	io;
	    Fs_IOReply	reply;

	    io.buffer = blockPtr->blockAddr + (offset & FS_BLOCK_OFFSET_MASK);
	    io.length = toWrite;
	    io.offset = offset;
	    io.flags = flags | FS_CLIENT_CACHE_WRITE;
	    io.flags &= ~(FS_USER | FS_SERVER_WRITE_THRU);

	    dummyStream.hdr.fileID.type = -1;
	    dummyStream.ioHandlePtr = cacheInfoPtr->hdrPtr;
	    status = Fsrmt_Write(&dummyStream, &io,
			 (Sync_RemoteWaiter *)NIL, &reply);
	    if (status != SUCCESS) {
		Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
		break;
	    }
	    modTime = 0;
	} else {
	    modTime = fsutil_TimeInSeconds;
	}

	/*
	 * Return the block to the cache.  At this time the block
	 * size is changed (by UnlockBlock). 
	 */
	if (offset + toWrite - 1 > cacheInfoPtr->attr.lastByte) {
	    cacheInfoPtr->attr.lastByte = offset + toWrite - 1;
	}
	blockSize = cacheInfoPtr->attr.lastByte + 1 -
		    (blockNum * FS_BLOCK_SIZE);
	if (blockSize > FS_BLOCK_SIZE) {
	    blockSize = FS_BLOCK_SIZE;
	}
	Fscache_UnlockBlock(blockPtr, (unsigned) modTime, blockAddr,
		blockSize, FSCACHE_CLEAR_READ_AHEAD |
		((flags&FS_WRITE_TO_DISK) ?
		    FSCACHE_WRITE_TO_DISK : 0));
    }

    *lenPtr = offset - oldOffset;
    Fs_StatAdd(offset - oldOffset, fs_Stats.blockCache.bytesWritten,
	       fs_Stats.blockCache.bytesWrittenOverflow);
#ifndef CLEAN
    if (fsdmKeepTypeInfo) {
	int fileType;

	fileType = Fsdm_FindFileType(cacheInfoPtr);
	fs_TypeStats.cacheBytes[FS_STAT_WRITE][fileType] += offset - oldOffset;
    }
    if (bytesToFree > 0) {
	Fsdm_RecordDeletionStats(cacheInfoPtr, bytesToFree);
    }
#endif CLEAN

    /*
     * Update the firstByte so that Fs_Read knows there is data.
     * This is support for named pipes, which care about firstByte.
     */
    if (cacheInfoPtr->attr.firstByte == -1 && *lenPtr > 0) {
	cacheInfoPtr->attr.firstByte = 0;
    }
    if (!(flags & FS_CLIENT_CACHE_WRITE) && *lenPtr > 0) {
	/*
	 * Update the modify time unless this write is a flush back from a
	 * user cache in which case we already get the correct mod time
	 * for the file when the client closes.
	 */
	cacheInfoPtr->attr.modifyTime = fsutil_TimeInSeconds;
    }
exit:
    if (status == FS_WOULD_BLOCK) {
	Fsutil_FastWaitListInsert(fscacheFullWaitList, remoteWaitPtr);
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_BlockRead --
 *
 *	Return a pointer to the cache block that contains a
 *	block of a file.  This is used to directly access cache blocks
 *	and so avoid the cost of a copy.  The number of valid bytes
 *	in the cache block is returned.
 *
 * Results:
 *	SUCCESS or error when reading file system block.  Upon failure,
 *	or a zero length read with the allocate flag not set, the cache block 
 *	is not locked down and the caller doesn't have to worry about it; 
 *	NIL is returned in *blockPtrPtr.
 *	This returns FS_WOULD_BLOCK if blockType includes FSCACHE_DONT_BLOCK
 *	and the cache is full.
 *
 * Side effects:
 *	The cache block is locked down, and its contents are filled in
 *	from disk if neccessary.  The caller has to unlock the block when done,
 *	if it is non-NIL.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fscache_BlockRead(cacheInfoPtr, blockNum, blockPtrPtr, numBytesPtr, blockType,
		 allocate)
    register	Fscache_FileInfo *cacheInfoPtr;	/* File to read block from.
						 * Should be locked on entry */
    int				blockNum;	/* Block to read. */
    register	Fscache_Block	**blockPtrPtr;	/* Where to put pointer to 
						 * block.*/
    register	int		*numBytesPtr;	/* Return number of bytes 
						 * read. */
    int				blockType;	/* One of FSCACHE_DATA_BLOCK
						 * and FSCACHE_DIR_BLOCK.
						 * | FSCACHE_DONT_BLOCK */
    Boolean			allocate;	/* TRUE => return the cache
						 * block even though there
						 * is not data in it. */
{
    Boolean		found;
    ReturnStatus	status = SUCCESS;
    int			offset;
    Fscache_Block	*blockPtr;

    LOCK_MONITOR;

    *blockPtrPtr = (Fscache_Block *)NIL;
    *numBytesPtr = 0;

   if (cacheInfoPtr->flags & FSCACHE_FILE_NOT_CACHEABLE) {
	panic( "Fscache_BlockRead, file not cacheable!\n");
	status = FS_NOT_CACHEABLE;
	goto exit;
    }
    offset = blockNum * FS_BLOCK_SIZE;
    if (offset > cacheInfoPtr->attr.lastByte) {
	status = SUCCESS;
	goto exit;
    }
    fs_Stats.blockCache.readAccesses++;
    if (blockType & FSCACHE_DIR_BLOCK) {
        fs_Stats.blockCache.dirBlockAccesses++;
    }
    Fscache_FetchBlock(cacheInfoPtr, blockNum, blockType,
			blockPtrPtr, &found);
    if (*blockPtrPtr == (Fscache_Block *)NIL) {
	status = FS_WOULD_BLOCK;
	goto exit;
    }
    blockPtr = *blockPtrPtr;

    if (!found) {
	/*
	 * Read in the cache block.  If the block isn't full (includes EOF)
	 * then blockSize is set and the rest of the cache block has
	 * been zero filled.
	 */
	status = (cacheInfoPtr->ioProcsPtr->blockRead)
		    (cacheInfoPtr->hdrPtr, blockPtr, (Sync_RemoteWaiter *) NIL);
#ifdef lint
	status = Fsio_FileBlockRead(cacheInfoPtr->hdrPtr,
		blockPtr, (Sync_RemoteWaiter *) NIL);
	status = FsrmtFileBlockRead(cacheInfoPtr->hdrPtr,
		blockPtr, (Sync_RemoteWaiter *) NIL);
#endif /* lint */
	if (status == SUCCESS && blockPtr->blockSize == 0 &&
	    offset < cacheInfoPtr->attr.lastByte + 1) {
	    /*
	     * Due to delayed writes the disk descriptor has no space allocated
	     * and the file block read routine thinks we've read past eof.
	     * Actually were in a hole in the file and should return zeros.
	     * The blockRead routine always zero fills for us.
	     */
	    printf("Fscache_BlockRead: Giving zeros to \"%s\" <%d,%d> block %d amount %d\n",
		    Fsutil_HandleName(cacheInfoPtr->hdrPtr),
		    cacheInfoPtr->hdrPtr->fileID.major,
		    cacheInfoPtr->hdrPtr->fileID.minor,
		    blockPtr->blockNum,
		    cacheInfoPtr->attr.lastByte - offset);
	    blockPtr->blockSize = cacheInfoPtr->attr.lastByte - offset;
	}
	if ((status != SUCCESS) ||
	    (blockPtr->blockSize == 0 && !allocate)) {
	    /*
	     * We hit a disk error or are really past the end-of-file.
	     */
	    Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
	    *blockPtrPtr = (Fscache_Block *)NIL;
	}
    } else {
	if (blockType & FSCACHE_DIR_BLOCK) {
	    fs_Stats.blockCache.dirBlockHits++;
	}
	if (blockPtr->flags & FSCACHE_READ_AHEAD_BLOCK) {
	    fs_Stats.blockCache.readAheadHits++;
	}
	if (blockPtr->timeDirtied != 0) {
	    fs_Stats.blockCache.readHitsOnDirtyBlock++;
	} else {
	    fs_Stats.blockCache.readHitsOnCleanBlock++;
	}
    }
    if (status == SUCCESS) {
	*numBytesPtr = blockPtr->blockSize;
    }

    if (blockType & FSCACHE_DIR_BLOCK) {
	fs_Stats.blockCache.dirBytesRead += blockPtr->blockSize;
    } else {
	Fs_StatAdd(blockPtr->blockSize, fs_Stats.blockCache.bytesRead,
		   fs_Stats.blockCache.bytesReadOverflow);
#ifndef CLEAN
	if (fsdmKeepTypeInfo) {
	    int fileType;

	    fileType = Fsdm_FindFileType(cacheInfoPtr);
	    fs_TypeStats.cacheBytes[FS_STAT_READ][fileType] += blockPtr->blockSize;
	}
#endif CLEAN
    }
    /*
     * Read ahead the next block.
     */
    FscacheReadAhead(cacheInfoPtr, blockNum + 1);
exit:
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_Trunc --
 *
 *	Truncate data out of the cache.  This knows how to truncate
 *	Consuming streams.  This is called as part of file deletion
 *	before the disk-resident descriptor and index blocks are
 *	modified.  It is possible for blocks to remain in the cache
 *	after this call for various reasons like they are locked
 *	due to I/O.  The Fscache_DeleteFile procedure can be called
 *	after the descriptor is re-written in order to really clean
 *	up the cache state associated with this file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data blocks are removed from the cache.  The file's first and
 *	last byte indexes get updated.  The modify time of the file
 *	gets set.
 *
 *----------------------------------------------------------------------
 */
void
Fscache_Trunc(cacheInfoPtr, length, flags)
    Fscache_FileInfo *cacheInfoPtr;
    int length;
    Boolean flags;	/* FSCACHE_TRUNC_CONSUME | FSCACHE_TRUNC_DELETE */
{
    int firstBlock;
    int lastBlock;
    int firstByte, lastByte;	/* For debugging */

    LOCK_MONITOR;

    if (flags & FSCACHE_TRUNC_DELETE) {
	cacheInfoPtr->flags |= FSCACHE_FILE_GONE;
    }
    if ((cacheInfoPtr->flags & FS_NOT_CACHEABLE) == 0) {
	if (flags & FSCACHE_TRUNC_CONSUME) {
	    /*
	     * Truncation on consuming streams is defined to be truncation
	     * from the front of the file, ie. the most recently written
	     * bytes (length of them) will be left in the stream
	     */
	    if (cacheInfoPtr->attr.lastByte >= 0 &&
		cacheInfoPtr->attr.firstByte < 0) {
		panic( "Fscache_Trunc, bad firstByte %d\n",
			cacheInfoPtr->attr.firstByte);
		cacheInfoPtr->attr.firstByte = 0;
	    }
	    if (cacheInfoPtr->attr.firstByte >= 0) {
		int newFirstByte = cacheInfoPtr->attr.lastByte - length + 1;
		lastBlock = newFirstByte / FS_BLOCK_SIZE;
		firstBlock = cacheInfoPtr->attr.firstByte / FS_BLOCK_SIZE;
		if (length == 0) {
		    Fscache_FileInvalidate(cacheInfoPtr, firstBlock, lastBlock);
		    newFirstByte = -1;
		    cacheInfoPtr->attr.lastByte = -1;
		} else if (lastBlock > firstBlock) {
		    Fscache_FileInvalidate(cacheInfoPtr, firstBlock,
					    lastBlock-1);
		}
		cacheInfoPtr->attr.firstByte = newFirstByte;
	    }
	} else if (length - 1 < cacheInfoPtr->attr.lastByte) {
	    /*
	     * Do file like truncation, leave length bytes at the
	     * beginning of the file.
	     */
	    if (length == 0) {
		firstBlock = 0;
	    } else {
		firstBlock = (length - 1) / FS_BLOCK_SIZE + 1;
	    }
	    lastBlock = cacheInfoPtr->attr.lastByte / FS_BLOCK_SIZE;
	    firstByte = cacheInfoPtr->attr.firstByte;
	    lastByte = cacheInfoPtr->attr.lastByte;
            if (length - 1 < cacheInfoPtr->attr.firstByte) {
                cacheInfoPtr->attr.lastByte = -1;
	        cacheInfoPtr->attr.firstByte = -1;
            } else {
                cacheInfoPtr->attr.lastByte = length - 1;
            }
	    Fscache_FileInvalidate(cacheInfoPtr, firstBlock, lastBlock);
	    if (firstBlock > 0) {
		Fscache_BlockTrunc(cacheInfoPtr, firstBlock - 1,
				  length - (firstBlock - 1) * FS_BLOCK_SIZE);
	    }
        }
	cacheInfoPtr->attr.modifyTime = fsutil_TimeInSeconds;
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fscache_DeleteFile --
 *
 *	Nuke the cache state associated with this file.  This is called
 *	from Fsio_FileTrunc after the file has been deleted from disk.
 *	The file handle will be deleted shortly, and it is crucial that
 *	this procedure be called in order to fully clean up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data blocks are removed from the cache.
 *
 *----------------------------------------------------------------------
 */
void
Fscache_DeleteFile(cacheInfoPtr)
    Fscache_FileInfo *cacheInfoPtr;
{
    int firstBlock;
    int lastBlock;
    int firstByte, lastByte;	/* For debugging */

    LOCK_MONITOR;

    if (cacheInfoPtr->blocksInCache > 0) {
	printf("Fscache_DeleteFile \"%s\" <%d,%d>: %d cache blocks left\n",
			Fsutil_HandleName(cacheInfoPtr->hdrPtr),
			cacheInfoPtr->hdrPtr->fileID.major,
			cacheInfoPtr->hdrPtr->fileID.minor,
			cacheInfoPtr->blocksInCache);
	/*
	 * Use this loop to recover.  Disk full
	 * conditions can cause file truncations to fail.
	 */
	while (!List_IsEmpty(&cacheInfoPtr->blockList)) {
	    register Fscache_Block *blockPtr;
	    register List_Links *listItem;
	    listItem = List_First(&cacheInfoPtr->blockList);
	    blockPtr = FILE_LINKS_TO_BLOCK(listItem);
	    printf("%d ", blockPtr->blockNum);
	    if (blockPtr->refCount > 0) {
		printf("ref %d! ", blockPtr->refCount);
	    }
	    Fscache_FileInvalidate(cacheInfoPtr, blockPtr->blockNum,
		    blockPtr->blockNum);
	}
	printf("\n");
    }
    if (!List_IsEmpty(&cacheInfoPtr->dirtyList)) {
	register Fscache_Block *blockPtr;
	register List_Links *listItem;
	printf("Fscache_DeleteFile \"%s\" <%d,%d>: dirty blocks left in cache\n",
	    Fsutil_HandleName(cacheInfoPtr->hdrPtr),
	    cacheInfoPtr->hdrPtr->fileID.major,
	    cacheInfoPtr->hdrPtr->fileID.minor);
	listItem = List_First(&cacheInfoPtr->dirtyList);
	while (!List_IsAtEnd(&cacheInfoPtr->dirtyList, listItem)) {
	    blockPtr = DIRTY_LINKS_TO_BLOCK(listItem);
	    printf("%d ", blockPtr->blockNum);
	    if (blockPtr->refCount > 0) {
		printf("ref %d! ", blockPtr->refCount);
	    }
	    listItem = listItem->nextPtr;
	    Fscache_FileInvalidate(cacheInfoPtr, blockPtr->blockNum,
		    blockPtr->blockNum);
	}
	printf("\n");
    }
    /*
     * At this point the file should have no cache blocks associated
     * with it, clean or dirty, and the file itself should not be
     * on the dirty list or being written out.
     */
    if ((cacheInfoPtr->blocksInCache > 0) ||
	(cacheInfoPtr->flags & (FSCACHE_FILE_ON_DIRTY_LIST|
				FSCACHE_FILE_BEING_WRITTEN))) {
	panic("Fscache_DeleteFile failed \"%s\" blocks %d flags %x\n",
		Fsutil_HandleName(cacheInfoPtr->hdrPtr),
		cacheInfoPtr->blocksInCache,
		cacheInfoPtr->flags);
    }
    UNLOCK_MONITOR;
}
