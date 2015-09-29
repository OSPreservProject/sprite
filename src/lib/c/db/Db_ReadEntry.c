/* 
 * Db_ReadEntry.c --
 *
 *	Source code for the Db_ReadEntry procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/Db_ReadEntry.c,v 1.5 89/06/15 22:45:23 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include "dbInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Db_ReadEntry --
 *
 *	Read a buffer from a specified location in the shared database.
 *	This opens and locks the file, reads the data, and closes the
 *	file.  The lock is polled if necessary.    
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	0 indicates success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Db_ReadEntry(file, buffer, index, size, lockHow)
    char *file;
    char *buffer;
    int index;
    int size;
    Db_LockHow lockHow;
{
    int status;
    int offset;
    int streamID;
    int bytesRead;
    Db_Handle handle;
    
    streamID = open(file, O_RDONLY | O_CREAT, FILE_MODE);
    if (streamID == -1) {
	syslog(LOG_ERR, "Db_ReadEntry: error opening file %s: %s.\n", file,
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
    handle.lockType = LOCK_SH;
#ifndef CLEAN
    handle.fileName = file;
#endif /* CLEAN */
    status = DbLockDesc(&handle);
    if (status == -1) {
	return(status);
    }
    
    bytesRead = read(streamID, buffer, size);
    if (bytesRead == -1) {
	status = -1;
    } else if (bytesRead != size) {
	status = -1;
	errno = 0;
    } else {
	status = 0;
    }
    (void) flock(streamID, LOCK_SH | LOCK_UN);
    (void) close(streamID);

    return(status);
}
