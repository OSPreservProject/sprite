/* 
 * Db_WriteEntry.c --
 *
 *	Source code for the Db_WriteEntry procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/Db_WriteEntry.c,v 1.5 89/06/15 22:45:33 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include "dbInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Db_WriteEntry --
 *
 *	Write a buffer into a specified location in the shared database.
 *	This opens and locks the file, writes the data, and closes the
 *	file.  The lock is polled if necessary.  The file is opened with
 *	the default mode as defined above.  
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	0 indicates success.
 *
 * Side effects:
 *	The global database is modified.
 *
 *----------------------------------------------------------------------
 */

int
Db_WriteEntry(file, buffer, index, size, lockHow)
    char *file;
    char *buffer;
    int index;
    int size;
    Db_LockHow lockHow;
{
    int status;
    int offset;
    int streamID;
    int bytesWritten;
    Db_Handle handle;
    
#ifdef DEBUG_DB
    syslog(LOG_INFO, "Debug msg: Db_WriteEntry(%s) called.", file);
#endif    
    streamID = open(file, O_WRONLY | O_CREAT, FILE_MODE);
    if (streamID == -1) {
	syslog(LOG_ERR, "Db_WriteEntry: error opening file %s: %s.\n", file,
	       strerror(errno));
	return(streamID);
    }

    offset = index * size;
    status = lseek(streamID, (long) offset, L_SET);
    if (status == -1) {
	return(status);
    }
    /*
     * Fake a Db_Handle for DbLockDesc.
     */
    handle.streamID = streamID;
    handle.lockHow = lockHow;
    handle.lockType = LOCK_EX;
#ifndef CLEAN
    handle.fileName = file;
#endif /* CLEAN */
    status = DbLockDesc(&handle);
    if (status == -1) {
#ifdef DEBUG_DB
	syslog(LOG_INFO, "Debug msg: Db_WriteEntry(%s) returning %x.", file, status);
#endif    
	return(status);
    }
    
    bytesWritten = write(streamID, buffer, size);
    if (bytesWritten == -1) {
	status = -1;
    } else if (bytesWritten != size) {
	status = -1;
	errno = 0;
    } else {
	status = 0;
    }
    (void) flock(streamID, LOCK_EX | LOCK_UN);
    (void) close(streamID);

#ifdef DEBUG_DB
    syslog(LOG_INFO, "Debug msg: Db_WriteEntry(%s) returning %x.", file, status);
#endif    
    return(status);
}
