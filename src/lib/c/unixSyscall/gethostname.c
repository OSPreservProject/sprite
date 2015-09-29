/* 
 * gethostname.c --
 *
 *	Procedure to simulate Unix gethostname system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/gethostname.c,v 1.5 92/06/16 11:20:59 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <host.h>
#include <string.h>
#include <sys.h>
#include <sys/param.h>
#include <status.h>
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * gethostname --
 *
 *	Puts the host name into the the given buffer.
 *
 * Results:
 *	0 is returned if the call was completed successfully.
 *	Otherwise, -1 is returned and errno gives more information.
 *
 * Side effects:
 *	The name buffer is modified.
 *
 *----------------------------------------------------------------------
 */

int
gethostname(name, nameLen)
    char *name;		/* Place to store name. */
    int nameLen;	/* Length of name buffer. */
{
    char		tmp[MAXHOSTNAMELEN];
    ReturnStatus 	status;

    Host_Entry *entry;
    int localID;
    /*
     * Try using the new system call. If that doesn't work then do it
     * the old way. Strip out the old way once all kernels have the
     * system call -- jhh
     */

    status = Sys_GetHostName(tmp);
    if (status == SYS_INVALID_SYSTEM_CALL) {
	status = Proc_GetHostIDs(&localID, (int *) NULL);
	if (status != SUCCESS) {
		errno = Compat_MapCode(status);
	    return UNIX_ERROR;
	}

	entry = Host_ByID(localID);
	if (entry == (Host_Entry *) NULL) {
	    Host_End();
	    return UNIX_ERROR;
	}
	strncpy(name, entry->name, nameLen-1);
	name[nameLen-1] = 0;
	Host_End();
    } else if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return UNIX_ERROR;
    } else {
	strncpy(name, tmp, nameLen);
    }
    return 0;
}
