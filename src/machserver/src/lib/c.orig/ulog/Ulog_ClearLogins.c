/* 
 * Ulog_ClearLogins.c --
 *
 *	Source code for the Ulog_ClearLogins procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/ulog/RCS/Ulog_ClearLogins.c,v 1.2 89/01/13 11:49:52 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <ulog.h>
#include "ulogInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Ulog_ClearLogins --
 *
 *	Void any user log records for the current host.
 *
 * Results:
 *	0 on success, -1 if an error occurs (and errno will indicate the
 *	nature of the error).
 *
 * Side effects:
 *	The 'user log' is opened and locked while it is being modified.
 *
 *----------------------------------------------------------------------
 */


int
Ulog_ClearLogins()
{
    char myHostName[ULOG_LOC_LENGTH];
    char buffer[ULOG_RECORD_LENGTH];
    Host_Entry *hostPtr;
    Db_Handle handle;
    int errnoTmp;
    int i;

    if (gethostname(myHostName, ULOG_LOC_LENGTH) < 0) {
	syslog(LOG_ERR, "Ulog_ClearLogins: error in gethostname.\n");
	return(-1);
    }
    hostPtr = Host_ByName(myHostName);
    Host_End();
    if (hostPtr == (Host_Entry *) NULL) {
	syslog(LOG_ERR,
	       "Ulog_ClearLogins: error in Host_ByName for current host.\n");
	return(-1);
    }

    if (Db_Open(ULOG_FILE_NAME, ULOG_RECORD_LENGTH, &handle, 1,
		     DB_LOCK_OPEN, DB_LOCK_BREAK, 0) < 0) {
	return(-1);
    }

    bzero(buffer, ULOG_RECORD_LENGTH);
    (void) sprintf(buffer, ULOG_FORMAT_STRING, -1, hostPtr->id, -1,
		 0, "(none)");

    /*
     * Write the first entry, specifying the location.  Then do writes
     * sequentially (avoiding seeks).
     */

    if (Db_Put(&handle, buffer, hostPtr->id * ULOG_MAX_PORTS) < 0) {
	goto error;
    }
    
    for (i = 1; i < ULOG_MAX_PORTS; i++) {
	if (Db_Put(&handle, buffer, -1) < 0) {
	    goto error;
	}
    }
    if (Db_Close(&handle) < 0) {
	return(-1);
    }
    return(0);


    /*
     * On error, try to close the database, but make sure to return
     * the error in errno that came from the error that made us quit.
     */
error:
    errnoTmp = errno;
    (void) Db_Close(&handle);
    errno = errnoTmp;

    return(-1);
}







