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
#include "fsInt.h"
#include "fsCacheOps.h"
#include "fsConsist.h"
#include "fsBlockCache.h"
#include "fsOpTable.h"
#include "fsDisk.h"
#include "fsStat.h"
#include "fsTrace.h"
#include "fsLocalDomain.h"
#include "fsReadAhead.h"
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
 * FsCacheUpdate --
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
FsCacheUpdate(cacheInfoPtr, openForWriting, version, cacheable, attrPtr)
    FsCacheFileInfo *cacheInfoPtr;	/* Cache state of file to update. */
    Boolean		openForWriting;	/* TRUE if opening for writing */
    int			version;	/* Version number used to verify
					 * our cached data blocks. */
    Boolean		cacheable;	/* TRUE if server says we can cache. */
    FsCachedAttributes	*attrPtr;	/* Attributes from server */
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
	printf("FsCacheUpdate: <%d,%d> version %d->%d, %s, %s\n",
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
    if (cacheInfoPtr->flags & FS_FILE_GONE) {
	/*
	 * Reset the deleted bit.  By this time any deletion
	 * actions will have completed.
	 */
	cacheInfoPtr->flags &= ~FS_FILE_GONE;
    }

    /*
     * If we were caching and the file is no longer cacheable, or our copy
     * is outOfDate, we need to invalidate our cache.  Note that we do
     * the cache invalidate with our previous notion of the size.
     */
    if (!cacheable || outOfDate) {
	fsStats.handle.cacheFlushes++;
	FsCacheFileInvalidate(cacheInfoPtr, 0, FS_LAST_BLOCK);
    }
    if (outOfDate) {
	fsStats.handle.versionMismatch++;
	cacheInfoPtr->attr = *attrPtr;
    }

    /*
     * Propogate the cachability of the handle.
     */
    if (cacheable) {
	if (cacheInfoPtr->flags & FS_FILE_NOT_CACHEABLE) {
	    /*
	     * The handle wasn't client cacheable before.  In this
	     * case mark the handle as cacheable and update the cached
	     * attributes since they could have changed 
	     * while we didn't have the file cached.
	     */
	    cacheInfoPtr->attr = *attrPtr;
	    cacheInfoPtr->flags &= ~FS_FILE_NOT_CACHEABLE;
	}
    } else {
	cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
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
 * FsUpdateAttrFromClient --
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
FsUpdateAttrFromClient(clientID, cacheInfoPtr, attrPtr)
    int clientID;				/* Client, for warning msg */
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state to update. */
    register FsCachedAttributes	*attrPtr;	/* Attributes from client */
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
	"FsUpdateAttrFromClient %d: \"%s\" <%d,%d> short size %d not %d\n",
		clientID,
		FsHandleName(cacheInfoPtr->hdrPtr),
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
 * FsUpdateDirSize --
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
FsUpdateDirSize(cacheInfoPtr, newLastByte)
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state to update. */
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
 * FsUpdateAttrFromCache --
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
FsUpdateAttrFromCache(cacheInfoPtr, attrPtr)
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state to check. */
    register Fs_Attributes	*attrPtr;	/* Attributes from server */
{
    LOCK_MONITOR;
    if ((cacheInfoPtr->version == attrPtr->version) &&
	(cacheInfoPtr->flags & FS_FILE_NOT_CACHEABLE) == 0) {
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
 * FsGetCachedAttr --
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
FsGetCachedAttr(cacheInfoPtr, versionPtr, attrPtr)
    register FsCacheFileInfo	*cacheInfoPtr;	/* Cache state to update. */
    register int		*versionPtr;	/* Version number of file */
    register FsCachedAttributes	*attrPtr;	/* New attributes from server */
{
    LOCK_MONITOR;
    *versionPtr = cacheInfoPtr->version;
    *attrPtr = cacheInfoPtr->attr;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsUpdateCachedAttr --
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
FsUpdateCachedAttr(cacheInfoPtr, attrPtr, flags)
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state to update. */
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
 * FsCacheCheckVersion --
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
FsCacheCheckVersion(cacheInfoPtr, version, clientID)
    register FsCacheFileInfo	*cacheInfoPtr;	/* Cache state to check. */
    int				version;
    int				clientID;
{
    ReturnStatus status = SUCCESS;
    LOCK_MONITOR;
    if (version != cacheInfoPtr->version) {
	printf(
    "Version mismatch: clt %d, srv %d, file \"%s\" <%d,%d>, from client %d\n",
	    version, cacheInfoPtr->version, FsHandleName(cacheInfoPtr->hdrPtr),
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
 * FsCacheOkToScavenge --
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
FsCacheOkToScavenge(cacheInfoPtr)
    register FsCacheFileInfo	*cacheInfoPtr;	/* Cache state to check. */
{
    register Boolean ok;
    LOCK_MONITOR;
    ok = FsBlockCacheOkToScavenge(cacheInfoPtr);
    UNLOCK_MONITOR;
    return(ok);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsCacheConsist --
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
FsCacheConsist(cacheInfoPtr, flags, cachedAttrPtr)
    register FsCacheFileInfo	*cacheInfoPtr;
    int				flags;
    FsCachedAttributes		*cachedAttrPtr;
{
    ReturnStatus status;
    int firstBlock;
    int numSkipped;

    LOCK_MONITOR;

    if (cacheInfoPtr->attr.firstByte == -1) {
	firstBlock = 0;
    } else {
	firstBlock = cacheInfoPtr->attr.firstByte / FS_BLOCK_SIZE;
    }
    status = SUCCESS;
    switch (flags & ~FS_DEBUG_CONSIST) {
	case FS_WRITE_BACK_BLOCKS:
	    status = FsCacheFileWriteBack(cacheInfoPtr, firstBlock,
			FS_LAST_BLOCK, FS_FILE_WB_WAIT, &numSkipped);
	    break;
	case FS_CANT_READ_CACHE_PIPE:
	case FS_INVALIDATE_BLOCKS:
	    FsCacheFileInvalidate(cacheInfoPtr, firstBlock, FS_LAST_BLOCK);
	    cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
	    break;
	case FS_INVALIDATE_BLOCKS | FS_WRITE_BACK_BLOCKS:
	    status = FsCacheFileWriteBack(cacheInfoPtr, firstBlock,
			FS_LAST_BLOCK, FS_FILE_WB_INVALIDATE | FS_FILE_WB_WAIT,
			&numSkipped);
	    cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
	    break;
	case FS_DELETE_FILE:
	    cacheInfoPtr->flags |= FS_FILE_GONE;
	    FsCacheFileInvalidate(cacheInfoPtr, firstBlock, FS_LAST_BLOCK);
	    cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
	    break;
	case FS_WRITE_BACK_ATTRS:
	    break;
	default:
	    printf( 
		      "FsCacheConsist: Bad consistency action %x\n", flags);
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
 * FsCacheRead --
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
FsCacheRead(cacheInfoPtr, flags, buffer, offset, lenPtr, remoteWaitPtr)
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state for file. */
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
    FsCacheBlock	*blockPtr;	/* For fetching blocks from cache */
    int			amountRead;	/* Amount that was read each time */
    int			streamType;	/* Stream type from handle header */

    /*
     * Serialiaze access to the cache for this file.
     */
    LOCK_MONITOR;

    if (cacheInfoPtr->flags & FS_FILE_NOT_CACHEABLE) {
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
    streamType = cacheInfoPtr->hdrPtr->fileID.type;

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
	FsReadAhead(cacheInfoPtr, blockNum + 1);

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
	FsCacheFetchBlock(cacheInfoPtr, blockNum, FS_DATA_CACHE_BLOCK,
			  &blockPtr, &found);
	fsStats.blockCache.readAccesses++;
	if (found) {
	    amountRead = 0;
	    if (blockPtr->timeDirtied != 0) {
		fsStats.blockCache.readHitsOnDirtyBlock++;
	    } else {
		fsStats.blockCache.readHitsOnCleanBlock++;
	    }
	    if (blockPtr->flags & FS_READ_AHEAD_BLOCK) {
		fsStats.blockCache.readAheadHits++;
	    }
	} else {
	    /*
	     * We didn't find the block in the cache so we have to read it in.
	     */
	    int blockOffset = blockNum * FS_BLOCK_SIZE;
	    amountRead = FS_BLOCK_SIZE;
	    status = (*fsStreamOpTable[streamType].blockRead)
			(cacheInfoPtr->hdrPtr, 0, blockPtr->blockAddr,
				&blockOffset, &amountRead, remoteWaitPtr);
	    if (status != SUCCESS) {
		fsStats.blockCache.domainReadFails++;
		FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
		break;
	    } else if (amountRead < FS_BLOCK_SIZE) {
                /*
                 * We always must make sure that every cache block is filled
		 * with zeroes so reads in holes or just past eof do not
		 * return garbage.
                 */
		fsStats.blockCache.readZeroFills++;
                bzero(blockPtr->blockAddr + amountRead, FS_BLOCK_SIZE - amountRead);
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
		FsCacheUnlockBlock(blockPtr, 0, -1, amountRead, FS_CLEAR_READ_AHEAD);
		break;
	    }
	} else {
	    bcopy(blockPtr->blockAddr + (offset & FS_BLOCK_OFFSET_MASK),
		      buffer, toRead);
	}
	FsCacheUnlockBlock(blockPtr, 0, -1, amountRead, FS_CLEAR_READ_AHEAD);
    }
    *lenPtr -= size;
    FsStat_Add(*lenPtr, fsStats.blockCache.bytesRead,
	       fsStats.blockCache.bytesReadOverflow);
#ifndef CLEAN
    if (fsKeepTypeInfo) {
	int fileType;

	fileType = FsFindFileType(cacheInfoPtr);
	fsTypeStats.cacheBytes[FS_STAT_READ][fileType] += *lenPtr;
    }
#endif CLEAN

    /*
     * Consume data if the flags indicate a consuming stream.
     * This doesn't update the modify time of the file.
     */
    if (flags & FS_CONSUME) {
	int length;
	length = cacheInfoPtr->attr.lastByte - (int) offset + 1;
	(void)(*fsStreamOpTable[streamType].ioControl)
		(cacheInfoPtr->hdrPtr, IOC_TRUNCATE, mach_Format,
			sizeof(length), (Address) &length, 0, (Address) NIL); 
    }
exit:
    if (status == SUCCESS) {
	cacheInfoPtr->attr.accessTime = fsTimeInSeconds;
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsCacheWrite --
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
FsCacheWrite(cacheInfoPtr, flags, buffer, offset, lenPtr, remoteWaitPtr)
    register FsCacheFileInfo *cacheInfoPtr;	/* Cache state for the file. */
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
    FsCacheBlock 	*blockPtr;	/* From fetching cached blocks */
    int			blockSize;	/* The number of bytes in the current
					 * block. */
    ReturnStatus 	status = SUCCESS; /* Return from I/O operations */
    int			firstBlock;	/* The first block of the write */
    int			lastBlock;	/* The last block to write */
    Boolean		found;		/* From fetching cached blocks */
    int			numBytes;	/* Amount actually written each time */
    int			oldOffset;	/* Initial value of the offset */
    int			lastFileBlock;	/* Last block in the file */
    int			blockAddr;	/* For allocating blocks */
    Boolean		newBlock;	/* A brand new block was allocated. */
    int			streamType;	/* Type from handle header */
    int			bytesToFree; 	/* number of bytes overwritten
					 * in file */
    int			modTime;	/* File modify time. */

    /*
     * Serialize access to the cache for this file.
     */
    LOCK_MONITOR;

    if (cacheInfoPtr->flags & FS_FILE_NOT_CACHEABLE) {
	/*
	 * Not cached.  The flag is checked here under monitor lock.
	 */
	status = FS_NOT_CACHEABLE;
	goto exit;
    }
    if (cacheInfoPtr->flags & FS_FILE_GONE) {
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
    streamType = cacheInfoPtr->hdrPtr->fileID.type;

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
	 * Allocate space for the new data on disk.
	 */
	status = (*fsStreamOpTable[streamType].allocate)(cacheInfoPtr->hdrPtr,
			offset, toAlloc, &blockAddr, &newBlock);
	if (blockAddr == FS_NIL_INDEX) {
	    /*
	     * HACK to limit the number of "Alloc failed" messages.
	     * MAX_MESSAGES - the max number of messages within the interval
	     * INTERVAL_LIMIT - the time in which MAX_MESSAGES can appear.
	     * The idea is that less than MAX_MESSAGES can appear within
	     * any INTERVAL_LIMIT period.
	     */
	    static int lastPrintTime = 0;
	    static int numRecentPrints = 0;
#define MAX_MESSAGES	2
#define INTERVAL_LIMIT	30

	    if (fsTimeInSeconds - lastPrintTime >= INTERVAL_LIMIT) {
		numRecentPrints = 0;
	    }
	    if (numRecentPrints < MAX_MESSAGES) {
		numRecentPrints++;
		printf("Fs_Write: Alloc failed <%d,%d> \"%s\"\n",
		    cacheInfoPtr->hdrPtr->fileID.major,
		    cacheInfoPtr->hdrPtr->fileID.major,
		    FsHandleName(cacheInfoPtr->hdrPtr));
		lastPrintTime = fsTimeInSeconds;
	    }
#undef MAX_MESSAGES
#undef INTERVAL_LIMIT

	    if (status == SUCCESS) {
		status = FS_NO_DISK_SPACE;
	    }
	    break;
	}

	fsStats.blockCache.writeAccesses++;
	FsCacheFetchBlock(cacheInfoPtr, blockNum, 
			  (int)(FS_IO_IN_PROGRESS | FS_DATA_CACHE_BLOCK), 
			  &blockPtr, &found);
	if (toWrite == FS_BLOCK_SIZE) {
	    if (found) {
		fsStats.blockCache.overWrites++;
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
		    fsStats.blockCache.partialWriteHits++;
		    if (blockPtr->flags & FS_READ_AHEAD_BLOCK) {
			fsStats.blockCache.readAheadHits++;
		    }
		    bytesToFree += toWrite;
		} else {
		    int blockOffset = blockNum * FS_BLOCK_SIZE;

		    numBytes = FS_BLOCK_SIZE;
		    fsStats.blockCache.partialWriteMisses++;
		    status = (*fsStreamOpTable[streamType].blockRead)
			(cacheInfoPtr->hdrPtr, 0, blockPtr->blockAddr,
				&blockOffset, &numBytes, remoteWaitPtr);
		    if (status != SUCCESS) {
			fsStats.blockCache.domainReadFails++;
			FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
			break;
		    }  else if (numBytes < FS_BLOCK_SIZE) {
			/*
			 * We always want cache blocks to be zero filled.
			 */
			fsStats.blockCache.writeZeroFills1++;
			bzero(blockPtr->blockAddr + numBytes, FS_BLOCK_SIZE - numBytes);
		    }
		    bytesToFree += numBytes;
		}
	    } else {
		/*
		 * We are writing to the end of the file or the block
		 * that we are writing to is brand new.  In this case zero
		 * fill the block if it isn't in the cache yet.
		 */
		if (!found) {
		    fsStats.blockCache.writeZeroFills2++;
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
		FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
		break;
	    }
	} else {
	    bcopy(buffer, blockPtr->blockAddr + (offset & FS_BLOCK_OFFSET_MASK), toWrite);
	}

	/*
	 * If the block is write-thru then write the data through to the 
	 * server and then check the block back in as clean.
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

	    dummyStream.ioHandlePtr = cacheInfoPtr->hdrPtr;
	    status = FsRemoteWrite(&dummyStream, &io,
			 (Sync_RemoteWaiter *)NIL, &reply);
	    if (status != SUCCESS) {
		FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
		break;
	    }
	    modTime = 0;
	} else {
	    modTime = fsTimeInSeconds;
	}

	/*
	 * Return the block to the cache.
	 */
	if (offset + toWrite - 1 > cacheInfoPtr->attr.lastByte) {
	    cacheInfoPtr->attr.lastByte = offset + toWrite - 1;
	}
	blockSize = cacheInfoPtr->attr.lastByte + 1 -
		    (blockNum * FS_BLOCK_SIZE);
	if (blockSize > FS_BLOCK_SIZE) {
	    blockSize = FS_BLOCK_SIZE;
	}
	FsCacheUnlockBlock(blockPtr, (unsigned) modTime, blockAddr,
				blockSize, FS_CLEAR_READ_AHEAD);
    }

    *lenPtr = offset - oldOffset;
    FsStat_Add(offset - oldOffset, fsStats.blockCache.bytesWritten,
	       fsStats.blockCache.bytesWrittenOverflow);
#ifndef CLEAN
    if (fsKeepTypeInfo) {
	int fileType;

	fileType = FsFindFileType(cacheInfoPtr);
	fsTypeStats.cacheBytes[FS_STAT_WRITE][fileType] += offset - oldOffset;
    }
    if (bytesToFree > 0) {
	FsRecordDeletionStats(cacheInfoPtr, bytesToFree);
    }
#endif CLEAN

    /*
     * Update the firstByte so that Fs_Read knows there is data.
     */
    if (cacheInfoPtr->attr.firstByte == -1 && *lenPtr > 0) {
	cacheInfoPtr->attr.firstByte = 0;
    }
    if (!(flags & FS_CLIENT_CACHE_WRITE)) {
	/*
	 * Update the modify time unless this write is a flush back from a
	 * user cache in which case we already get the correct mod time
	 * for the file when the client closes.
	 */
	cacheInfoPtr->attr.modifyTime = fsTimeInSeconds;
    }
exit:
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsCacheBlockRead --
 *
 *	Return a pointer to the cache block that contains a
 *	block of a file.  This is used directly access cache blocks
 *	and so avoid the cost of a copy.  The number of valid bytes
 *	in the cache block is returned.
 *
 * Results:
 *	SUCCESS or error when reading file system block.  Upon failure,
 *	or a zero length read with the allocate flag not set, the cache block 
 *	is not locked down and the caller doesn't have to worry about it; 
 *	NIL is returned in *blockPtrPtr.
 *
 * Side effects:
 *	The cache block is locked down, and its contents are filled in
 *	from disk if neccessary.  The caller has to unlock the block when done,
 *	if it is non-NIL.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsCacheBlockRead(cacheInfoPtr, blockNum, blockPtrPtr, numBytesPtr, blockType,
		 allocate)
    register	FsCacheFileInfo *cacheInfoPtr;	/* File to read block from.
						 * Should be locked on entry */
    int				blockNum;	/* Block to read. */
    register	FsCacheBlock	**blockPtrPtr;	/* Where to put pointer to 
						 * block.*/
    register	int		*numBytesPtr;	/* Return number of bytes 
						 * read. */
    int				blockType;	/* One of FS_DATA_CACHE_BLOCK
						 * and FS_DIR_CACHE_BLOCK. */
    Boolean			allocate;	/* TRUE => return the cache
						 * block even though there
						 * is not data in it. */
{
    Boolean		found;
    ReturnStatus	status = SUCCESS;
    int			offset;

    LOCK_MONITOR;

    *blockPtrPtr = (FsCacheBlock *)NIL;
    if (cacheInfoPtr->flags & FS_FILE_NOT_CACHEABLE) {
	panic( "FsCacheBlockRead, file not cacheable!\n");
	status = FS_NOT_CACHEABLE;
	goto exit;
    }
    offset = blockNum * FS_BLOCK_SIZE;
    if (offset > cacheInfoPtr->attr.lastByte) {
	*numBytesPtr = 0;
	status =SUCCESS;
	goto exit;
    }
    fsStats.blockCache.readAccesses++;
    if (blockType == FS_DIR_CACHE_BLOCK) {
        fsStats.blockCache.dirBlockAccesses++;
    }
    FsCacheFetchBlock(cacheInfoPtr, blockNum, blockType, blockPtrPtr, &found);

    if (!found) {
	/*
	 * Set up the stream needed to get the data if its not in the cache.
	 */
	*numBytesPtr = FS_BLOCK_SIZE;
	status = (*fsStreamOpTable[cacheInfoPtr->hdrPtr->fileID.type].blockRead)
		    (cacheInfoPtr->hdrPtr, 0, (*blockPtrPtr)->blockAddr,
		     &offset, numBytesPtr, (Sync_RemoteWaiter *) NIL);
	if (status == SUCCESS && *numBytesPtr == 0 &&
	    offset < cacheInfoPtr->attr.lastByte + 1) {
	    /*
	     * Due to delayed writes the disk descriptor has no space allocated
	     * and the file block read routine thinks we've read past eof.
	     * Actually were in a hole in the file and should return zeros.
	     */
	    bzero((*blockPtrPtr)->blockAddr, FS_BLOCK_SIZE);
	    *numBytesPtr = FS_BLOCK_SIZE;
	}
	if (status != SUCCESS || (*numBytesPtr == 0 && !allocate)) {
	    /*
	     * We hit a disk error or are really past the end-of-file.
	     * Note that if allocate is TRUE well return a cache block
	     * to our caller with *numBytesPtr == 0.
	     */
	    FsCacheUnlockBlock(*blockPtrPtr, 0, -1, 0, FS_DELETE_BLOCK);
	    *blockPtrPtr = (FsCacheBlock *)NIL;
	    *numBytesPtr = 0;
	}
	if ((*numBytesPtr > 0) && (*numBytesPtr < FS_BLOCK_SIZE) && 
	    (blockType & FS_DATA_CACHE_BLOCK)) {
	    /*
	     * If did not read a full data block then zero fill the rest
	     * because we can't have any partial data blocks in the cache.
	     */
	    fsStats.blockCache.readZeroFills++;
	    bzero((*blockPtrPtr)->blockAddr + *numBytesPtr, FS_BLOCK_SIZE - *numBytesPtr);
	}
    } else {
	if (blockType == FS_DIR_CACHE_BLOCK) {
	    fsStats.blockCache.dirBlockHits++;
	}
	if ((*blockPtrPtr)->flags & FS_READ_AHEAD_BLOCK) {
	    fsStats.blockCache.readAheadHits++;
	}
	if ((*blockPtrPtr)->timeDirtied != 0) {
	    fsStats.blockCache.readHitsOnDirtyBlock++;
	} else {
	    fsStats.blockCache.readHitsOnCleanBlock++;
	}

	if (cacheInfoPtr->attr.lastByte > offset + FS_BLOCK_SIZE) {
	    *numBytesPtr = FS_BLOCK_SIZE;
	} else {
	    *numBytesPtr = cacheInfoPtr->attr.lastByte - offset + 1;
	}
    }
    if (blockType == FS_DIR_CACHE_BLOCK) {
	fsStats.blockCache.dirBytesRead += *numBytesPtr;
    } else {
	FsStat_Add(*numBytesPtr, fsStats.blockCache.bytesRead,
		   fsStats.blockCache.bytesReadOverflow);
#ifndef CLEAN
	if (fsKeepTypeInfo) {
	    int fileType;

	    fileType = FsFindFileType(cacheInfoPtr);
	    fsTypeStats.cacheBytes[FS_STAT_READ][fileType] += *numBytesPtr;
	}
#endif CLEAN
    }
    /*
     * Read ahead the next block.
     */
    FsReadAhead(cacheInfoPtr, blockNum + 1);
exit:
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsCacheTrunc --
 *
 *	Truncate data out of the cache.  This knows how to truncate
 *	Consuming streams.
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
FsCacheTrunc(cacheInfoPtr, length, flags)
    FsCacheFileInfo *cacheInfoPtr;
    int length;
    Boolean flags;	/* FS_TRUNC_CONSUME | FS_TRUNC_DELETE */
{
    int firstBlock;
    int lastBlock;
    int firstByte, lastByte;	/* For debugging */

    LOCK_MONITOR;

    if (flags & FS_TRUNC_DELETE) {
	cacheInfoPtr->flags |= FS_FILE_GONE;
    }
    if ((cacheInfoPtr->flags & FS_NOT_CACHEABLE) == 0) {
	if (flags & FS_TRUNC_CONSUME) {
	    /*
	     * Truncation on consuming streams is defined to be truncation
	     * from the front of the file, ie. the most recently written
	     * bytes (length of them) will be left in the stream
	     */
	    if (cacheInfoPtr->attr.lastByte >= 0 &&
		cacheInfoPtr->attr.firstByte < 0) {
		panic( "FsCacheTrunc, bad firstByte %d\n",
			cacheInfoPtr->attr.firstByte);
		cacheInfoPtr->attr.firstByte = 0;
	    }
	    if (cacheInfoPtr->attr.firstByte >= 0) {
		int newFirstByte = cacheInfoPtr->attr.lastByte - length + 1;
		lastBlock = newFirstByte / FS_BLOCK_SIZE;
		firstBlock = cacheInfoPtr->attr.firstByte / FS_BLOCK_SIZE;
		if (length == 0) {
		    FsCacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock);
		    newFirstByte = -1;
		    cacheInfoPtr->attr.lastByte = -1;
		} else if (lastBlock > firstBlock) {
		    FsCacheFileInvalidate(cacheInfoPtr, firstBlock,
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
	    FsCacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock);
	    if (firstBlock > 0) {
		FsCacheBlockTrunc(cacheInfoPtr, firstBlock - 1,
				  length - (firstBlock - 1) * FS_BLOCK_SIZE);
	    }
	    if (flags & FS_TRUNC_DELETE) {
		if (!List_IsEmpty(&cacheInfoPtr->blockList)) {
		    printf("FirstByte %d LastByte %d\n", firstByte, lastByte);
		    printf("File \"%s\" <%d,%d>: %d cache blocks left after delete blocks %d->%d\n",
			FsHandleName(cacheInfoPtr->hdrPtr),
			cacheInfoPtr->hdrPtr->fileID.major,
			cacheInfoPtr->hdrPtr->fileID.minor,
			cacheInfoPtr->blocksInCache, firstBlock, lastBlock);
		    /*
		     * Use this loop to recover.  Apparently disk full
		     * conditions can cause file truncations to fail.
		     */
		    while (!List_IsEmpty(&cacheInfoPtr->blockList)) {
			register FsCacheBlock *blockPtr;
			register List_Links *listItem;
			listItem = List_First(&cacheInfoPtr->blockList);
			blockPtr = FILE_LINKS_TO_BLOCK(listItem);
			printf("%d ", blockPtr->blockNum);
			if (blockPtr->refCount > 0) {
			    printf("ref %d! ", blockPtr->refCount);
			}
			if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
			    panic("Mistake in block list code\n");
			}
			FsCacheFileInvalidate(cacheInfoPtr, blockPtr->blockNum,
				blockPtr->blockNum);
		    }
		    printf("\n");
		}
		if (!List_IsEmpty(&cacheInfoPtr->dirtyList)) {
		    register FsCacheBlock *blockPtr;
		    register List_Links *listItem;
		    printf("File \"%s\" <%d,%d>: dirty blocks left in cache\n",
			FsHandleName(cacheInfoPtr->hdrPtr),
			cacheInfoPtr->hdrPtr->fileID.major,
			cacheInfoPtr->hdrPtr->fileID.minor);
		    printf("FirstByte %d LastByte %d Blocks:",
			firstByte, lastByte);
		    listItem = List_First(&cacheInfoPtr->dirtyList);
		    while (!List_IsAtEnd(&cacheInfoPtr->dirtyList, listItem)) {
			blockPtr = DIRTY_LINKS_TO_BLOCK(listItem);
			printf("%d ", blockPtr->blockNum);
			if (blockPtr->refCount > 0) {
			    printf("ref %d! ", blockPtr->refCount);
			}
			if (blockPtr->fileNum != cacheInfoPtr->hdrPtr->fileID.minor) {
			    panic("Mistake in block list code\n");
			}
			listItem = listItem->nextPtr;
#ifdef notdef
			/*
			 * Don't want to invalidate these as they are
			 * probably indirect blocks that will be needed
			 * shortly by FsDescTrunc.
			 */
			FsCacheFileInvalidate(cacheInfoPtr, blockPtr->blockNum,
				blockPtr->blockNum);
#endif notdef
		    }
		    printf("\n");
		}
	    }
        }
	cacheInfoPtr->attr.modifyTime = fsTimeInSeconds;
    }
    UNLOCK_MONITOR;
}
