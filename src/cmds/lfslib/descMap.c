/* 
 * descMap.c --
 *
 *	Routines for accessing LFS file systems desc map datastuctures from
 *	a user level program.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#include "lfslibInt.h"
#ifdef _HAS_PROTOTYPES
#include <varargs.h>
#include <sys/types.h>
#endif /* _HAS_PROTOTYPES */
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>
#include <bstring.h>
#include <sys/time.h>

/*
 *----------------------------------------------------------------------
 *
 * LfsGetNewFileNumber --
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
LfsGetNewFileNumber(lfsPtr, dirFileNumber, fileNumberPtr)
    Lfs	 	*lfsPtr;	/* FS to allocate the file 
				 * descriptor out of. */
    int	 dirFileNumber;	/* File number of the directory that
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

    maxNumDesc = lfsPtr->superBlock.descMap.maxDesc;
    if (dirFileNumber == -1) {
	if (dirSeed == 0) {
	    dirSeed = time(0);
	} 
        /*
         * Search linearly from a random starting descriptor.
         */
        startDesc = ((dirSeed * 1103515245 + 12345) & 0x7fffffff) %
                        maxNumDesc;
	dirSeed++;
    } else {
	startDesc = dirFileNumber;
    }
    entryPtr = LfsGetDescMapEntry(lfsPtr, startDesc);
    i = startDesc;
    do { 
	if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	    found = TRUE;
	    break;
	}
	i++;
        if (i == maxNumDesc) {
	    i = 0;
	}
	entryPtr = LfsGetDescMapEntry(lfsPtr, i);
   } while (i != startDesc);
    if (!found) {	
        panic( "Out of file descriptors.\n");
	return FAILURE;
    }
    mapPtr->checkPoint.numAllocDesc++;
    entryPtr->blockAddress = FSDM_NIL_INDEX;
    entryPtr->flags = LFS_DESC_MAP_ALLOCED;
    *fileNumberPtr = i;
    LfsDescMapEntryModified(lfsPtr, i);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * LoadDescMap --
 *
 *	Load the descriptor map array into memory.
 *
 * Results:
 *	TRUE if array can not be loaded. FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
LfsLoadDescMap(lfsPtr, checkPointSize, checkPointPtr)
    Lfs	*lfsPtr;		/* File system. */
    int	checkPointSize; /* Size of the checkpoint region. */
    char *checkPointPtr; /* The checkpoint region. */
{
    LfsStableMemCheckPoint *cpPtr;
    LfsStableMemParams  *smemParamsPtr;
   Boolean 		ret = FALSE;

    if (checkPointSize < sizeof(LfsDescMapCheckPoint)) {
	fprintf(stderr,"%s: Bad DescMap checkpoint size %d\n", 
			lfsPtr->deviceName, checkPointSize);
	return TRUE;
    }
    smemParamsPtr = &(lfsPtr->superBlock.descMap.stableMem);


    bcopy(checkPointPtr, (char *) &lfsPtr->descMap.checkPoint, 
		sizeof(LfsDescMapCheckPoint));


     cpPtr = (LfsStableMemCheckPoint *)
			(checkPointPtr + sizeof(LfsDescMapCheckPoint));

    lfsPtr->descMap.smemPtr =  LfsLoadStableMem(lfsPtr, smemParamsPtr, cpPtr);

    return ret;

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
LfsDescMapCheckpoint(segPtr, checkPointPtr, checkPointSizePtr)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    char   *checkPointPtr;      /* Buffer to write checkpoint data. */
    int	   *checkPointSizePtr;  /* Bytes added to the checkpoint area.*/
{

    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);
    LfsDescMapCheckPoint *cp = (LfsDescMapCheckPoint *) checkPointPtr;
    int		size, dataSize;
    Boolean	full;

    *cp = mapPtr->checkPoint;
    size = sizeof(LfsDescMapCheckPoint);
    dataSize = 0;
    full = LfsStableMemCheckpoint(segPtr, checkPointPtr + size, 
			&dataSize, segPtr->lfsPtr->descMap.smemPtr);
    if (!full) { 
	(*checkPointSizePtr) = dataSize + size;
    }
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

void
LfsDescMapWriteDone(segPtr, flags)
    LfsSeg *segPtr;		/* Segment containing data for checkpoint. */
    int	   flags;		/* Flags for checkpoint */
{
    Lfs		      *lfsPtr = segPtr->lfsPtr;
    LfsDescMap	      *mapPtr = &(lfsPtr->descMap);

    LfsStableMemWriteDone(segPtr, flags, mapPtr->smemPtr);
    return;

}


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
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status = SUCCESS;

    entryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FS_FILE_NOT_FOUND;
    } else { 
	*versionNumPtr = entryPtr->truncVersion;
    }
    return status;

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
    int  *diskAddrPtr; /* Current disk address.*/
{
    LfsDescMapEntry   *entryPtr;
    ReturnStatus      status = SUCCESS;


    entryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);

    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FS_FILE_NOT_FOUND;
    }
    *diskAddrPtr = entryPtr->blockAddress;
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
    int diskAddr; /* New disk address.*/
{
    LfsDescMapEntry   *entryPtr;
    ReturnStatus	status = SUCCESS;


    entryPtr = LfsGetDescMapEntry(lfsPtr, fileNumber);
    if (!(entryPtr->flags & LFS_DESC_MAP_ALLOCED)) {
	status = FAILURE;
    } else { 
	entryPtr->blockAddress = diskAddr;
	LfsDescMapEntryModified(lfsPtr, fileNumber);
    }
    return status;

}

