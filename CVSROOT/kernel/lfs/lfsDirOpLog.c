/* 
 * lfsDirOpLog.c --
 *
 *	Routines for LFS directory change log.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <fs.h>
#include <fsdm.h>
#include <lfsInt.h>


/*
 * FILL_IN_ENTRY - Fill in a new log entry.
 */
#define FILL_IN_ENTRY(entryPtr, opFlags) 				\
    (entryPtr)->hdr.opFlags = opFlags;					\
    (entryPtr)->hdr.dirFileNumber = dirFileNumber;			\
    (entryPtr)->hdr.dirOffset = dirOffset;				\
    (entryPtr)->hdr.linkCount = 					\
		(fileDescPtr == (Fsdm_FileDescriptor *) NIL) ?	0 :	\
				fileDescPtr->numLinks;			\
    (entryPtr)->dirEntry.fileNumber = fileNumber;			\
    (entryPtr)->dirEntry.recordLength = Fslcl_DirRecLength(nameLen);	\
    (entryPtr)->dirEntry.nameLength = nameLen;				\
    bcopy(name, (entryPtr)->dirEntry.fileName, nameLen);


/*
 *----------------------------------------------------------------------
 *
 * Lfs_DirOpStart --
 *
 *	Mark the start of a directory operation on an OFS file system.
 *
 * Results:
 *	NIL
 *	
 *
 * Side effects:
 *	Log record added. Will sleep until checkpoint operation done.
 *
 *----------------------------------------------------------------------
 */

ClientData
Lfs_DirOpStart(domainPtr, opFlags, name, nameLen, fileNumber, fileDescPtr,
		 dirFileNumber, dirOffset, dirFileDescPtr)
    Fsdm_Domain	*domainPtr;	/* Domain containing the object being modified.
				 */
    int		opFlags;	/* Operation code and flags. See fsdm.h for
				 * definitions. */
    char	*name;		/* Name of object being operated on. */
    int		nameLen;	/* Length in characters of name. */
    int		fileNumber;	/* File number of objecting being operated on.*/
    Fsdm_FileDescriptor *fileDescPtr; /* FileDescriptor object being operated on
				       * before operation starts. */
    int		dirFileNumber;	/* File number of directory containing object.*/
    int		dirOffset;	/* Byte offset into directory of the directory
				 * entry containing operation. */
    Fsdm_FileDescriptor *dirFileDescPtr; /* FileDescriptor of directory before
					  * operation starts. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    LfsDirOpLogEntry *entryPtr;
    ClientData	clientData;
    Boolean found;
    int	 entrySize;

#define	LOCKPTR	&lfsPtr->lock
    LOCK_MONITOR;

    while (lfsPtr->activeFlags & LFS_CHECKPOINT_ACTIVE) {
	Sync_Wait(&lfsPtr->checkPointWait, FALSE);
    }
    lfsPtr->dirModsActive++;
    UNLOCK_MONITOR;
#undef LOCKPTR
#define	LOCKPTR	&lfsPtr->logLock

    LOCK_MONITOR;
    entrySize = Fslcl_DirRecLength(nameLen) + sizeof(LfsDirOpLogEntryHdr);
    entryPtr = LfsDirLogEntryAlloc(lfsPtr, entrySize,  -1, &found);
    FILL_IN_ENTRY(entryPtr, opFlags);
    clientData = (ClientData) entryPtr->hdr.logSeqNum;
    UNLOCK_MONITOR;

    return clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * Lfs_DirOpEnd --
 *
 *	Mark the end of a directory operation on an LFS file system.
 *
 * Results:
 *	None
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Lfs_DirOpEnd(domainPtr, clientData, status, opFlags, name, nameLen, 
		fileNumber,  fileDescPtr, dirFileNumber, dirOffset,
		dirFileDescPtr)
    Fsdm_Domain	*domainPtr;	/* Domain containing the object modified. */
    ClientData	clientData;	/* ClientData as returned by DirOpStart. */
    ReturnStatus status;	/* Return status of the operation, SUCCESS if
				 * operation succeeded. FAILURE otherwise. */
    int		opFlags;	/* Operation code and flags. See fsdm.h for
				 * definitions. */
    char	*name;		/* Name of object being operated on. */
    int		nameLen;	/* Length in characters of name. */
    int		fileNumber;	/* File number of objecting being operated on.*/
    Fsdm_FileDescriptor *fileDescPtr; /* FileDescriptor object after
				      * operation.*/
    int		dirFileNumber;	/* File number of directory containing object.*/
    int		dirOffset;	/* Byte offset into directory of the directory
				 * entry containing operation. */
    Fsdm_FileDescriptor *dirFileDescPtr; /* FileDescriptor of directory after
					  * operation. */
{
    Lfs	*lfsPtr = LfsFromDomainPtr(domainPtr);
    int logSeqNum = (int) clientData;
    LfsDirOpLogEntry *entryPtr;
    int entrySize;
    Boolean found;


#define	LOCKPTR	&lfsPtr->logLock
    LOCK_MONITOR;
    entrySize = Fslcl_DirRecLength(nameLen) + sizeof(LfsDirOpLogEntryHdr);
    entryPtr = LfsDirLogEntryAlloc(lfsPtr, entrySize, logSeqNum, &found);
    if (!found) { 
	FILL_IN_ENTRY(entryPtr, opFlags);
    } else {
	entryPtr->hdr.opFlags |= opFlags;
	entryPtr->hdr.dirOffset = dirOffset;
	if (fileDescPtr != (Fsdm_FileDescriptor *) NIL) {
	    entryPtr->hdr.linkCount = fileDescPtr->numLinks;
	}
	entryPtr->dirEntry.fileNumber = fileNumber;
    }
    UNLOCK_MONITOR;
#undef LOCKPTR
#define	LOCKPTR	&lfsPtr->lock
    LOCK_MONITOR;
    lfsPtr->dirModsActive--;
    if (lfsPtr->dirModsActive == 0) {
	Sync_Broadcast(&lfsPtr->checkPointWait);
    }
    UNLOCK_MONITOR;

    return;
}

