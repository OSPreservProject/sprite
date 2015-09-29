/* 
 * link.c --
 *
 *	Procedure to map from Unix link system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: link.c,v 1.1 88/06/19 14:31:36 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * link --
 *
 *	Procedure to map from Unix link system call to Sprite Fs_HardLink.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	Cause the two pathnames to refer to the same file.
 *
 *----------------------------------------------------------------------
 */

int
link(name1, name2)
    char *name1;
    char *name2;
{
    ReturnStatus status;	/* result returned by Prefix_Link */

    status = Fs_HardLink(name1,name2);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
