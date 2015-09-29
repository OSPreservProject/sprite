/* 
 * Db_Close.c --
 *
 *	Source code for the Db_Close procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/Db_Close.c,v 1.2 89/01/13 11:43:51 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include "dbInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Db_Close --
 *
 *	Unlock (if needed) and close the file referred to by the handle.
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	0 indicates success.
 *
 * Side effects:
 *	Resets streamID to -1 and buffer to NULL to guard against use
 * 	after Db_Close has been called.
 *
 *----------------------------------------------------------------------
 */

int
Db_Close(handlePtr)
    Db_Handle *handlePtr;
{
    register int status;

    if (handlePtr->buffer != (char *) NULL) {
	(void) free(handlePtr->buffer);
	handlePtr->buffer = (char *) NULL;
    }
#ifndef CLEAN
    (void) free(handlePtr->fileName);
    handlePtr->fileName = (char *) NULL;
#endif /* CLEAN */

    if (handlePtr->lockWhen == DB_LOCK_OPEN) {
	(void) flock(handlePtr->streamID, handlePtr->lockType | LOCK_UN);
    }
    status = close(handlePtr->streamID);
    handlePtr->streamID = -1;
    return(status);
}
