/* 
 * lfsMain.c --
 *
 *	Routines for attaching and detaching LFS file systems.
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
#include <stdlib.h>
#include <fsioDevice.h>
#include <fsdm.h>
#include <proc.h>
#include <string.h>
#include <fsCmd.h>


typedef struct CheckPointData {
    Lfs		*lfsPtr;	/* Lfs data structure of file system. */
    Boolean	interval;	/* Set to TRUE if the checkpoint should
				 * stop. */
} CheckPointData;

static void CheckpointCallBack _ARGS_((ClientData clientData, 
		Proc_CallInfo *callInfoPtr));
static ReturnStatus GetDomainFromCmdArgs _ARGS_((int *bufSizePtr, 
			char **bufferPtr, Fsdm_Domain **domainPtrPtr));


/*
 *----------------------------------------------------------------------
 *
 * Lfs_Init --
 *
 *	Initialized the modules of LFS.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *----------------------------------------------------------------------
 */
void
Lfs_Init() 
{
    LfsFileLayoutInit();
    LfsDescMapInit();
    LfsSegUsageInit();

    Fsdm_RegisterDiskManager("LFS", Lfs_AttachDisk);
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_AttachDomain --
 *
 *	Attach a LFS file system from the specified device. 
 *
 * Results:
 *	SUCCESS if device is attached, FAILURE otherwise.
 *
 * Side effects:
 *	LFS modules initialied if this is the first LFS attached.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_AttachDisk(devicePtr, localName, flags, domainNumPtr)
    Fs_Device	*devicePtr;	/* Device containing file system. */
    char *localName;		/* The local prefix for the domain */
    int  flags;			/* Attach flags. */
    int *domainNumPtr;		/* OUT: Domain number allocated. */
{
    Lfs			*lfsPtr;
    LfsDiskAddr		diskAddr;
    ReturnStatus	status;
    CheckPointData 	*cpDataPtr;
    /*
     * Allocate space for the Lfs data structure fill in the fields need
     * by the rest of the Lfs module to perform initialization correctly.
     */
    lfsPtr = (Lfs *) malloc(sizeof(*lfsPtr));
    bzero((char *)lfsPtr, sizeof(*lfsPtr));
    lfsPtr->devicePtr = devicePtr;
    lfsPtr->name = localName;
    lfsPtr->name = malloc(strlen(localName)+1);
    lfsPtr->controlFlags = 0;
    (void) strcpy(lfsPtr->name, localName);
    lfsPtr->attachFlags = flags;

    /*
     * Read the super block of the file system. Put a lot of trust in the
     * magic number. 
     */
    LfsOffsetToDiskAddr(LFS_SUPER_BLOCK_OFFSET, &diskAddr);
    status = LfsReadBytes(lfsPtr, diskAddr,  LFS_SUPER_BLOCK_SIZE,
		 (char *) &(lfsPtr->superBlock));
    if (status != SUCCESS) {
	free((char *) lfsPtr);
	return status;
    }
    /*
     * Validate the super block here.
     */
    if ((lfsPtr->superBlock.hdr.magic != LFS_SUPER_BLOCK_MAGIC) ||
	(lfsPtr->superBlock.hdr.version != LFS_SUPER_BLOCK_VERSION)) {
	free((char *) lfsPtr);
	return FAILURE;
    }
    /*
     * Check to make sure that the partition that the file system was
     * built in matches the partition it is to be loaded from. To be
     * backwards compatible we check the domainUID. If it's zero then
     * the partition number isn't valid either. This check should go away
     * once all lfs systems are upgraded to the new version of the 
     * superblock.
     */
    if (lfsPtr->superBlock.hdr.domainUID != 0) {
	if (lfsPtr->superBlock.hdr.partition != 
	    devicePtr->unit % FSDM_NUM_DISK_PARTS) {

	    /* 
	     * File system was built on a different partition.
	     */
	    free((char *) lfsPtr);
	    return FAILURE;
	}
    }
    lfsPtr->blockSizeShift = LfsLogBase2((unsigned)LfsBlockSize(lfsPtr));
    lfsPtr->checkpointIntervalPtr = (int *) NIL;
    Sync_LockInitDynamic(&(lfsPtr->lock), "LfsLock");
    lfsPtr->activeFlags = 0;
    lfsPtr->cleanerProcPtr = (Proc_ControlBlock *) NIL;
    lfsPtr->dirModsActive = 0;
    lfsPtr->numDirtyBlocks = 0;
    status = LfsLoadFileSystem(lfsPtr, flags); 
    if (status != SUCCESS) { 
	free((char *)lfsPtr);
	return status;
    }
    LfsMemInit(lfsPtr);
    *domainNumPtr = lfsPtr->domainPtr->domainNumber;
    /*
     * Make our own copy of the prefix name.
     */
    lfsPtr->name = malloc(strlen(localName)+1);
    (void) strcpy(lfsPtr->name, localName);
    cpDataPtr = (CheckPointData *) malloc(sizeof(*cpDataPtr));
    cpDataPtr->lfsPtr = lfsPtr;
    cpDataPtr->interval = lfsPtr->superBlock.hdr.checkpointInterval *
				timer_IntOneSecond;

    lfsPtr->checkpointIntervalPtr = &(cpDataPtr->interval);

    Proc_CallFunc(CheckpointCallBack, (ClientData) cpDataPtr,
			cpDataPtr->interval);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_DetachDomain --
 *
 *	Detach a LFS file system domain.
 *
 * Results:
 *	SUCCESS if device is detached, FAILURE otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Lfs_DetachDisk(domainPtr)
     Fsdm_Domain *domainPtr;	/* Domain to detach. */

{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    ReturnStatus status;

    status = LfsDetachFileSystem(lfsPtr);
    LfsMemDetach(lfsPtr);
    Fscache_UnregisterBackend(lfsPtr->domainPtr->backendPtr);
    lfsPtr->domainPtr->backendPtr = (Fscache_Backend *) NIL;
    Sync_LockClear(&lfsPtr->lock);
    free(lfsPtr->name);
    free((char *)lfsPtr);
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_RereadSummaryInfo --
 *
 *	Reread the summary sector associated with the prefix and update
 *	the domain information. This should be called if the summary
 *	sector on the disk has been changed since the domain was attached.
 *	LFS uses this call to reread the superBlock and get any changes made.
 *
 * Results:
 *	SUCCESS 
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Lfs_RereadSummaryInfo(domainPtr)
    Fsdm_Domain		*domainPtr;	/* Domain to reread summary for. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    LfsDiskAddr		diskAddr;
    ReturnStatus status;
    LfsSuperBlock	newSuperBlock;
    /*
     * Read the super block of the file system. Put a lot of trust in the
     * magic number. 
     */
    LfsOffsetToDiskAddr(LFS_SUPER_BLOCK_OFFSET, &diskAddr);
    status = LfsReadBytes(lfsPtr, diskAddr,  LFS_SUPER_BLOCK_SIZE,
		 (char *) &newSuperBlock);
    if (status != SUCCESS) {
	return status;
    }
    /*
     * Validate the super block here.
     */
    if ((newSuperBlock.hdr.magic != LFS_SUPER_BLOCK_MAGIC) ||
	(newSuperBlock.hdr.version != LFS_SUPER_BLOCK_VERSION)) {
	return FAILURE;
    }
    /*
     * Looks ok, update our incore version.
     */
    bcopy((char *) &newSuperBlock, (char *) &lfsPtr->superBlock,
		sizeof(LfsSuperBlock));
    return SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Lfs_DomainWriteBack --
 *
 *	Force all domain information to disk.
 *
 * Results:
 *	Error code if the write failed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Lfs_DomainWriteBack(domainPtr, shutdown)
    Fsdm_Domain	*domainPtr;	/* Domain to be written back. */
    Boolean	shutdown;	/* TRUE if are syncing to shutdown the system.*/
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);

    return LfsCheckPointFileSystem(lfsPtr, LFS_CHECKPOINT_WRITEBACK);

}

/*
 *----------------------------------------------------------------------
 *
 * GetDomainFromCmdArgs --
 *
 *	Return the Fsdm_Domain specified in the Lfs_Command arguments.
 *	This routine updates the bufSize and bufferPtr to remove the
 *	domain specifier argument.
 *
 * Results:
 *	SUCCESS if domain fetched.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


static ReturnStatus
GetDomainFromCmdArgs(bufSizePtr, bufferPtr, domainPtrPtr)
    int	*bufSizePtr;	/* Size of Lfs_Command argument buffer. */
    char **bufferPtr;	/* Argument buffer. */
    Fsdm_Domain	**domainPtrPtr; /* OUT: Lfs Domain specified by arguments. */
{
    int		bufSize = (*bufSizePtr);
    char	*buffer = (*bufferPtr);
    int		domainNumber;
    Fsdm_Domain	*domainPtr;

    if (bufSize < sizeof(int)) {
	return GEN_INVALID_ARG;
    }
    bcopy(buffer, (char *) &domainNumber, sizeof(int));
    domainPtr = Fsdm_DomainFetch(domainNumber, FALSE);
    if (domainPtr == (Fsdm_Domain *) NIL) {
	return GEN_INVALID_ARG;
    }
    if (domainPtr->domainOpsPtr->attachDisk != Lfs_AttachDisk) {
	Fsdm_DomainRelease(domainNumber);
	return GEN_INVALID_ARG;
    }
    (*domainPtrPtr) = domainPtr;
    (*bufSizePtr) -= sizeof(int);
    (*bufferPtr) += sizeof(int);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_Command --
 *
 *	Perform a user specified command on a LFS file system
 *
 * Results:
 *	SUCCESS if the operation succeeded. An ReturnStatus otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


ReturnStatus
Lfs_Command(command, bufSize, buffer)
    int command;	/* Command to perform. */
    int bufSize;	/* Size of the user's input/output buffer. */
    Address buffer;	/* The user's input or output buffer. */
{
    Lfs	*lfsPtr;
    ReturnStatus status;
    Fsdm_Domain	*domainPtr;
    char	*outBufferPtr = buffer;

    switch (command) {
	case	FS_CLEAN_LFS_COMMAND: {
	    status = GetDomainFromCmdArgs(&bufSize, &buffer, &domainPtr);
	    if (status != SUCCESS) {
		return status;
	    }
	    lfsPtr = (Lfs *) domainPtr->clientData;
	    LfsSegCleanStart(lfsPtr);
	    Fsdm_DomainRelease(domainPtr->domainNumber);
	    break;
	}
	case FS_SET_CONTROL_FLAGS_LFS_COMMAND: {
	    status = GetDomainFromCmdArgs(&bufSize, &buffer, &domainPtr);
	    if (status != SUCCESS) {
		return status;
	    }
	    lfsPtr = (Lfs *) domainPtr->clientData;
	    if (bufSize >= sizeof(int)) {
		bcopy(buffer, (char *) &(lfsPtr->controlFlags), sizeof(int));
	    } else {
		status = GEN_INVALID_ARG;
	    }
	    Fsdm_DomainRelease(domainPtr->domainNumber);
	    break;
	}
	case FS_GET_CONTROL_FLAGS_LFS_COMMAND: {
	    status = GetDomainFromCmdArgs(&bufSize, &buffer, &domainPtr);
	    if (status != SUCCESS) {
		return status;
	    }
	    lfsPtr = (Lfs *) domainPtr->clientData;
	    if (bufSize >= sizeof(int)) {
		bcopy((char *) &(lfsPtr->controlFlags), outBufferPtr, 
				sizeof(int));
	    } else {
		status = GEN_INVALID_ARG;
	    }
	    Fsdm_DomainRelease(domainPtr->domainNumber);
	    break;
	}
	case FS_FREE_FILE_NUMBER_LFS_COMMAND: {
	    int	fileNumber;
	    status = GetDomainFromCmdArgs(&bufSize, &buffer, &domainPtr);
	    if (status != SUCCESS) {
		return status;
	    }
	    lfsPtr = (Lfs *) domainPtr->clientData;
	    if (bufSize >= sizeof(int)) {
		bcopy(buffer, (char *) &fileNumber, sizeof(int));
		printf("Lfs_FreeFileNumber(%s,%d)\n", lfsPtr->name, 
				fileNumber);
		status = Lfs_FreeFileNumber(domainPtr, fileNumber);
	    } else {
		status = GEN_INVALID_ARG;
	    }
	    Fsdm_DomainRelease(domainPtr->domainNumber);
	    return status;
	}
	case FS_ADJUST_SEG_USAGE_LFS_COMMAND: {
	    int	segNumber, activeBytes;
	    status = GetDomainFromCmdArgs(&bufSize, &buffer, &domainPtr);
	    if (status != SUCCESS) {
		return status;
	    }
	    lfsPtr = (Lfs *) domainPtr->clientData;
	    if (bufSize >= sizeof(int)*2) {
		bcopy(buffer, (char *) &segNumber, sizeof(int));
		bcopy(buffer+sizeof(int), (char *) &activeBytes, sizeof(int));
		printf("LfsSetSegUsage(%s,%d,%d)\n", lfsPtr->name, 
				segNumber, activeBytes);
		LfsSetSegUsage(lfsPtr, segNumber, activeBytes);
	    } else {
		status = GEN_INVALID_ARG;
	    }

	    return status;
	}
	case FS_ZERO_ASPLOS_STATS_LFS_COMMAND: {/* For ASPLOS only.  Remove when
					        * that's done. -Mary 2/15/92. */
	    /*
	     * Zero the stats used for asplos measurements.  We've gotta
	     * run this once per file system  before removing the stats so that
	     * the checkpoint copies of the padding bytes will be zeroed again.
	     */
	    status = GetDomainFromCmdArgs(&bufSize, &buffer, &domainPtr);
	    if (status != SUCCESS) {
		return status;
	    }
	    lfsPtr = (Lfs *) domainPtr->clientData;
	    bzero((char *) &(lfsPtr->stats.log.fsyncWrites),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.fsyncPartialWrites),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.fsyncBytes),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.fsyncPartialBytes),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.partialWriteBytes),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.cleanPartialWriteBytes),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.fileBytesWritten),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.cleanFileBytesWritten),
		    sizeof(LfsStatsCounter));
	    bzero((char *) &(lfsPtr->stats.log.partialFileBytes),
		    sizeof(LfsStatsCounter));

	    bzero((char *) &(lfsPtr->stats.layout.descLayoutBytes),
		    sizeof(LfsStatsCounter));
	    
	    bzero((char *) &(lfsPtr->stats.dirlog.cleaningBytesWritten),
		    sizeof(LfsStatsCounter));

	    break;
	}
	case FS_TOGGLE_ASPLOS_STATS_LFS_COMMAND: {/* Just for ASPLOS. */
	    /*
	     * Toggle on or off the taking of Lfs ASPLOS stats. The last time
	     * these stats are taken, they must be turned off and then
	     * zeroed with the above command.
	     */
	    Lfs_DoASPLOSStats = 1 - Lfs_DoASPLOSStats;
	    status = SUCCESS;
	    break;
	}
	default: {
	    status = GEN_INVALID_ARG;
	}
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckpointCallBack --
 *
 *	A Proc_CallFunc for checkpoint LFS file systems.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
CheckpointCallBack(clientData, callInfoPtr)
    ClientData       clientData;		/* Lfs structure of LFS */
    Proc_CallInfo	*callInfoPtr;
{

    CheckPointData *cpDataPtr = (CheckPointData *) clientData;
    Lfs	*lfsPtr = cpDataPtr->lfsPtr;

    if (cpDataPtr->interval > 0) {
	(void) LfsCheckPointFileSystem(lfsPtr, LFS_CHECKPOINT_TIMER);
	callInfoPtr->interval = cpDataPtr->interval;
    } else if (cpDataPtr->interval == 0) {
	free((char *) cpDataPtr);
	callInfoPtr->interval = 0;
    } else {
	callInfoPtr->interval = -cpDataPtr->interval;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * LfsLogBase2 --
 *
 *	Compute the log base 2 of the given integer. The argument is assumed
 *	to have only one bit set in it.
 *
 * Results:
 *	Log base 2 of the integer.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

int
LfsLogBase2(val)
    unsigned	int	val;	/* Value of take log of. */
{
    register int	 bit;

    for (bit = 0; bit < sizeof(val)*8; bit++) {
	if (val & (1 << bit)) {
		break;
	}
    }
    if (val != (1 << bit)) {
	panic("LfsLogBase2: Botched computation of %d\n", val);
    }
    return bit;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsError --
 *
 *	Print an error message.
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
LfsError(lfsPtr, status, message)
    Lfs	*lfsPtr;
    ReturnStatus status;
    char *message;
{
    panic("LfsError: on %s status 0x%x, %s\n", lfsPtr->name, status, message);
}


