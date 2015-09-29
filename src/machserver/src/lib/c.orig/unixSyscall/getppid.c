/* 
 * getppid.c --
 *
 *	Source code for the getppid library procedure.
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
static char rcsid[] = "$Header: getppid.c,v 1.2 88/07/29 17:40:34 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <proc.h>
#include "compatInt.h"

/*
 *----------------------------------------------------------------------
 *
 * getppid --
 *
 *	Procedure to map from Unix getppid system call to Sprite Proc_GetIDs.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the ID of the parent of the current
 *	process is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getppid()
{
    ReturnStatus status;	/* result returned by Proc_GetIDs */
    int parentPid;		/* ID of parent of current process */

    status = Proc_GetIDs((int *) NULL, &parentPid, (int *) NULL, (int *) NULL);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(parentPid);
    }
}
