/* 
 * symlink.c --
 *
 *	Procedure to map from Unix symlink system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: symlink.c,v 1.1 88/06/19 14:32:07 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * symlink --
 *
 *	Procedure to map from Unix symlink system call to Sprite Fs_SymLink.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	Create a symbolic link file named link that refers to target.
 *
 *----------------------------------------------------------------------
 */

int
symlink(target, link)
    char *target;
    char *link;
{
    ReturnStatus status;	/* result returned by Prefix_Link */

    status = Fs_SymLink(target, link, FALSE);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
