/* 
 * fsdmFileDesc.c --
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

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsconsist.h>
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fsStat.h>
#include <fsdm.h>

#include <stdio.h>

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_GetNewFileNumber() --
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
Fsdm_GetNewFileNumber(domainPtr, dirFileNum, fileNumberPtr)
    register Fsdm_Domain 	*domainPtr;	/* Domain to allocate the file 
					 * descriptor out of. */
    int			dirFileNum;	/* File number of the directory that
					   the file is in.  -1 means that
					   this file descriptor is being
					   allocated for a directory. */
    int			*fileNumberPtr; /* Place to return the number of
					   the file descriptor allocated. */
{
    ReturnStatus status;
    status = domainPtr->domainOpsPtr->getNewFileNumber(domainPtr, dirFileNum,
					fileNumberPtr);
#ifdef lint
    status = Lfs_GetNewFileNumber(domainPtr, dirFileNum, fileNumberPtr);
    status = Ofs_GetNewFileNumber(domainPtr, dirFileNum, fileNumberPtr);
#endif /* lint */
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FreeFileNumber() --
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
Fsdm_FreeFileNumber(domainPtr, fileNumber)
    register Fsdm_Domain 	*domainPtr;	/* Domain that the file 
					 * descriptor is in. */
    int			fileNumber; 	/* Number of file descriptor to 
					   free.*/
{
    ReturnStatus status;
    status = domainPtr->domainOpsPtr->freeFileNumber(domainPtr, fileNumber);
#ifdef lint
    status = Lfs_FreeFileNumber(domainPtr, fileNumber);
    status = Ofs_FreeFileNumber(domainPtr, fileNumber);
#endif /* lint */
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileDescInit() --
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
Fsdm_FileDescInit(domainPtr, fileNumber, type, permissions, uid, gid, 
			fileDescPtr)
    register Fsdm_Domain 	*domainPtr;	/* Domain of the file */
    int			fileNumber; 	/* Number of file descriptor */
    int			type;		/* Type of the file */
    int			permissions;	/* Permission bits for the file */
    int			uid;		/* Owner ID for the file */
    int			gid;		/* Group ID for the file */
    Fsdm_FileDescriptor	*fileDescPtr;	/* File descriptor structure to
					   initialize. */
{

    ReturnStatus status;
    status = domainPtr->domainOpsPtr->fileDescInit
	(domainPtr, fileNumber, type, permissions, uid, gid, fileDescPtr);
#ifdef lint
    status = Lfs_FileDescInit(domainPtr, fileNumber, type, permissions,
					uid, gid, fileDescPtr);
    status = Ofs_FileDescInit(domainPtr, fileNumber, type, permissions,
					uid, gid, fileDescPtr);
#endif /* lint */

    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileDescFetch() --
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
Fsdm_FileDescFetch(domainPtr, fileNumber, fileDescPtr)
    register Fsdm_Domain 	*domainPtr;	/* Domain to fetch the file 
					 * descriptor from. */
    register int	fileNumber; 	/* Number of file descriptor to 
					   fetch.*/
    Fsdm_FileDescriptor	*fileDescPtr;	/* File descriptor structure to
					   initialize. */
{
    ReturnStatus status;
    status = domainPtr->domainOpsPtr->fileDescFetch(domainPtr, fileNumber,
		fileDescPtr);
#ifdef lint
    status = Lfs_FileDescFetch(domainPtr, fileNumber, fileDescPtr);
    status = Ofs_FileDescFetch(domainPtr, fileNumber, fileDescPtr);
#endif /* lint */
     return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsdm_FileDescStore() --
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
Fsdm_FileDescStore(handlePtr, forceOut)
    Fsio_FileIOHandle	*handlePtr;
    Boolean		forceOut;  /* Force the change to disk. */
{
    register Fsdm_Domain 	*domainPtr;	/* Domain to store the file 
					 * descriptor into. */
    int			fileNumber; 	/* Number of file descriptor to 
					   store.*/
    Fsdm_FileDescriptor	*fileDescPtr;	
    register ReturnStatus   status;

    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, TRUE);
    fileNumber = handlePtr->hdr.fileID.minor;
    fileDescPtr = handlePtr->descPtr;
    if (fileNumber == 0) {
	Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
	panic( "Fsdm_FileDescStore: file #0\n");
	return(FAILURE);
    }
    status = domainPtr->domainOpsPtr->fileDescStore(domainPtr, handlePtr,
			fileNumber, fileDescPtr, forceOut);
#ifdef lint
    status = Lfs_FileDescStore(domainPtr, handlePtr, fileNumber, fileDescPtr,
					forceOut);
    status = Lfs_FileDescStore(domainPtr, handlePtr, fileNumber, fileDescPtr,
					forceOut);
#endif /* lint */
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsdm_UpdateDescAttr --
 *
 *	Update the attribute in the FileDesc from those in the cached 
 *	attributes.
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
Fsdm_UpdateDescAttr(handlePtr, attrPtr, dirtyFlags)
    register Fsio_FileIOHandle	*handlePtr;	/* Handle of file. */
    register Fscache_Attributes *attrPtr;	/* Cached attributes of file. */
    int		dirtyFlags;		  /* Bit FSDM_FD_ bits of attributes
					   * check. -1 means update all. */
{
    register Fsdm_FileDescriptor	*descPtr;
    register ReturnStatus     	status = SUCCESS;

    descPtr = handlePtr->descPtr;
    if (dirtyFlags == -1) { 
	/*
	 * If the handle times differ from the descriptor times then force
	 * them out to the descriptor.
	 */
	if (descPtr->accessTime < attrPtr->accessTime) {
	    descPtr->accessTime = attrPtr->accessTime;
	    descPtr->flags |= FSDM_FD_ACCESSTIME_DIRTY;
	}
	if (descPtr->dataModifyTime < attrPtr->modifyTime) {
	    descPtr->dataModifyTime = attrPtr->modifyTime;
	    descPtr->flags |= FSDM_FD_MODTIME_DIRTY;
	    if (descPtr->dataModifyTime > descPtr->descModifyTime) {
		descPtr->descModifyTime = descPtr->dataModifyTime;
		descPtr->flags |= FSDM_FD_OTHERS_DIRTY;
	    }
	}
	if (descPtr->dataModifyTime > descPtr->descModifyTime) {
	    descPtr->descModifyTime = descPtr->dataModifyTime;
	    descPtr->flags |= FSDM_FD_OTHERS_DIRTY;
	}
	if (descPtr->userType != attrPtr->userType) {
	    descPtr->userType = attrPtr->userType;
	    descPtr->flags |= FSDM_FD_USERTYPE_DIRTY;
	}
    } else if (dirtyFlags & FSDM_FD_ACCESSTIME_DIRTY) {
	if (descPtr->accessTime < attrPtr->accessTime) {
	    descPtr->accessTime = attrPtr->accessTime;
	    descPtr->flags |= FSDM_FD_ACCESSTIME_DIRTY;
	}
    } else if (dirtyFlags & FSDM_FD_MODTIME_DIRTY) {
	if (descPtr->dataModifyTime < attrPtr->modifyTime) {
	    descPtr->dataModifyTime = attrPtr->modifyTime;
	    descPtr->flags |= FSDM_FD_MODTIME_DIRTY;
	}
	if (descPtr->dataModifyTime > descPtr->descModifyTime) {
	    descPtr->descModifyTime = descPtr->dataModifyTime;
	    descPtr->flags |= FSDM_FD_OTHERS_DIRTY;
	}
    } else if (dirtyFlags & FSDM_FD_USERTYPE_DIRTY) {
	if (descPtr->userType != attrPtr->userType) {
	    descPtr->userType = attrPtr->userType;
	    descPtr->flags |= FSDM_FD_USERTYPE_DIRTY;
	}
   }
   if (descPtr->flags & FSDM_FD_DIRTY) {
	status =  Fsdm_FileDescStore(handlePtr, FALSE);
	if (status != SUCCESS) {
	    printf(
	    "Fsdm_UpdateDescAttr: Could not put desc <%d,%d> into cache\n",
		    handlePtr->hdr.fileID.major,
		    handlePtr->hdr.fileID.minor);
	}
    }
    return(status);
}
