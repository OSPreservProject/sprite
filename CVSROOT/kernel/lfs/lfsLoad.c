/* 
 * lfsLoad.c --
 *
 *	Code to handle the loading, checkpoint, and detaching of LFS file
 *	system.
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
#include "lfsInt.h"
#include "stdlib.h"
#include "rpc.h"	/* For rpc_SpriteID. */

#include "string.h"
#include "lfs.h"

static Fsdm_DomainOps lfsDomainOps = {
	Lfs_AttachDisk,
	Lfs_DetachDisk,
	Lfs_DomainWriteBack,
	Lfs_RereadSummaryInfo,
	Lfs_DomainInfo,
	Lfs_BlockAllocate,
	Lfs_GetNewFileNumber,
	Lfs_FreeFileNumber,
	Lfs_FileDescInit,
	Lfs_FileDescFetch,
	Lfs_FileDescStore,
	Lfs_FileBlockRead,
	Lfs_FileBlockWrite,
	Lfs_FileTrunc

};


static Fscache_BackendRoutines  lfsBackendRoutines = {
	    Fsdm_BlockAllocate,
	    Fsdm_FileBlockRead,
	    Fsdm_FileBlockWrite,
	    Lfs_ReallocBlock,
	    Lfs_StartWriteBack,

};


/*
 *----------------------------------------------------------------------
 *
 * LfsLoadFileSystem --
 *
 *	Load the checkpointed state of a file system and call the LFS
 *	module attach routines to initialize a file system.
 *
 * Results:
 *	SUCCESS if the file system was successfully loaded. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsLoadFileSystem(lfsPtr)
    Lfs		*lfsPtr;	/* File system to load checkpoint state. */
{
    ReturnStatus	status;
    LfsCheckPointHdr	checkPointHdr[2], *checkPointHdrPtr;
    LfsCheckPointTrailer *trailerPtr;
    int			choosenOne, maxSize;
    char		*checkPointPtr;
    int			checkPointSize;

    /*
     * Examine the two checkpoint areas to locate the checkpoint area with the
     * newest timestamp.
     */
    status = LfsReadBytes(lfsPtr, lfsPtr->superBlock.hdr.checkPointOffset[0],
			sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+0));
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't read checkpoint header #0");
	return status;
    }
    status = LfsReadBytes(lfsPtr, lfsPtr->superBlock.hdr.checkPointOffset[1],
			sizeof(LfsCheckPointHdr), (char *) (checkPointHdr+1));
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't read checkpoint header #1");
	return status;
    }
    choosenOne = (checkPointHdr[0].timestamp<checkPointHdr[1].timestamp) ? 1 : 0;

    /*
     * Read in the entire checkpoint region into a buffer.
     */
    maxSize = LfsBlocksToBytes(lfsPtr, 
				(lfsPtr->superBlock.hdr.maxCheckPointBlocks));
    checkPointPtr = malloc(maxSize);

    status = LfsReadBytes(lfsPtr,
			lfsPtr->superBlock.hdr.checkPointOffset[choosenOne],
			 maxSize, checkPointPtr);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "Can't read checkpoint region");
	return status;
    }
    /*
     * Verify checksum checkpoint region here.
     */
    checkPointHdrPtr = (LfsCheckPointHdr *) checkPointPtr;
    trailerPtr = (LfsCheckPointTrailer *) 
		(checkPointPtr + checkPointHdrPtr->size - 
				sizeof(LfsCheckPointTrailer));

    if (checkPointHdrPtr->timestamp != trailerPtr->timestamp) {
	LfsError(lfsPtr, SUCCESS, "Bad checkpoint timestamps");
    }
    printf("LfsLoad using checkpoint area %d with timestamp %d\n",
	choosenOne, checkPointHdrPtr->timestamp);

    /*
     * Install the domain if we can. 
     */
    status = Fsdm_InstallDomain(checkPointHdrPtr->domainNumber, 
				checkPointHdrPtr->serverID, lfsPtr->name, 
				&(lfsPtr->domainPtr));
    if (status != SUCCESS) {
	free((char *) checkPointHdrPtr);
	return (status);
    }
    lfsPtr->domainPtr->backendPtr = 
	Fscache_RegisterBackend(&lfsBackendRoutines,(ClientData) lfsPtr, 0);
    lfsPtr->domainPtr->domainOpsPtr = &lfsDomainOps;
    lfsPtr->domainPtr->clientData = (ClientData) lfsPtr;

    checkPointPtr = checkPointPtr + sizeof(LfsCheckPointHdr);
    checkPointSize =  ((char *)trailerPtr) - checkPointPtr;
    /*
     * Process the checkpoint for each region by  calling segment attach
     * procedures for the modules doing segment I/O.
     */

    status = LfsSegAttach(lfsPtr, checkPointPtr, checkPointSize);

    if (status != SUCCESS) {
	free((char *) checkPointHdrPtr);
	return status;
    }

    /*
     * Setup checkPoint data structure for next checkpoint operation. We use
     * the buffer we allocated and set the nextRegion to be the one we
     * didn't load from. Also set the timestamp into the future.
     */
    lfsPtr->checkPoint.timestamp = checkPointHdrPtr->timestamp+1;
    lfsPtr->checkPoint.buffer = (char *) checkPointHdrPtr;
    lfsPtr->checkPoint.nextArea = !choosenOne;

    /*
     * Fill in the checkPointHdrPtr will the fields that don't change
     * between checkpoints.
     */
    checkPointHdrPtr->timestamp = lfsPtr->checkPoint.timestamp;
    checkPointHdrPtr->size = 0;
    checkPointHdrPtr->version = 1;
    bzero(checkPointHdrPtr->domainPrefix,
		sizeof(checkPointHdrPtr->domainPrefix));
    (void)strncpy(checkPointHdrPtr->domainPrefix, lfsPtr->name, 
			sizeof(checkPointHdrPtr->domainPrefix)-1);
    checkPointHdrPtr->domainNumber = lfsPtr->domainPtr->domainNumber;
    checkPointHdrPtr->attachSeconds = fsutil_TimeInSeconds;
    checkPointHdrPtr->detachSeconds = fsutil_TimeInSeconds;
    checkPointHdrPtr->serverID = rpc_SpriteID;

    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsCheckPointFileSystem --
 *
 *	Checkpoint the state of a file system by calling the LFS
 *	module attach routines.
 *
 * Results:
 *	SUCCESS if the file system was successfully checkpointed. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsCheckPointFileSystem(lfsPtr, flags)
    Lfs		*lfsPtr;	/* File system to be checkpointed. */
    int flags;		/* Flags for checkpoint. */
{
    LfsCheckPointHdr	*checkPointHdrPtr;
    int			size, blocks;
    LfsCheckPointTrailer *trailerPtr;
    ReturnStatus	status;

    checkPointHdrPtr = (LfsCheckPointHdr *) lfsPtr->checkPoint.buffer;

    status = LfsSegCheckPoint(lfsPtr, flags, 
			(char *)(checkPointHdrPtr+1), &size);
    if (status != SUCCESS) {
	return status;
    }
    /*
     * Fill in check point header and trailer.
     */
    checkPointHdrPtr->timestamp = LfsGetCurrentTimestamp(lfsPtr);
    checkPointHdrPtr->size = size + sizeof(LfsCheckPointHdr) + 
			     sizeof(LfsCheckPointTrailer);
    checkPointHdrPtr->version = 1;
    checkPointHdrPtr->detachSeconds = fsutil_TimeInSeconds;
    trailerPtr = (LfsCheckPointTrailer *) 
		(lfsPtr->checkPoint.buffer + size + sizeof(LfsCheckPointHdr));
    trailerPtr->timestamp = checkPointHdrPtr->timestamp;
    trailerPtr->checkSum = 0;

    blocks = LfsBytesToBlocks(lfsPtr, (checkPointHdrPtr->size+
			lfsPtr->superBlock.hdr.blockSize-1));
    status = LfsWriteBytes(lfsPtr,
	lfsPtr->superBlock.hdr.checkPointOffset[lfsPtr->checkPoint.nextArea],
	LfsBlocksToBytes(lfsPtr, blocks), (char *) checkPointHdrPtr);
    if (status != SUCCESS) {
	return status;
    }
    /*
     * Set the file system up to use the other checkpoint buffer next time.
     */
    lfsPtr->checkPoint.nextArea = !lfsPtr->checkPoint.nextArea;
    return SUCCESS;
}
