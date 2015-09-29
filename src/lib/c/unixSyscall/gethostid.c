/* 
 * gethostid.c --
 *
 *	Procedure to simulate Unix gethostid system call.
 *
 * Copyright 1992 Regents of the University of California
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
 * gethostid --
 *
 *	Gets the sprite identifier of the current host.
 *
 * Results:
 *	The sprite identifier.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
gethostid()
{
    ReturnStatus status;
    int localID;

    status = Proc_GetHostIDs(&localID, (int *) NULL);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return UNIX_ERROR;
    }
    return localID;
}
