/* 
 * Ulog_RecordLogin.c --
 *
 *	Source code for the Ulog_RecordLogin procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/ulog/RCS/Ulog_RecordLogin.c,v 1.5 89/01/02 13:54:17 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <ulog.h>
#include "ulogInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Ulog_RecordLogin --
 *
 *	Record information for a user in the database.
 *
 * Results:
 *	-1 indicates an error, in which case errno indicates more details.
 *	0 indicates success.
 *
 * Side effects:
 *	The database file is updated.
 *
 *----------------------------------------------------------------------
 */

int
Ulog_RecordLogin(uid, location, portID)
    int uid;		/* user identifier */
    char *location;	/* string identifying user's location (host
			   or "terminal") */
    int portID;		/* index into host's area in file*/
{
    struct timeval time;
    int status;
    char myHostName[ULOG_LOC_LENGTH];
    char buffer[ULOG_RECORD_LENGTH];
    Host_Entry *hostPtr;


    if (portID >= ULOG_MAX_PORTS) {
	syslog(LOG_WARNING,
	       "Unable to record login for uid %d, port %d: maximum number of entries exceeded.",
	       uid, portID);
	errno = EINVAL;
	return(-1);
    }
    if (portID < 0) {
	syslog(LOG_ERR, "Invalid port for recording login: %d\n", portID);
	errno = EINVAL;
	return(-1);
    }
    if (strlen(location) >= ULOG_LOC_LENGTH) {
	syslog(LOG_ERR, "Ulog_RecordLogin: location name (%s) too large.",
	       location);
	errno = EINVAL;
	return(-1);
    }
#ifdef DEBUG
    syslog(LOG_INFO, "Recording login for uid %d, port %d.", uid, portID);
#endif 
    status = gettimeofday(&time, (struct timezone *) NULL);
    if (status == -1) {
	return(status);
    }
    if (gethostname(myHostName, ULOG_LOC_LENGTH) < 0) {
	syslog(LOG_ERR, "Ulog_RecordLogin: error in gethostname.\n");
	return(-1);
    }
    hostPtr = Host_ByName(myHostName);
    Host_End();
    if (hostPtr == (Host_Entry *) NULL) {
	syslog(LOG_ERR,
	       "Ulog_RecordLogin: error in Host_ByName for current host.\n");
	return(-1);
    }
    bzero(buffer, ULOG_RECORD_LENGTH);
    (void) sprintf(buffer, ULOG_FORMAT_STRING, uid, hostPtr->id, portID,
		     time.tv_sec, location);

    status = Db_WriteEntry(ULOG_FILE_NAME, buffer, 
			   hostPtr->id * ULOG_MAX_PORTS + portID,
			   ULOG_RECORD_LENGTH, DB_LOCK_BREAK);
    if (status != 0) {
	return(status);
    }
    status = Db_WriteEntry(LASTLOG_FILE_NAME, buffer, uid, ULOG_RECORD_LENGTH,
			   DB_LOCK_BREAK);
    return(status);
}
