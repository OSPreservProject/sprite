/* 
 * lfsDescMap.c --
 *
 *	Routines providing access fields to the LFS descriptor map.
 *	This modules responsible for cacheing, writing, and checkpointing the
 *	LFS descriptor map for a file system.
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
#include <lfsDescMap.h>
#include <lfsDesc.h>
#include <lfsStableMemInt.h>
#include <lfsSeg.h>
#include <fsutil.h>


/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapGetVersion --
 *
 *	Return the descriptor map truncate version for the specified
 *	file.
 *
 * Results:
 *	SUCCESS if the entry is resident in the descriptor map.
 *	FS_FILE_NOT_FOUND if the file is not allocated.
 *	GEN_INVALID_ARG if the fileNumber is not available in the map.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapGetVersion(lfsPtr, fileNumber, versionNumPtr)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number of descriptor. */ 
    unsigned short  *versionNumPtr; /* Area to return version number in.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsStableMemEntry smemEntry;
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status;

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE,
		&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FS_FILE_NOT_FOUND;
    } else { 
	*versionNumPtr = entryPtr->truncVersion;
    }
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, FALSE);
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapIncVersion --
 *
 *	Increment the truncate version number for the specified
 *	file.
 *
 * Results:
 *	SUCCESS if the entry is resident in the descriptor map.
 *	GEN_INVALID_ARG if the fileNumber is not available in the map.
 *	
 * Side effects:
 *	Version number of entry increment.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapIncVersion(lfsPtr, fileNumber, versionPtr)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number of descriptor. */ 
    int	  *versionPtr;  /* OUT: New truncate version number. */
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status;
    LfsStableMemEntry	smemEntry;

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE,
					&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    *versionPtr = ++(entryPtr->truncVersion);
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, TRUE);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapGetDiskAddr --
 *
 *	Return the disk address of the specified descriptor.
 *
 * Results:
 *	SUCCESS if the entry is resident in the descriptor map.
 *	FS_FILE_NOT_FOUND if the file is not allocated.
 *	GEN_INVALID_ARG if the fileNumber is not available in the map.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapGetDiskAddr(lfsPtr, fileNumber, diskAddrPtr)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number of descriptor. */ 
    LfsDiskAddr  *diskAddrPtr; /* Current disk address.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status;
    LfsStableMemEntry	smemEntry;

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE,
			 &smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FS_FILE_NOT_FOUND;
    }
    *diskAddrPtr = entryPtr->blockAddress;
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, FALSE);
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapSetDiskAddr --
 *
 *	Set the disk address for the specified descriptor.
 *
 * Results:
 *	SUCCESS if the entry is resident in the descriptor map.
 *	FAILURE if the fileNumber is not available in the map.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapSetDiskAddr(lfsPtr, fileNumber, diskAddr)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number of descriptor. */ 
    LfsDiskAddr diskAddr; /* New disk address.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    ReturnStatus	status;
    LfsStableMemEntry	smemEntry;

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE,
				&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FAILURE;
    } else { 
	if (!LfsIsNullDiskAddr(entryPtr->blockAddress)) { 
	    LfsSegUsageFreeBlocks(lfsPtr, sizeof(LfsFileDescriptor), 1, 
			      &entryPtr->blockAddress);
	    LFS_STATS_INC(lfsPtr->stats.desc.descMoved);
	}
	entryPtr->blockAddress = diskAddr;
    }
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, (status == SUCCESS));
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapGetAccessTime --
 *
 *	Return the file access time for the specified file number.
 *
 * Results:
 *	SUCCESS if the entry is resident in the descriptor map.
 *	FAILURE if the fileNumber is not available in the map.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapGetAccessTime(lfsPtr, fileNumber, accessTimePtr)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number of descriptor. */ 
    int  *accessTimePtr; /* Current access time.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status;
    LfsStableMemEntry	smemEntry;

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE, 
				&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FAILURE;
    }
    *accessTimePtr = entryPtr->accessTime;
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, FALSE);
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapSetAccessTime --
 *
 *	Set the file access time for the specified file number.
 *
 * Results:
 *	SUCCESS if the entry is resident in the descriptor map.
 *	FAILURE if the fileNumber is not available in the map.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapSetAccessTime(lfsPtr, fileNumber, accessTime)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number of descriptor. */ 
    int  accessTime; /* New current access time.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status;
    LfsStableMemEntry	smemEntry;

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE,
				&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FAILURE;
    } else { 
	entryPtr->accessTime = accessTime;
    }
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, (status == SUCCESS));

    return status;

}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_GetNewFileNumber --
 *
 *	Allocate an used file number for a newly created file or directory.
 *
 * Results:
 *	An error if could not find a free file descriptor.
 *	SUCCESS if a file number can be allocate.
 *	FAILURE if all available file numbers are taken.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_GetNewFileNumber(domainPtr, dirFileNum, fileNumberPtr)
    Fsdm_Domain 	*domainPtr;	/* Domain to allocate the file 
					 * descriptor out of. */
    int	 dirFileNum;	/* File number of the directory that
			 * the file is in.  -1 means that
			 * this file descriptor is being
			 * allocated for a directory. */
    int	*fileNumberPtr; /* Place to return the file number allocated. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    register LfsDescMapEntry   *entryPtr;
    register int maxNumDesc, startDesc, i;
    Boolean	found = FALSE;
    ReturnStatus      status;
    static	int dirSeed = 0;
    LfsStableMemEntry	smemEntry;

    maxNumDesc = mapPtr->params.maxDesc;
    LFS_STATS_INC(lfsPtr->stats.desc.getNewFileNumber);
    if (dirFileNum == -1) {
	if (dirSeed == 0) {
	    dirSeed = Fsutil_TimeInSeconds();
	} 
        /*
         * Search linearly from a random starting descriptor.
         */
        startDesc = ((dirSeed * 1103515245 + 12345) & 0x7fffffff) %
                        maxNumDesc;
	dirSeed++;
    } else {
	startDesc = dirFileNum;
    }
    status = LfsStableMemFetch(&(mapPtr->stableMem), startDesc, FALSE,
			&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);
    i = startDesc;
    do { 
	if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	    found = TRUE;
	    break;
	}
	LFS_STATS_INC(lfsPtr->stats.desc.scans);
	i++;
        if (i == maxNumDesc) {
	    i = 0;
	}
	status = LfsStableMemFetch(&(mapPtr->stableMem), i, TRUE, &smemEntry);
	if (status != SUCCESS) {
	    return status;
	}
	entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);
   } while (i != startDesc);
    if (!found) {	
	LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, FALSE);
        printf( "Out of file descriptors.\n");
	return FAILURE;
    }
    mapPtr->checkPoint.numAllocDesc++;
    LfsSetNilDiskAddr(&entryPtr->blockAddress);
    entryPtr->flags = LFS_DESC_MAP_ALLOCED;
    *fileNumberPtr = i;
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, TRUE);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_FreeFileNumber() --
 *
 *	Mark a file number as unused and make it available for re-allocation.
 *
 * Results:
 *	SUCCESS if a file number was not allocated.
 *	FAILURE if all available file numbers are taken.
 *	
 * Side effects:
 *	Descriptor map entry is modified for the file.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_FreeFileNumber(domainPtr, fileNumber)
    Fsdm_Domain 	*domainPtr;	/* Domain of the file descriptor. */
    int	  fileNumber;   /* File number to free. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status;
    LfsStableMemEntry	smemEntry;

    LFS_STATS_INC(lfsPtr->stats.desc.free);

    status = LfsStableMemFetch(&(mapPtr->stableMem), fileNumber, FALSE,
		&smemEntry);
    if (status != SUCCESS) {
	return status;
    }
    entryPtr = (LfsDescMapEntry *) LfsStableMemEntryAddr(&smemEntry);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FAILURE;
    } else { 
	entryPtr->flags &= ~LFS_DESC_MAP_ALLOCED;
	LfsSegUsageFreeBlocks(lfsPtr, sizeof(LfsFileDescriptor), 1, 
			  &entryPtr->blockAddress);
	LfsSetNilDiskAddr(&entryPtr->blockAddress);
	mapPtr->checkPoint.numAllocDesc--;
    }
    LfsStableMemRelease(&(mapPtr->stableMem), &smemEntry, (status == SUCCESS));
    return status;
}



extern ReturnStatus LfsDescMapAttach _ARGS_((Lfs *lfsPtr, int checkPointSize, 
		char *checkPointPtr));
extern Boolean LfsDescMapCheckpoint _ARGS_((LfsSeg *segPtr, int flags, 
		char *checkPointPtr, int *checkPointSizePtr, 
		ClientData *clientDataPtr));
extern Boolean LfsDescMapLayout _ARGS_((LfsSeg *segPtr, int flags, 
		ClientData *clientDataPtr));
extern void LfsDescMapWriteDone _ARGS_((LfsSeg *segPtr, int flags,
		ClientData *clientDataPtr));
extern Boolean LfsDescMapClean _ARGS_((LfsSeg *segPtr, int *sizePtr, 
		int *numCacheBlocksPtr, ClientData *clientDataPtr));

static LfsSegIoInterface descMapIoInterface = 
	{ LfsDescMapAttach, LfsDescMapLayout, LfsDescMapClean,
	  LfsDescMapCheckpoint, LfsDescMapWriteDone,  0};


/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapInit --
 *
 *	Initialize the the descriptor map data structures.  
 *
 * Results:
 *	None
 *	
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
LfsDescMapInit()
{
    LfsSegIoRegister(LFS_DESC_MAP_MOD,&descMapIoInterface);
}


/*
 *----------------------------------------------------------------------
 *
 * DescMapAttach --
 *
 *	Attach routine for the descriptor map. Creates and initializes the
 *	map for this file system.
 *
 * Results:
 *	SUCCESS if attaching is going ok.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapAttach(lfsPtr, checkPointSize, checkPointPtr)
    Lfs   *lfsPtr;	     /* File system for attach. */
    int   checkPointSize;    /* Size of checkpoint data. */
    char  *checkPointPtr;     /* Data from last checkpoint before shutdown. */
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapCheckPoint *cp = (LfsDescMapCheckPoint *) checkPointPtr;
    ReturnStatus	status;
    int			size;

    /*
     * Allocate and fill in memory data structure for descriptor map.
     */
    mapPtr->params = lfsPtr->superBlock.descMap;
    mapPtr->checkPoint = *cp;
    /*
     * Load the index and buffer using the LfsStableMem routines.
     */
    size = sizeof(LfsDescMapCheckPoint);
    status = LfsStableMemLoad(lfsPtr, &(mapPtr->params.stableMem), 
			checkPointSize - size,
			checkPointPtr + size,
			&(mapPtr->stableMem));
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't loading descriptor map index\n");
	return status;
    }

    printf("LfsDescMapAttach - %d allocated descriptors.\n", 
	    mapPtr->checkPoint.numAllocDesc);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * DescMapCheckpoint --
 *
 *	Routine to handle checkpointing of the descriptor map data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	checkpointed.
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsDescMapCheckpoint(segPtr, flags, checkPointPtr, checkPointSizePtr, 
			clientDataPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for shutdown */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
    ClientData *clientDataPtr;	
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapCheckPoint *cp = (LfsDescMapCheckPoint *) checkPointPtr;
    int		size, dataSize;
    Boolean	full;

    *cp = mapPtr->checkPoint;
    size = sizeof(LfsDescMapCheckPoint);
    dataSize = 0;
    full = LfsStableMemCheckpoint(segPtr, checkPointPtr + size, flags,
			&dataSize, clientDataPtr, &(mapPtr->stableMem));
    if (!full) { 
	(*checkPointSizePtr) = dataSize + size;
    }
    return full;

}

/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapLayout --
 *
 *	Routine to handle writing of the descriptor map data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	statisified
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */
Boolean
LfsDescMapLayout(segPtr, flags, clientDataPtr)
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
    int	flags;		/* Flags. */
    ClientData	*clientDataPtr;
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);

    if ((flags & LFS_CLEANING_LAYOUT) != 0) {
	return FALSE;
    }
    return LfsStableMemLayout(segPtr, flags, clientDataPtr, &(mapPtr->stableMem));
}

/*
 *----------------------------------------------------------------------
 *
 * DescWriteDone --
 *
 *	Routine to handle finishing of a checkpoint.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Many
 *
 *----------------------------------------------------------------------
 */

void
LfsDescMapWriteDone(segPtr, flags, clientDataPtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
    ClientData *clientDataPtr;
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);

    LFS_STATS_ADD(lfsPtr->stats.desc.mapBlocksWritten,
		(LfsSegSummaryBytesLeft(segPtr) / sizeof(int)));

    LfsStableMemWriteDone(segPtr, flags, clientDataPtr, &(mapPtr->stableMem));
    return;

}


/*
 *----------------------------------------------------------------------
 *
 * DescMapClean --
 *
 *	Routine to handle cleaning of descriptor map data.
 *
 * Results:
 *	TRUE if more data needs to be written, FALSE if this module is
 *	happy for the time being.
 *
 * Side effects:
 *	
 *
 *----------------------------------------------------------------------
 */

Boolean
LfsDescMapClean(segPtr, sizePtr, numCacheBlocksPtr, clientDataPtr)
    LfsSeg *segPtr;	/* Segment containing data to clean. */
    int	   *sizePtr;		/* Segment to place data blocks in. */
    int *numCacheBlocksPtr;
    ClientData *clientDataPtr;
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    Boolean full;

    full =  LfsStableMemClean(segPtr, sizePtr, numCacheBlocksPtr, 
		clientDataPtr, &(mapPtr->stableMem));

    LFS_STATS_ADD(lfsPtr->stats.desc.mapBlockCleaned,
		*sizePtr/mapPtr->stableMem.params.blockSize);
    return full;
}

