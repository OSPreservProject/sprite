/* 
 * fsFileDesc.c --
 *
 *	Routines to allocate, initialize, and free file descriptors.
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
#include "fsLocalDomain.h"
#include "fsOpTable.h"
#include "fsDevice.h"
#include "fsStat.h"

ReturnStatus	FsFetchFileDesc();
ReturnStatus	FsStoreFileDesc();

#define LOCKPTR (&domainPtr->fileDescLock)

/*
 * Array to provide the ability to set and extract bits out of a bitmap byte.
 */

static unsigned char bitmasks[8] = {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

int	fsDescSearchStarts = 0;
int	fsDescBytesSearched = 0;


/*
 *----------------------------------------------------------------------
 *
 * FsFileDescAllocInit --
 *
 *	Initialize the data structure needed for allocation of file
 *	descriptors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated for the bit map and the bit map is read in.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsFileDescAllocInit(domainPtr)
    register FsDomain *domainPtr;
{
    register ReturnStatus	status;

    SYNC_LOCK_INIT_DYNAMIC(&(domainPtr->fileDescLock));
    /*
     * Allocate the bit map.
     */

    domainPtr->fileDescBitmap = (unsigned char *) 
	malloc(domainPtr->headerPtr->fdBitmapBlocks * FS_BLOCK_SIZE);

    /* 
     * Read in the bit map.
     */

    status = FsDeviceBlockIO(FS_READ, &(domainPtr->headerPtr->device), 
		    domainPtr->headerPtr->fdBitmapOffset * 4, 
		    domainPtr->headerPtr->fdBitmapBlocks * 4,
		    (Address) domainPtr->fileDescBitmap);
    if (status != SUCCESS) {
	printf( "Could not read in file descriptor bit map.\n");
	return(status);
    } else {
	fsStats.gen.physBytesRead += FS_BLOCK_SIZE;
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsWriteBackFileDescBitmap() --
 *
 *	Write the file descriptor bit map out to disk for this domain.
 *
 * Results:
 *	Error if the write failed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
FsWriteBackFileDescBitmap(domainPtr)
    register FsDomain *domainPtr;
{
    register ReturnStatus	status;

    LOCK_MONITOR;

    status = FsDeviceBlockIO(FS_WRITE, &(domainPtr->headerPtr->device), 
		    domainPtr->headerPtr->fdBitmapOffset * 4, 
		    domainPtr->headerPtr->fdBitmapBlocks * 4,
		    (Address) domainPtr->fileDescBitmap);
    if (status != SUCCESS) {
	printf( "Could not write out file desc bit map.\n");
    } else {
	fsStats.gen.physBytesWritten += FS_BLOCK_SIZE;
    }

    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsGetNewFileNumber() --
 *
 *	Get a new file number by allocating a free file descriptor
 *	from the file descriptor bitmap.
 *
 * Results:
 *	An error if could not find a free file descriptor.
 *
 * Side effects:
 *	fileNumberPtr is set to the number of the file descriptor allocated.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
FsGetNewFileNumber(domainPtr, dirFileNum, fileNumberPtr)
    register FsDomain 	*domainPtr;	/* Domain to allocate the file 
					 * descriptor out of. */
    int			dirFileNum;	/* File number of the directory that
					   the file is in.  -1 means that
					   this file descriptor is being
					   allocated for a directory. */
    int			*fileNumberPtr; /* Place to return the number of
					   the file descriptor allocated. */
{
    register int 	   	i;
    register int		j;
    int				startByte;
    register unsigned char 	*bitmapPtr;
    register unsigned char 	*bitmaskPtr;
    Boolean		   	found = FALSE;
    int			   	descBytes;

    LOCK_MONITOR;

    fsDescSearchStarts++;
    descBytes = domainPtr->headerPtr->numFileDesc >> 3;
    
    if (dirFileNum == -1) {
	/*
	 * Search linearly from a random starting byte.
	 */
	startByte = ((fsTimeInSeconds * 1103515245 + 12345) & 0x7fffffff) % 
			descBytes;
    } else {
	/*
	 * Start search where directory is.
	 */
	startByte = dirFileNum / 8;
    }

    /*
     * Linear search forward the bit map a byte at a time.
     */
    bitmapPtr = &(domainPtr->fileDescBitmap[startByte]);
    i = startByte;
    do {
	fsDescBytesSearched++;
	if (*bitmapPtr != 0xff) {
	    found = TRUE;
	    break;
	}
	i++;
	if (i == descBytes) {
	    i = 0;
	    bitmapPtr = domainPtr->fileDescBitmap;
	} else {
	    bitmapPtr++;
	}
    } while (i != startByte);

    if (!found) {
	printf( "Out of file descriptors.\n");
	UNLOCK_MONITOR;
	return(FAILURE);
    }

    domainPtr->summaryInfoPtr->numFreeFileDesc--;
    /*
     * Now find which file descriptor is free within the byte.
     */
    for (j = 0, bitmaskPtr = bitmasks; 
	 j < 8 && (*bitmapPtr & *bitmaskPtr) != 0; 
	 j++, bitmaskPtr++) {
    }
    *fileNumberPtr = i * 8 + j;
    *bitmapPtr |= *bitmaskPtr;

    UNLOCK_MONITOR;

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsFreeFileNumber() --
 *
 *	Free a file number by clearing the corresponding bit the in
 *	file descriptor bit map.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Bit map modified.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
FsFreeFileNumber(domainPtr, fileNumber)
    register FsDomain 	*domainPtr;	/* Domain that the file 
					 * descriptor is in. */
    int			fileNumber; 	/* Number of file descriptor to 
					   free.*/
{
    LOCK_MONITOR;

    domainPtr->summaryInfoPtr->numFreeFileDesc++;
    domainPtr->fileDescBitmap[fileNumber / 8] &= ~bitmasks[fileNumber & 0x7];

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * FsInitFileDesc() --
 *
 *	Initialize a new file descriptor.
 *
 * Results:
 *	An error if could not read the file descriptor from disk.
 *
 * Side effects:
 *	The file decriptor is initialized.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsInitFileDesc(domainPtr, fileNumber, type, permissions, uid, gid, fileDescPtr)
    register FsDomain 	*domainPtr;	/* Domain of the file */
    int			fileNumber; 	/* Number of file descriptor */
    int			type;		/* Type of the file */
    int			permissions;	/* Permission bits for the file */
    int			uid;		/* Owner ID for the file */
    int			gid;		/* Group ID for the file */
    register FsFileDescriptor	*fileDescPtr;	/* File descriptor structure to
					   initialize. */
{
    ReturnStatus status;
    register int index;
    /*
     * Fetch the file descriptor and do rudimentation consistency checks.
     * This also gets its block into the cache which will happen sooner
     * or later anyway.
     */
    status = FsFetchFileDesc(domainPtr, fileNumber, fileDescPtr);
    if (status != SUCCESS) {
	return(status);
    }
    if (fileDescPtr->flags != FS_FD_FREE) {
	printf( "FsInitFileDesc fetched non-free file desc\n");
	return(FS_FILE_EXISTS);
    }
    fileDescPtr->magic = FS_FD_MAGIC;
    fileDescPtr->flags = FS_FD_ALLOC;
    fileDescPtr->fileType = type;
    fileDescPtr->permissions = permissions;
    fileDescPtr->uid = uid;
    fileDescPtr->gid = gid;
    fileDescPtr->lastByte = -1;
    fileDescPtr->firstByte = -1;
    fileDescPtr->userType = FS_USER_TYPE_UNDEFINED;
    fileDescPtr->numLinks = 1;
    fileDescPtr->numKbytes = 0;
    /*
     * Give this new file a new version number.  The increment is by 2 to
     * ensure that a client invalidates any cache blocks associated with
     * the previous incarnation of the file.  Remember that when a client
     * opens for writing a version number 1 greater means that its old
     * cache blocks are still ok, and also remember that clients with
     * clean blocks are not told when a file is deleted.
     */
    fileDescPtr->version += 2;

    /*
     * Clear out device info.  It is set up properly by the make-device routine.
     */
    fileDescPtr->devServerID = -1;
    fileDescPtr->devType = -1;
    fileDescPtr->devUnit = -1;

    /*
     * Set the time stamps.  This assumes that universal time, not local
     * time, is used for time stamps.
     */
    fileDescPtr->createTime = fsTimeInSeconds;
    fileDescPtr->accessTime = fsTimeInSeconds;
    fileDescPtr->descModifyTime = fsTimeInSeconds;
    fileDescPtr->dataModifyTime = fsTimeInSeconds;

    for (index = 0; index < FS_NUM_DIRECT_BLOCKS ; index++) {
	fileDescPtr->direct[index] = FS_NIL_INDEX;
    }
    for (index = 0; index < FS_NUM_INDIRECT_BLOCKS ; index++) {
	fileDescPtr->indirect[index] = FS_NIL_INDEX;
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsFetchFileDesc() --
 *
 *	Fetch the given file descriptor from disk and store it into
 *	*fileDescPtr.
 *
 * Results:
 *	An error if could not read the file descriptor from disk.
 *
 * Side effects:
 *	*fileDescPtr is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsFetchFileDesc(domainPtr, fileNumber, fileDescPtr)
    register FsDomain 	*domainPtr;	/* Domain to fetch the file 
					 * descriptor from. */
    register int	fileNumber; 	/* Number of file descriptor to 
					   fetch.*/
    FsFileDescriptor	*fileDescPtr;	/* File descriptor structure to
					   initialize. */
{
    register ReturnStatus     status;
    register FsDomainHeader *headerPtr;
    register int 	    blockNum;
    int 		    offset;
    FsCacheBlock	    *blockPtr;
    Boolean		    found;

    if (fileNumber == 0) {
	panic( "FsFetchFileDesc: file #0\n");
	return(FAILURE);
    }
    headerPtr = domainPtr->headerPtr;
    blockNum = headerPtr->fileDescOffset + fileNumber / FS_FILE_DESC_PER_BLOCK;
    offset = (fileNumber & (FS_FILE_DESC_PER_BLOCK - 1)) *
		FS_MAX_FILE_DESC_SIZE;

    fsStats.blockCache.fileDescReads++;
    FsCacheFetchBlock(&domainPtr->physHandle.cacheInfo, blockNum, 
		      FS_DESC_CACHE_BLOCK, &blockPtr, &found);
    if (!found) {
	status = FsDeviceBlockIO(FS_READ, &headerPtr->device, 
			   blockNum * FS_FRAGMENTS_PER_BLOCK,
			   FS_FRAGMENTS_PER_BLOCK, blockPtr->blockAddr);
	if (status != SUCCESS) {
	    printf( "Could not read in file descriptor\n");
	    FsCacheUnlockBlock(blockPtr, 0, -1, 0, FS_DELETE_BLOCK);
	    return(status);
	} else {
	    fsStats.gen.physBytesRead += FS_BLOCK_SIZE;
	}
    } else {
	fsStats.blockCache.fileDescReadHits++;
    }
    bcopy(blockPtr->blockAddr + offset, (Address) fileDescPtr,
	sizeof(FsFileDescriptor));
    FsCacheUnlockBlock(blockPtr, 0, blockNum * FS_FRAGMENTS_PER_BLOCK, 
   			 FS_BLOCK_SIZE, 0);

    if (fileDescPtr->magic != FS_FD_MAGIC) {
	printf( "FsFetchFileDesc found junky file desc\n");
	return(FAILURE);
    } else {
	return(SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * FsStoreFileDesc() --
 *
 *	Store the given file descriptor back into the file system block
 *	where it came from.  This involves putting the block back into
 *	the cache.
 *
 * Results:
 *	An error if could not read the file descriptor from disk.
 *
 * Side effects:
 *	Cache block is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsStoreFileDesc(domainPtr, fileNumber, fileDescPtr)
    register FsDomain 	*domainPtr;	/* Domain to store the file 
					 * descriptor into. */
    int			fileNumber; 	/* Number of file descriptor to 
					   store.*/
    FsFileDescriptor	*fileDescPtr;	/* File descriptor structure to
					   store. */
{
    register ReturnStatus   status;
    register FsDomainHeader *headerPtr;
    register int 	    blockNum;
    int 		    offset;
    FsCacheBlock	    *blockPtr;
    Boolean		    found;

    if (fileNumber == 0) {
	panic( "FsStoreFileDesc: file #0\n");
	return(FAILURE);
    }
    headerPtr = domainPtr->headerPtr;
    blockNum = headerPtr->fileDescOffset + fileNumber / FS_FILE_DESC_PER_BLOCK;
    offset = (fileNumber & (FS_FILE_DESC_PER_BLOCK - 1)) *
		FS_MAX_FILE_DESC_SIZE;

    fsStats.blockCache.fileDescWrites++;
    FsCacheFetchBlock(&domainPtr->physHandle.cacheInfo, blockNum, 
		      (int)(FS_IO_IN_PROGRESS | FS_DESC_CACHE_BLOCK),
		      &blockPtr, &found);
    if (!found) {
	status = FsDeviceBlockIO(FS_READ, &headerPtr->device, 
			   blockNum * FS_FRAGMENTS_PER_BLOCK,
			   FS_FRAGMENTS_PER_BLOCK, blockPtr->blockAddr);
	if (status != SUCCESS) {
	    printf( "Could not read in file descriptor\n");
	    FsCacheUnlockBlock(blockPtr, 0, blockNum * FS_FRAGMENTS_PER_BLOCK,
				FS_BLOCK_SIZE, FS_DELETE_BLOCK);
	    return(status);
	} else {
	    fsStats.gen.physBytesWritten += FS_BLOCK_SIZE;
	}
    } else {
	fsStats.blockCache.fileDescWriteHits++;
    }
    bcopy((Address) fileDescPtr, blockPtr->blockAddr + offset, sizeof(FsFileDescriptor));
    /*
     * Put the block back into the cache setting the modify time to 1 which
     * will guarantee that the next time the cache is written back this block
     * is written back as well.
     */
    FsCacheUnlockBlock(blockPtr, 1, blockNum * FS_FRAGMENTS_PER_BLOCK,
			FS_BLOCK_SIZE, 0);
    
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsWriteBackDesc --
 *
 *	Force the file descriptor for the handle to disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File descriptor block forced to disk.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsWriteBackDesc(handlePtr, doWriteBack)
    register FsLocalFileIOHandle	*handlePtr;	/* Handle that points
					 * to descriptor to write back. */
    Boolean		doWriteBack;	/* Do a cache write back, not only
					 * a store into the cache block. */
{
    register FsDomainHeader	*headerPtr;
    register FsFileDescriptor	*descPtr;
    register FsDomain		*domainPtr;
    register ReturnStatus     	status = SUCCESS;
    register int 	    	blockNum;
    int				blocksSkipped;

    domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	return(FS_DOMAIN_UNAVAILABLE);
    }
    descPtr = handlePtr->descPtr;
    /*
     * If the handle times differ from the descriptor times then force
     * them out to the descriptor.
     */
    if (descPtr->accessTime < handlePtr->cacheInfo.attr.accessTime) {
	descPtr->accessTime = handlePtr->cacheInfo.attr.accessTime;
	descPtr->flags |= FS_FD_DIRTY;
    }
    if (descPtr->dataModifyTime < handlePtr->cacheInfo.attr.modifyTime) {
	descPtr->dataModifyTime = handlePtr->cacheInfo.attr.modifyTime;
	descPtr->flags |= FS_FD_DIRTY;
    }
    if (descPtr->flags & FS_FD_DIRTY) {
	descPtr->flags &= ~FS_FD_DIRTY;
	status =  FsStoreFileDesc(domainPtr, handlePtr->hdr.fileID.minor, 
				  descPtr);
	if (status != SUCCESS) {
	    printf("FsWriteBackDesc: Could not put desc <%d,%d> into cache\n",
		    handlePtr->hdr.fileID.major,
		    handlePtr->hdr.fileID.minor);
	}
    }
    if (status == SUCCESS && doWriteBack) {
	headerPtr = domainPtr->headerPtr;
	blockNum = headerPtr->fileDescOffset + 
		   handlePtr->hdr.fileID.minor / FS_FILE_DESC_PER_BLOCK;
	status = FsCacheFileWriteBack(&domainPtr->physHandle.cacheInfo,
		    blockNum, blockNum, FS_FILE_WB_WAIT, &blocksSkipped);
	if (status != SUCCESS) {
	    printf("FsWritebackDesc: Couldn't write back desc <%d,%d>\n",
		    handlePtr->hdr.fileID.major,
		    handlePtr->hdr.fileID.minor);
	}
    }
    FsDomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

