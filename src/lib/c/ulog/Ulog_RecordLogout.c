/* 
 * ULog_RecordLogout.c --
 *
 *	Source code for the ULog_RecordLogout procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/ulog/RCS/Ulog_RecordLogout.c,v 1.5 89/01/02 13:54:25 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <ulog.h>
#include "ulogInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Ulog_RecordLogout --
 *
 *	Remove information for a user from the database after the user
 * 	logs out.
 *
 *	Note: for now, this just nulls out the "user log" entry showing
 *	who's logged in on a particular host/port combination.  It
 *	can later be modified to update a "last" like database showing time
 *	logged out as well as time logged in.
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

/* ARGSUSED */
int
Ulog_RecordLogout(uid, portID)
    int uid;		/* user identifier */
    int portID;		/* index into host's area in userLog */
{
    int status;
    char myHostName[ULOG_LOC_LENGTH];
    char buffer[ULOG_RECORD_LENGTH];
    Host_Entry *hostPtr;

    if (portID >= ULOG_MAX_PORTS || portID < 0) {
#ifdef DEBUG
	syslog(LOG_ERR, "recording logout: invalid port: %d\n", portID);
#endif
	errno = EINVAL;
	return(-1);
    }

    if (gethostname(myHostName, ULOG_LOC_LENGTH) < 0) {
	syslog(LOG_ERR, "Ulog_RecordLogout: error in gethostname.\n");
	return(-1);
    }
    hostPtr = Host_ByName(myHostName);
    Host_End();
    if (hostPtr == (Host_Entry *) NULL) {
	syslog(LOG_ERR,
	       "Ulog_RecordLogout: error in Host_ByName for current host.\n");
	return(-1);
    }

    bzero(buffer, ULOG_RECORD_LENGTH);
    (void) sprintf(buffer, ULOG_FORMAT_STRING, -1, hostPtr->id, portID,
		     0, "(none)");

    status = Db_WriteEntry(ULOG_FILE_NAME, buffer,
			   hostPtr->id * ULOG_MAX_PORTS + portID,
			   ULOG_RECORD_LENGTH, DB_LOCK_BREAK);
    return(status);
}
