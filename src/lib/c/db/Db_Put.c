/* 
 * Db_Put.c --
 *
 *	Source code for the Db_Put procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/Db_Put.c,v 1.5 89/01/13 11:44:28 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include "dbInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Db_Put --
 *
 *	Write a random entry, or the next entry in order, given the handle
 *	for a database.
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	0 indicates success.
 *
 * Side effects:
 *	The position in the file is updated, and data are written to the file.
 *
 *----------------------------------------------------------------------
 */

int
Db_Put(handlePtr, bufPtr, index)
    Db_Handle *handlePtr;
    char *bufPtr;
    int index;				/* -1 to indicate next in order */
{
    register int streamID;
    int bufSize;
    int offset;
    int bytesWritten;
    int status;

    streamID = handlePtr->streamID;
    bufSize = handlePtr->structSize;
    if (handlePtr->lockWhen == DB_LOCK_ACCESS) {
	status = DbLockDesc(handlePtr);
	if (status == -1) {
	    return(status);
	}
    }
    if (index == -1) {
	index = handlePtr->index;
    } else {
	offset = index * bufSize;
	status = lseek(streamID, (long) offset, L_SET);
	if (status == -1) {
	    return(status);
	}
    }
    bytesWritten = write(streamID, bufPtr, bufSize);
    if (bytesWritten == -1) {
	status = -1;
    } else if (bytesWritten != bufSize) {
	status = -1;
	errno = 0;
    } else {
	status = 0;
    }
    handlePtr->index = index + 1;
    if (handlePtr->lockWhen == DB_LOCK_ACCESS ||
	handlePtr->lockWhen == DB_LOCK_READ_MOD_WRITE) {
	(void) flock(streamID, LOCK_EX | LOCK_UN);
    }
    return(status);
}
