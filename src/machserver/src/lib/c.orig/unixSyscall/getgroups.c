/* 
 * getgroups.c --
 *
 *	Procedure to map from Unix getgroups system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getgroups.c,v 1.1 88/06/19 14:31:23 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getgroups --
 *
 *	Procedure to map from Unix getgroups system call to 
 *	Sprite Proc_GetGroupIDs.
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
getgroups(gidsetlen, gidset)
    int gidsetlen;
    int *gidset;
{
    ReturnStatus status;	/* result returned by Proc_GetGroupIDs */
    int	numGids;

    status = Proc_GetGroupIDs(gidsetlen, gidset, &numGids);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	if (numGids > gidsetlen) {
	    numGids = gidsetlen;
	}
	return(numGids);
    }
}
