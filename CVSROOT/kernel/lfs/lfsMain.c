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
#include <stdlib.h>
#include <string.h>


typedef struct CheckPointData {
    Lfs		*lfsPtr;	/* Lfs data structure of file system. */
    Boolean	interval;	/* Set to TRUE if the checkpoint should
				 * stop. */
} CheckPointData;

static void CheckpointCallBack _ARGS_((ClientData clientData, 
		Proc_CallInfo *callInfoPtr));

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
    lfsPtr->blockSizeShift = LfsLogBase2((unsigned)LfsBlockSize(lfsPtr));
    lfsPtr->checkpointIntervalPtr = (int *) NIL;
    Sync_LockInitDynamic(&(lfsPtr->lock), "LfsLock");
    lfsPtr->activeFlags = 0;
    lfsPtr->dirModsActive = 0;
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
    return SUCCESS;
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


