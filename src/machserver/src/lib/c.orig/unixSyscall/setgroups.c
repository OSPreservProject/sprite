/* 
 * setgroups.c --
 *
 *	Procedure to map from Unix setgroups system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: setgroups.c,v 1.1 88/06/19 14:31:56 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * setgroups --
 *
 *	Procedure to map from Unix setgroups system call to 
 *	Sprite Proc_SetGroupIDs.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
setgroups(ngroups, gidset)
    int ngroups;
    int *gidset;
{
    ReturnStatus status;	/* result returned by Proc_SetGroupIDs */

    status = Proc_SetGroupIDs(ngroups, gidset);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
