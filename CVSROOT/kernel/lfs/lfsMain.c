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

#include "sprite.h"
#include "lfs.h"
#include "lfsInt.h"
#include "stdlib.h"

#include "fsdm.h"


/*
 *----------------------------------------------------------------------
 *
 * Init --
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
static void
Init() 
{
    LfsFileLayoutInit();
    LfsDescMapInit();
    LfsSegUsageInit();
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
Lfs_AttachDomain(devicePtr, name, flags, domainPtr)
     Fs_Device *devicePtr;	/* Device containing file system. */
     char      *name;		/* Prefix we are trying to attach. */
     int 	flags;		/* FS_ATTACH_READ_ONLY or FS_ATTACH_LOCAL */
     Fsdm_Domain *domainPtr;	/* Domain structure for file system being 
				 * attached. */

{
    Lfs			*lfsPtr;
    ReturnStatus	status;
    static Boolean initialized = FALSE;   

    /*
     * If this is the first time we are called initialize everything.
     */
    if (!initialized) {
	Init();
    }
    /*
     * Allocate space for the Lfs data structure fill in the fields need
     * by the rest of the Lfs module to perform initialization correctly.
     */
    lfsPtr = (Lfs *) malloc(sizeof(*lfsPtr));
    bzero((char *)lfsPtr, sizeof(*lfsPtr));
    lfsPtr->devicePtr = devicePtr;
    lfsPtr->name = name;
    lfsPtr->domainPtr = domainPtr;
    lfsPtr->readOnly = ((flags & FS_ATTACH_READ_ONLY) != 0);
    lfsPtr->dirty = FALSE;

    /*
     * Read the super block of the file system. Put a lot of trust in the
     * magic number. 
     */

    status = LfsReadBytes(lfsPtr, LFS_SUPER_BLOCK_OFFSET,  
		LFS_SUPER_BLOCK_SIZE, (char *) &(lfsPtr->superBlock));
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
    lfsPtr->blockSizeShift = LfsLogBase2(lfsPtr->superBlock.hdr.blockSize);
    lfsPtr->writeActive = FALSE;
    status = LfsLoadFileSystem(lfsPtr); 
    if (status != SUCCESS) {
	free((char *)lfsPtr);
    } else {
	((Lfs *) ((domainPtr)->dataBlockBitmap)) = lfsPtr;
    }
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
Lfs_DomainWriteBack(domainPtr, shutdown, detach)
    Fsdm_Domain	*domainPtr;	/* Domain to be written back. */
    Boolean	shutdown;	/* TRUE if are syncing to shutdown the system.*/
    Boolean	detach;		/* TRUE if are writing back as part of 
				 * detaching the domain.  */
{
    Lfs	*lfsPtr;
    int	flags;
    ReturnStatus status = SUCCESS;

    lfsPtr = LfsFromDomainPtr(domainPtr);
    flags = 0;
    if (shutdown) {
	flags |= LFS_DETACH;
    }
    if (lfsPtr->dirty) { 
	status = LfsCheckPointFileSystem(lfsPtr, flags);
    }
    if (!shutdown && (lfsPtr->usageArray.checkPoint.numDirty > 10)) {
	LfsSegCleanStart(lfsPtr);
    }
    return status;

}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_DomainStartWrite --
 *
 *	Start a write..
 *
 * Results:
 *	Error code if the write failed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Lfs_DomainStartWrite(domainPtr)
    Fsdm_Domain	*domainPtr;	/* Domain to be written back. */
{
    LfsSegWriteStart(LfsFromDomainPtr(domainPtr));
    return;

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
{
    panic("LfsError: 0x%x, %s\n", status, message);
}

