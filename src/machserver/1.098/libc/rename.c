/* 
 * link.c --
 *
 *	Procedure to map from Unix rename system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: rename.c,v 1.1 88/06/19 14:31:53 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * rename --
 *
 *	Procedure to map from Unix rename system call to Sprite Fs_Rename.
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
rename(from, to)
    char *from;
    char *to;
{
    ReturnStatus status;	/* result returned by Prefix_Link */

    status = Fs_Rename(from, to);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
