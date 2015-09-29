/* 
 * Db_Open.c --
 *
 *	Source code for the Db_Open procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/Db_Open.c,v 1.6 89/06/15 22:45:11 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include <errno.h>
#include "dbInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Db_Open --
 *
 *	Open a system data file and return a handle to it.
 *
 * Results:
 *	The handle is returned in *handlePtr.
 *	On error, -1 is returned, otherwise 0 is returned.
 *
 * Side effects:
 *	The file is opened and (optionally) locked.
 *
 *----------------------------------------------------------------------
 */

int
Db_Open(file, size, handlePtr, writing, lockWhen, lockHow, numBuf)
    char *file;
    int size;
    Db_Handle *handlePtr;
    int writing;	 	/* 1 if opening for writing */
    Db_LockWhen lockWhen;
    Db_LockHow lockHow;
    int numBuf;			/* number of records to buffer */
{
    int streamID;
    
    streamID = open(file, (writing ? O_RDWR : O_RDONLY) | O_CREAT, FILE_MODE);
    if (streamID == -1) {
	syslog(LOG_ERR, "Db_Open: error opening file %s: %s.\n", file,
	       strerror(errno));
	return(-1);
    }

    /*
     * Don't allow buffering for files locked a record at a time.
     */
    if (numBuf > 0 &&
	(lockWhen == DB_LOCK_ACCESS || lockWhen == DB_LOCK_READ_MOD_WRITE)) {
	errno = EINVAL;
	return(-1);
    }
    
    handlePtr->index = 0;
    handlePtr->structSize = size;
    handlePtr->lockType = writing ? LOCK_EX : LOCK_SH;
    handlePtr->lockWhen = lockWhen;
    handlePtr->lockHow = lockHow;
    handlePtr->streamID = streamID;
    handlePtr->firstRec = -1;
    handlePtr->numBuf = numBuf;
    if (numBuf > 0) {
	handlePtr->buffer = malloc((unsigned) numBuf * size);
    } else {
	handlePtr->buffer = (char *) NULL;
    }
#ifndef CLEAN
    handlePtr->fileName = malloc((unsigned) strlen(file) + 1);
    (void) strcpy(handlePtr->fileName, file);
#endif /* CLEAN */
    

    if (lockWhen == DB_LOCK_OPEN) {
	return(DbLockDesc(handlePtr));
    }
    return(0);
}
