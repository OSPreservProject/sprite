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

#include "sprite.h"
#include "lfs.h"
#include "lfsInt.h"
#include "lfsDescMap.h"
#include "lfsDesc.h"
#include "lfsStableMem.h"
#include "lfsSeg.h"
#include "fsutil.h"


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
 *	FAILURE if the fileNumber is not available in the map.
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
    LfsDescMapEntry   *entryPtr;

    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }

    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FAILURE;
    }
    *versionNumPtr = entryPtr->truncVersion;
    return SUCCESS;

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
 *	FAILURE if the fileNumber is not available in the map.
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

    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }
    lfsPtr->dirty = TRUE;
    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    LfsStableMemMarkDirty(&(mapPtr->stableMem),(char *)entryPtr, sizeof(*entryPtr));
    *versionPtr = ++(entryPtr->truncVersion);
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
 *	FAILURE if the fileNumber is not available in the map.
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
    unsigned int  *diskAddrPtr; /* Current disk address.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;

    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }

    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FAILURE;
    }
    *diskAddrPtr = entryPtr->blockAddress;
    return SUCCESS;

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
    unsigned int  diskAddr; /* New disk address.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }
    lfsPtr->dirty = TRUE;

    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FAILURE;
    }
    LfsStableMemMarkDirty(&(mapPtr->stableMem),(char *)entryPtr, sizeof(*entryPtr));
    LfsSegUsageFreeBlocks(lfsPtr, sizeof(LfsFileDescriptor), 1, 
			  &entryPtr->blockAddress);
    entryPtr->blockAddress = diskAddr;
    return SUCCESS;


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
    unsigned int  *accessTimePtr; /* Current access time.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;

    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }

    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FAILURE;
    }
    *accessTimePtr = entryPtr->accessTime;
    return SUCCESS;

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
    unsigned int  accessTime; /* New current access time.*/
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }
    lfsPtr->dirty = TRUE;

    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FAILURE;
    }
    LfsStableMemMarkDirty(&(mapPtr->stableMem),(char *)entryPtr, sizeof(*entryPtr));
    entryPtr->accessTime = accessTime;
    return SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapAllocFileNum --
 *
 *	Allocate an unused file number for a newly created file
 *	or directory.
 *
 * Results:
 *	SUCCESS if a file number can be allocate.
 *	FAILURE if all available file numbers are taken.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapAllocFileNum(lfsPtr, dirFileNum, fileNumberPtr)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	 dirFileNum;	/* File number of the directory that
			 * the file is in.  -1 means that
			 * this file descriptor is being
			 * allocated for a directory. */
    int	*fileNumberPtr; /* Place to return the file number allocated. */
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    register LfsDescMapEntry   *entryPtr;
    register int maxNumDesc, startDesc, i;
    Boolean	found = FALSE;
    static	int dirSeed = 0;

    maxNumDesc = mapPtr->params.maxDesc;

    if (dirFileNum == -1) {
	if (dirSeed == 0) {
	    dirSeed = fsutil_TimeInSeconds;
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
    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + startDesc;
    i = startDesc;
    do { 
	if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	    found = TRUE;
	    break;
	}
	i++;
	entryPtr++;
        if (i == maxNumDesc) {
	    i = 0;
	    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr);
	}

    } while (i != startDesc);
    if (!found) {	
        printf( "Out of file descriptors.\n");
	return FAILURE;
    }
    lfsPtr->dirty = TRUE;
    mapPtr->checkPoint.numAllocDesc++;
    entryPtr->flags = LFS_DESC_MAP_ALLOCED;
    *fileNumberPtr = i;
    LfsStableMemMarkDirty(&(mapPtr->stableMem),(char *)entryPtr, sizeof(*entryPtr));
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * LfsDescMapFreeFileNum --
 *
 *	Mark a file number as unused.
 *
 * Results:
 *	SUCCESS if a file number was not allocated.
 *	FAILURE if all available file numbers are taken.
 *	
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsDescMapFreeFileNum(lfsPtr, fileNumber)
    Lfs	  *lfsPtr;	/* File system of descriptor. */
    int	  fileNumber;   /* File number to free. */
{
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapEntry   *entryPtr;
    /*
     * Do some error checking.
     */
    if ((fileNumber < 0) || (fileNumber >= mapPtr->params.maxDesc)) {
	return FAILURE;
    }
    entryPtr = ((LfsDescMapEntry *) mapPtr->stableMem.dataPtr) + fileNumber;

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	return FAILURE;
    }
    lfsPtr->dirty = TRUE;
    LfsStableMemMarkDirty(&(mapPtr->stableMem),(char *)entryPtr,
			sizeof(*entryPtr));
    entryPtr->flags &= ~LFS_DESC_MAP_ALLOCED;
    mapPtr->checkPoint.numAllocDesc--;
    return SUCCESS;
}

static ReturnStatus DescMapAttach();
static Boolean	    DescMapClean();
static Boolean 	    DescMapCheckpoint();
static void	    DescMapWriteDone();

static LfsSegIoInterface descMapIoInterface = 
	{ DescMapAttach, LfsSegNullLayout, DescMapClean,
	  DescMapCheckpoint, DescMapWriteDone,  0};


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

static ReturnStatus
DescMapAttach(lfsPtr, checkPointSize, checkPointPtr)
    Lfs   *lfsPtr;	     /* File system for attach. */
    int   checkPointSize;    /* Size of checkpoint data. */
    char  *checkPointPtr;     /* Data from last checkpoint before shutdown. */
{
    LfsDescMapParams  *paramPtr;
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
    status = LfsStableMemLoad(lfsPtr, &(paramPtr->stableMem), 
			checkPointSize - size,
			checkPointPtr + size,
			&(mapPtr->stableMem));
    if (status != SUCCESS) {
	LfsError(status, lfsPtr,"Can't loading descriptor map index\n");
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

static Boolean
DescMapCheckpoint(segPtr, flags, checkPointPtr, checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for shutdown */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapCheckPoint *cp = (LfsDescMapCheckPoint *) checkPointPtr;
    int		size;
    Boolean	full;

    *cp = mapPtr->checkPoint;
    size = sizeof(LfsDescMapCheckPoint);
    full = LfsStableMemCheckpoint(segPtr, checkPointPtr + size, flags,
				checkPointSizePtr, &(mapPtr->stableMem));
    (*checkPointSizePtr) = (*checkPointSizePtr) + size;
    return full;

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

static void
DescMapWriteDone(segPtr, flags)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);

    LfsStableMemWriteDone(segPtr, flags, &(mapPtr->stableMem));
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

static Boolean
DescMapClean(segToCleanPtr, segPtr)
    LfsSeg *segToCleanPtr;	/* Segment containing data to clean. */
    LfsSeg *segPtr;		/* Segment to place data blocks in. */
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);

    return LfsStableMemClean(segToCleanPtr, segPtr, &(mapPtr->stableMem));

}

