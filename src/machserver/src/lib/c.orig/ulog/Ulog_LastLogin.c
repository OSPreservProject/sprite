/* 
 * Ulog_LastLogin.c --
 *
 *	Source code for the Ulog_LastLogin procedure.
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
static char rcsid[] = "$Header: Ulog_LastLogin.c,v 1.4 88/09/22 22:14:13 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <ulog.h>
#include "ulogInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Ulog_LastLogin --
 *
 *	Retrieve information for the last login of the specified user.
 *
 * Results:
 *	The user log data structure is returned if the retrieval is
 *	successful.  If there is no valid entry for the specified user,
 *	or if an error occurs accessing the 'last log', NULL is returned.
 *
 * Side effects:
 *	The 'last log' is opened, locked, and read before closing it again.
 *
 *----------------------------------------------------------------------
 */


Ulog_Data *
Ulog_LastLogin(uid)
    int uid;
{
    static Ulog_Data data;
    char buffer[ULOG_RECORD_LENGTH];
    int status;
    int count;
    
    status = Db_ReadEntry(LASTLOG_FILE_NAME, buffer, uid, ULOG_RECORD_LENGTH,
			   DB_LOCK_BREAK);
    if (status != 0) {
	return((Ulog_Data *) NULL);
    }
    if (buffer[0] == '\0') {
	errno = EACCES;
	return((Ulog_Data *) NULL);
    }
    /*
     * Try to parse the record.  It's okay if the location field
     * doesn't match because it may be empty.
     */
    count = sscanf(buffer, ULOG_FORMAT_STRING, &data.uid,
		   &data.hostID, &data.portID,
		   &data.updated, data.location);
    if (count < ULOG_ITEM_COUNT - 1) {
	syslog(LOG_ERR, "Ulog_LastLogin: unable to parse record %d",
	       uid);
	errno = EACCES;
	return((Ulog_Data *) NULL);
    } else if (count == ULOG_ITEM_COUNT - 1) {
	data.location[0] = '\0';
    }
    return(&data);
}







