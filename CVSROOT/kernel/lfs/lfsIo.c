/* 
 * lfsSegIo.c --
 *
 *	Read and write bytes from LFS segments.
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
#include "dev.h"
#include "fs.h"
#include "devFsOpTable.h"


/*
 *----------------------------------------------------------------------
 *
 * LfsReadBytes --
 *
 *	Read bytes from an lfs disk.
 *
 * Results:
 *	Error status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsReadBytes(lfsPtr, diskAddress, numBytes, bufferPtr)
    Lfs		*lfsPtr;	/* Target file system. */
    int diskAddress;	/* Disk address to read from. */
    int		numBytes;	/* Number of bytes to read. */
    char	*bufferPtr;	/* Buffer to read into. */
{
    struct args { 
	Fs_IOParam	readParams;
	Fs_IOReply	reply;
    } args;
    ReturnStatus status;
    char	smallBuffer[DEV_BYTES_PER_SECTOR];


    bzero((char *)&args, sizeof(args));

    if (numBytes < DEV_BYTES_PER_SECTOR) { 
	args.readParams.buffer = smallBuffer;
	args.readParams.length = DEV_BYTES_PER_SECTOR;
    } else {
	args.readParams.buffer = bufferPtr;
	args.readParams.length = numBytes;
    }
    args.readParams.offset = diskAddress * DEV_BYTES_PER_SECTOR;
    args.readParams.flags = 
    args.readParams.procID = 0;
    args.readParams.familyID = 0;
    args.readParams.uid = 0;
    args.readParams.reserved = 0;
    status = (*devFsOpTable[DEV_TYPE_INDEX(lfsPtr->devicePtr->type)].read)
		(lfsPtr->devicePtr, &args.readParams, &args.reply);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "LfsReadBytes");
    }
    if (numBytes < DEV_BYTES_PER_SECTOR) {
	bcopy(smallBuffer, bufferPtr, numBytes);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * LfsWriteBytes --
 *
 *	Write bytes from an lfs disk.
 *
 * Results:
 *	Error status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
LfsWriteBytes(lfsPtr, diskAddress, numBytes, bufferPtr)
    Lfs		*lfsPtr;	/* Target file system. */
    int diskAddress;	/* Disk address to send data. */
    int		numBytes;	/* Number of bytes to write. */
    char	*bufferPtr;	/* Buffer to write into. */
{
    struct args { 
	Fs_IOParam	writeParams;
	Fs_IOReply	reply;
    } args;
    char	smallBuffer[DEV_BYTES_PER_SECTOR];
    ReturnStatus	status;


    bzero((char *)&args, sizeof(args));

    if (numBytes < DEV_BYTES_PER_SECTOR) { 
	args.writeParams.buffer = smallBuffer;
	args.writeParams.length = DEV_BYTES_PER_SECTOR;
	bzero(smallBuffer + numBytes, DEV_BYTES_PER_SECTOR-numBytes);
	bcopy(bufferPtr, smallBuffer, numBytes);
    } else {
	args.writeParams.buffer = bufferPtr;
	args.writeParams.length = numBytes;
    }
    args.writeParams.offset = diskAddress * DEV_BYTES_PER_SECTOR;
    args.writeParams.flags = 
    args.writeParams.procID = 0;
    args.writeParams.familyID = 0;
    args.writeParams.uid = 0;
    args.writeParams.reserved = 0;
    status = (*devFsOpTable[DEV_TYPE_INDEX(lfsPtr->devicePtr->type)].write)
		(lfsPtr->devicePtr, &args.writeParams, &args.reply);
    if (status != SUCCESS) {
	LfsError(lfsPtr, status, "LfsWriteBytes");
    }
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
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Lfs_ReallocBlock --
 *
 *	Allocate a new block on disk to replace the given block.  This is
 *	intended to be used by the cache when it can't write out a block
 *	because of a disk error.
 *
 * Results:
 * 	None
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Lfs_ReallocBlock(data, callInfoPtr)
    ClientData		data;			/* Block to move */
    Proc_CallInfo	*callInfoPtr;	
{
    panic("Lfs_ReallocBlock called.\n");
}
