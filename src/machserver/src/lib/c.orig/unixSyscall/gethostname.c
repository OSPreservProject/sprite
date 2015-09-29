/* 
 * gethostname.c --
 *
 *	Procedure to simulate Unix gethostname system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/gethostname.c,v 1.4 89/10/16 14:34:26 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include <host.h>
#include <string.h>
#include <sys.h>
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
    Host_Entry *entry;
    ReturnStatus status;
    int localID;

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
    return 0;
}
