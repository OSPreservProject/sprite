/* 
 * lfsDesc.c --
 *
 *	File descriptor management routines for LFS. The routine in 
 *	this module provide the interface for file descriptors I/O 
 *      and allocation for a LFS file system.   The implementation uses
 *	the file system block cache to cache groups of descriptors and
 *	provide write buffering. 
 *
 * Copyright 1989 Regents of the University of California
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
#endif /* not lint */

#include <sprite.h>
#include <lfs.h>
#include <lfsInt.h>
#include <lfsDesc.h>
#include <lfsDescMap.h>
#include <fs.h>
#include <fsdm.h>


/*
 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 * Start of routines exported to higher levels of the file system.
 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 */


/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileDescFetch() --
 *
 *	Fetch the specified file descriptor from the file system and 
 *	store it in *fileDescPtr.
 *
 * Results:
 *	An error if could not read the file descriptor from disk or is not
 *	allocated.
 *
 * Side effects:
 *	A block of descriptors may be read into the cache.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_FileDescFetch(domainPtr, fileNumber, fileDescPtr)
     Fsdm_Domain 	*domainPtr;	/* Domain to fetch file descriptor. */
     int		fileNumber; 	/* Number of file descriptor to fetch.*/
     Fsdm_FileDescriptor *fileDescPtr;	/* File descriptor structure to fetch.*/
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    LfsFileDescriptor	*descPtr;
    Fscache_Block	    *blockPtr;
    ReturnStatus	status;
    LfsDiskAddr	       diskAddr;
    int		i, cacheFlags;
    Boolean	found;

    LFS_STATS_INC(lfsPtr->stats.desc.fetches);
    status = LfsDescMapGetDiskAddr(lfsPtr, fileNumber, &diskAddr);
    if (status != SUCCESS) {
	return status;
    }
    cacheFlags = FSCACHE_DESC_BLOCK;
    if (LfsIsCleanerProcess(lfsPtr)) {
	cacheFlags |= FSCACHE_CANT_BLOCK;
    }
    /*
     * See if the value is in the descriptor block cache. If it is not 
     * then read it in.
     */
    Fscache_FetchBlock(&lfsPtr->descCache.handle.cacheInfo, 
		      LfsDiskAddrToOffset(diskAddr), cacheFlags,
		       &blockPtr, &found);
    if (!found) {
	LFS_STATS_INC(lfsPtr->stats.desc.fetchCacheMiss);
	status = LfsReadBytes(lfsPtr, diskAddr, 
		lfsPtr->fileLayout.params.descPerBlock * sizeof(*descPtr),
		blockPtr->blockAddr);
#ifdef ERROR_CHECK
	 LfsCheckRead(lfsPtr, diskAddr, 
		lfsPtr->fileLayout.params.descPerBlock * sizeof(*descPtr));
#endif
	if (status != SUCCESS) {
	    printf( "Could not read in file descriptor\n");
	    Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
	    return status;
	}
    }
    descPtr = (LfsFileDescriptor *) (blockPtr->blockAddr);
    for (i = 0; i < lfsPtr->fileLayout.params.descPerBlock; i++) {
	if (!(descPtr->common.flags & FSDM_FD_ALLOC)) {
	    break;
	}
	if (descPtr->fileNumber == fileNumber) {
	     LFS_STATS_INC(lfsPtr->stats.desc.goodFetch);
	     LFS_STATS_ADD(lfsPtr->stats.desc.fetchSearched,i);
	     bcopy((char *) &(descPtr->common), (char *)fileDescPtr,
			sizeof(descPtr->common));
	     status = LfsDescMapGetAccessTime(lfsPtr, fileNumber,
			    &(fileDescPtr->accessTime));
	     if (status != SUCCESS) {
		  LfsError(lfsPtr, status, "Can't get access time.\n");
	     }
	    Fscache_UnlockBlock(blockPtr, 0, -1, FS_BLOCK_SIZE, 0);
	    return SUCCESS;
	}
	descPtr++;
    }
    Fscache_UnlockBlock(blockPtr, 0, -1, 0, FSCACHE_DELETE_BLOCK);
    panic("Descriptor map foulup, can't find file %d at %d\n", fileNumber,
			LfsDiskAddrToOffset(diskAddr));
    return FS_FILE_NOT_FOUND;

}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileDescStore() --
 *	Store the given file descriptor back into the file system block
 *	where it came from.  
 *
 * Results:
 *	An error if could not read the file descriptor from disk or is not
 *	allocated.
 *
 * Side effects:
 *	A block of descriptors may be read into the cache.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_FileDescStore(domainPtr, handlePtr, fileNumber, fileDescPtr, forceOut)
    register Fsdm_Domain *domainPtr;	/* Domain to store the file 
					 * descriptor into. */
    Fsio_FileIOHandle	*handlePtr;
    int			fileNumber; 	/* Number of file descriptor to 
					   store.*/
    Fsdm_FileDescriptor	*fileDescPtr;	 /* File descriptor to store. */
    Boolean		forceOut;  /* Force the change to disk. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    ReturnStatus	status = SUCCESS;

    LFS_STATS_INC(lfsPtr->stats.desc.stores);
    if (fileDescPtr->flags & FSDM_FD_FREE) {
	LFS_STATS_INC(lfsPtr->stats.desc.freeStores);
	return SUCCESS;
    }
    if (fileDescPtr->flags & FSDM_FD_ACCESSTIME_DIRTY) {
	 status = LfsDescMapSetAccessTime(lfsPtr, fileNumber, 
			     fileDescPtr->accessTime);
	  if (status != SUCCESS) {
		  LfsError(lfsPtr, status, "Can't update descriptor map.\n");
          }
	  fileDescPtr->flags &= ~FSDM_FD_ACCESSTIME_DIRTY;
	  LFS_STATS_INC(lfsPtr->stats.desc.accessTimeUpdate);
    }
    if (fileDescPtr->flags & FSDM_FD_DIRTY) {
	LFS_STATS_INC(lfsPtr->stats.desc.dirtyList);
	Fscache_PutFileOnDirtyList(&handlePtr->cacheInfo, 
				FSCACHE_FILE_DESC_DIRTY);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileTrunc --
 *
 *      Shorten a file to length bytes.  This updates the descriptor
 *      and may free blocks and indirect blocks from the end of the file.
 *
 * Results:
 *      Error if had problem with indirect blocks, otherwise SUCCESS.
 *
 * Side effects:
 *      May modify the truncateVersion number.
 *	Any allocated blocks after the given size are deleted.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_FileTrunc(domainPtr, handlePtr, size, delete)
    Fsdm_Domain		*domainPtr;
    Fsio_FileIOHandle   *handlePtr;     /* File to truncate. */
    int                 size;           /* Size to truncate the file to. */
    Boolean		delete;		/* TRUE if Truncate for delete. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    Fsdm_FileDescriptor	*descPtr;
    ReturnStatus	status = SUCCESS;
    int			newLastByte;

    if (size < 0) {
	return(GEN_INVALID_ARG);
    }
    LFS_STATS_INC(lfsPtr->stats.desc.truncs);

    descPtr = handlePtr->descPtr;

    newLastByte = size - 1;
    if (descPtr->lastByte <= newLastByte) {
	status = SUCCESS;
	goto exit;
    }

    /*
     * Pause the write back during the truncate to sync access to
     * file index.
     */
    Fscache_PreventWriteBacks(&handlePtr->cacheInfo);
    status = LfsFile_TruncIndex(lfsPtr, handlePtr, size);
    if (status == SUCCESS) {
	if (size == 0) {
	    int	newVersion;
	    LFS_STATS_INC(lfsPtr->stats.desc.truncSizeZero);
	    status = LfsDescMapIncVersion(lfsPtr, 
			handlePtr->hdr.fileID.minor, &newVersion);
	}
	descPtr->lastByte = newLastByte;
	descPtr->descModifyTime = Fsutil_TimeInSeconds();
	if (delete) {
	    /*
	     * XXX - need sync here. 
	     */
	    descPtr->flags &= ~FSDM_FD_DIRTY;
	} else {
	    descPtr->flags |= FSDM_FD_SIZE_DIRTY;
	}
    }
    Fscache_AllowWriteBacks(&handlePtr->cacheInfo);
exit:
    if (delete) { 
	LFS_STATS_INC(lfsPtr->stats.desc.delete);
	/*
	 * XXX - need sync here. 
	 */
	descPtr->flags &= ~FSDM_FD_DIRTY;
	status = Fscache_RemoveFileFromDirtyList(&handlePtr->cacheInfo);
    } else {
	if (descPtr->flags & FSDM_FD_DIRTY) {
	    status = Fscache_PutFileOnDirtyList(&handlePtr->cacheInfo, 
				FSCACHE_FILE_DESC_DIRTY);
	}
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_FileDescInit() --
 *
 *	Initialize a new file descriptor.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	The file decriptor is initialized.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Lfs_FileDescInit(domainPtr, fileNumber, type, permissions, uid, gid, fileDescPtr)
    Fsdm_Domain 	*domainPtr;	/* Domain of the file */
    int			fileNumber; 	/* Number of file descriptor */
    int			type;		/* Type of the file */
    int			permissions;	/* Permission bits for the file */
    int			uid;		/* Owner ID for the file */
    int			gid;		/* Group ID for the file */
    Fsdm_FileDescriptor	*fileDescPtr;	/* File descriptor structure to
					   initialize. */
{
    register int index;
    LFS_STATS_INC((LfsFromDomainPtr(domainPtr))->stats.desc.inits);

    fileDescPtr->magic = FSDM_FD_MAGIC;
    fileDescPtr->flags = FSDM_FD_ALLOC|FSDM_FD_DIRTY;
    fileDescPtr->fileType = type;
    fileDescPtr->permissions = permissions;
    fileDescPtr->uid = uid;
    fileDescPtr->gid = gid;
    fileDescPtr->lastByte = -1;
    fileDescPtr->lastByteXtra = 0;
    fileDescPtr->firstByte = -1;
    fileDescPtr->userType = FS_USER_TYPE_UNDEFINED;
    fileDescPtr->numLinks = 1;

    /*
     * Clear out device info.  It is set up properly by the make-device routine.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  These times should come from the client.
     */
    fileDescPtr->createTime = Fsutil_TimeInSeconds();
    fileDescPtr->accessTime = fileDescPtr->createTime;
    fileDescPtr->descModifyTime = fileDescPtr->createTime;
    fileDescPtr->dataModifyTime = fileDescPtr->createTime;

    for (index = 0; index < FSDM_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FSDM_NIL_INDEX;
    }
    for (index = 0; index < FSDM_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FSDM_NIL_INDEX;
    }
    fileDescPtr->numKbytes = 0;
    /*
     * Give this new file a new version number.  The increment is by 2 to
     * ensure that a client invalidates any cache blocks associated with
     * the previous incarnation of the file.  Remember that when a client
     * opens for writing a version number 1 greater means that its old
     * cache blocks are still ok, and also remember that clients with
     * clean blocks are not told when a file is deleted.
     */
    fileDescPtr->version = LfsGetCurrentTimestamp(LfsFromDomainPtr(domainPtr));
    return(SUCCESS);
}

/*
 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 * End of routines exported to higher levels of the file system.
 * @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 *
 * Start of LFS private routines. 
 */

/*
 *----------------------------------------------------------------------
 *
 * LfsDescCacheInit --
 *
 *	Initialize the descriptor cache for a file system.
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
LfsDescCacheInit(lfsPtr)
    Lfs		*lfsPtr;
{
    Fscache_Attributes		attr;
    /*
     * Initialize the file handle used to cache descriptor blocks.
     */

    bzero((char *)(&lfsPtr->descCache.handle),sizeof(lfsPtr->descCache.handle));
    lfsPtr->descCache.handle.hdr.fileID.serverID = rpc_SpriteID;
    lfsPtr->descCache.handle.hdr.fileID.major = lfsPtr->domainPtr->domainNumber;
    lfsPtr->descCache.handle.hdr.fileID.minor = 0;
    lfsPtr->descCache.handle.hdr.fileID.type = FSIO_LCL_FILE_STREAM;
    lfsPtr->descCache.handle.descPtr = (Fsdm_FileDescriptor *)NIL;


    bzero((Address)&attr, sizeof(attr));
    attr.lastByte = 0x7fffffff;
    Fscache_FileInfoInit(&lfsPtr->descCache.handle.cacheInfo,
		    (Fs_HandleHeader *) &lfsPtr->descCache.handle,
		    0, TRUE, &attr, lfsPtr->domainPtr->backendPtr);

}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescCacheBlockInit --
 *
 *	Initialize a block into the descriptor cache for a file system.
 *
 * Results:
 *	NIL if the block couldn't be initialized. A clientData otherwise. 
 *
 * Side effects:
 *	Cache block is fetched and held.
 *
 *----------------------------------------------------------------------
 */

ClientData
LfsDescCacheBlockInit(lfsPtr, diskBlockAddr, cantBlock, blockStartPtr)
    Lfs		*lfsPtr;	/* File system. */
    LfsDiskAddr	diskBlockAddr;	/* Descriptor cache block address. */
    Boolean 	cantBlock;	/* TRUE if process can`t block. */
    char	**blockStartPtr;	/* OUT: pointer to lock's contents. */
{
    Fscache_Block	*blockPtr;
    int			blockNum;
    Boolean	found;

    blockNum = LfsDiskAddrToOffset(diskBlockAddr);
    Fscache_FetchBlock(&lfsPtr->descCache.handle.cacheInfo,
	    blockNum, (FSCACHE_DESC_BLOCK|FSCACHE_CANT_BLOCK| (cantBlock ? FSCACHE_DONT_BLOCK:0)),
		    &blockPtr, &found);
    if (blockPtr == (Fscache_Block *) NIL) {
	return (ClientData) blockPtr;
    }
    if (!found) {
	if ((*blockStartPtr) != (char *) NIL) {
	    bcopy(*blockStartPtr, blockPtr->blockAddr,
	       lfsPtr->fileLayout.params.descPerBlock * 
			LFS_FILE_DESC_SIZE);
	} else {
	    bzero(blockPtr->blockAddr, lfsPtr->fileLayout.params.descPerBlock * 
			LFS_FILE_DESC_SIZE);
	}
	Fscache_IODone(blockPtr);
    } else {
#ifdef ERROR_CHECK
	if (((*blockStartPtr) != (char *) NIL) && 
	    (bcmp(*blockStartPtr, blockPtr->blockAddr, 
		lfsPtr->fileLayout.params.descPerBlock * LFS_FILE_DESC_SIZE) 
		!= 0)) {
		panic("LfsDescCacheBlockInit found wrong block\n");
	}
#endif
    }
    if (*blockStartPtr == (char *) NIL) { 
	*blockStartPtr = blockPtr->blockAddr;
    }
    return (ClientData) blockPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsDescCacheBlockRelease --
 *
 *	Release a desc cache block return by LfsDescCacheBlockInit.
 *
 * Results:
 *	void
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
LfsDescCacheBlockRelease(lfsPtr, clientData, deleteBlock)
    Lfs		*lfsPtr;
    ClientData	clientData;
    Boolean	deleteBlock; 	/* TRUE if block should be deleted. */
{
    Fscache_Block	*blockPtr = (Fscache_Block *) clientData;
    Fscache_UnlockBlock(blockPtr,(unsigned)0, -1, FS_BLOCK_SIZE, 
		deleteBlock ? FSCACHE_DELETE_BLOCK : 0);
}

