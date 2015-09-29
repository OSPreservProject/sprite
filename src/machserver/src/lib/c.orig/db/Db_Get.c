/* 
 * Db_Get.c --
 *
 *	Source code for the Db_Get procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/Db_Get.c,v 1.5 89/01/13 11:44:23 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include "dbInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Db_Get --
 *
 *	Obtain a random entry, or the next entry in order, given the handle
 *	for a database.
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	0 indicates success.
 *
 * Side effects:
 *	The position in the file is updated.
 *
 *----------------------------------------------------------------------
 */

int
Db_Get(handlePtr, bufPtr, index)
    Db_Handle *handlePtr;	/* handle to the database */
    char *bufPtr;		/* place to store record */
    int index; 			/* -1 to indicate next in order, else which
				 * record to obtain */
{
    register int streamID;
    int bufSize;
    int offset;
    int bytesRead;
    int status;
    int structSize = handlePtr->structSize;
    char *buffer;
    int doSeek;
    register int firstRec;

    if (index == -1) {
	index = handlePtr->index;
	doSeek = 0;
    } else if (index != handlePtr->index) {
	doSeek = 1;
    }
    if (handlePtr->numBuf > 0) {
	firstRec = handlePtr->firstRec;
	if ((firstRec >= 0) && (index >= firstRec) &&
	    (index < firstRec + handlePtr->numBuf)) {
	    bcopy(handlePtr->buffer + (index - firstRec) *
		  	structSize,
		  bufPtr, structSize);
	    handlePtr->index = index + 1;
	    return(0);
	}
	buffer = handlePtr->buffer;
	bufSize = handlePtr->numBuf * structSize;
    } else {
	buffer = bufPtr;
	bufSize = structSize;
    }
    
    streamID = handlePtr->streamID;
    if (handlePtr->lockWhen == DB_LOCK_ACCESS || 
	handlePtr->lockWhen == DB_LOCK_READ_MOD_WRITE) {
	status = DbLockDesc(handlePtr);
	if (status == -1) {
	    return(status);
	}
    }
    if (doSeek) {
	offset = index * structSize;
	status = lseek(streamID, (long) offset, L_SET);
	if (status == -1) {
	    return(status);
	}
    }
    bytesRead = read(streamID, buffer, bufSize);
    if (bytesRead == -1) {
	status = -1;
    } else if (bytesRead == 0) {
	/*
	 * end of file.
	 */
	status = -1;
	errno = 0;
    } else if (bytesRead % structSize != 0) {
#ifndef CLEAN
	syslog(LOG_ERR, "Db_Get: file %s is not a multiple of %d bytes",
	       handlePtr->fileName, structSize);
#endif /* CLEAN */
	status = -1;
	errno = EACCES;
    } else {
	status = 0;
	if (handlePtr->numBuf > 0) {
	    handlePtr->firstRec = index;
	    handlePtr->numBuf = bytesRead / structSize;
	    bcopy(handlePtr->buffer, bufPtr, structSize);
	}
	    
    }
    
    handlePtr->index = index + 1;

    /*
     * Try to unlock the file if we locked it before (and it's not a
     * read-modify-write) , but ignore any
     * errors from the unlock.  It's possible to get an RPC timeout doing
     * the lock but keep going, then get an error because the lock was
     * never performed.
     */
    if (handlePtr->lockWhen == DB_LOCK_ACCESS) {
	(void) flock(streamID, handlePtr->lockType | LOCK_UN);
    }
    return(status);
}
