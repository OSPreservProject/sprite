/* 
 * Ulog_GetAllLogins.c --
 *
 *	Source code for the Ulog_GetAllLogins procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/ulog/RCS/Ulog_GetAllLogins.c,v 1.4 89/01/13 11:49:34 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <ulog.h>
#include "ulogInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Ulog_GetAllLogins --
 *
 *	Get the records for all valid login records, up to the number of
 * 	records specified.  If the records correspond to a machine
 *	that is down, they will still be returned and it is the responsibility
 * 	of the caller to verify the status of each machine.
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	On success, the number of structures being returned in *dataPtr
 *	is returned as the value of the function.
 *	*locPtr is set to the next location to be searched.
 *
 * Side effects:
 *	The database file is opened for reading, locked, and later closed.
 *
 *----------------------------------------------------------------------
 */

#define NUMBUF 100

int
Ulog_GetAllLogins(numEntries, locPtr, dataPtr)
    int numEntries;		/* number of structures in *dataPtr */
    int *locPtr;		/* index of first structure to return, set
				 * to first unseen structure on return */
    Ulog_Data *dataPtr;		/* Pointer to array of structures */
{
    int status;
    Db_Handle handle;
    int i;
    register int loc = *locPtr;
    char buffer[ULOG_RECORD_LENGTH];
    Ulog_Data *thisPtr;
    int count;

    status = Db_Open(ULOG_FILE_NAME, ULOG_RECORD_LENGTH, &handle, 0,
		     DB_LOCK_OPEN, DB_LOCK_BREAK, NUMBUF);
    if (status != 0) {
	return(status);
    }
    i = 0;
    while(i < numEntries) {
	status = Db_Get(&handle, buffer, loc);
	thisPtr = &dataPtr[i];
	/*
	 * We go until we get a failure status, then return what we
	 * have so far.
	 */
	if (status == -1) {
	    break;
	}
	count = sscanf(buffer, ULOG_FORMAT_STRING, &thisPtr->uid,
		       &thisPtr->hostID, &thisPtr->portID,
		       &thisPtr->updated, thisPtr->location);
	/*
	 * Only return a record if its updated field is non-zero (i.e.,
	 * never initialized or invalidated by being reset to 0.
	 * In this case, increment i so the next record goes in the
	 * next location, and increment the record number to get.  If
	 * sscanf can't parse the record properly, assume it's invalid.
	 * It's okay if the location field doesn't match because it
	 * may be empty. 
	 */
	if (thisPtr->updated != 0 && count >= ULOG_ITEM_COUNT - 1) {
	    if (count == ULOG_ITEM_COUNT - 1) {
		thisPtr->location[0] = '\0';
	    }
	    i++;
	}
	loc++;
    }
    (void) Db_Close(&handle);
    *locPtr = loc;
    return(i);
}
