/* 
 * unlink.c --
 *
 *	Procedure to map from Unix unlink system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: unlink.c,v 1.1 88/06/19 14:32:10 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * unlink --
 *
 *	Procedure to map from Unix unlink system call to Sprite Fs_Remove.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	The named file is removed from the filesystem.
 *
 *----------------------------------------------------------------------
 */

int
unlink(pathName)
    char *pathName;			/* file to remove */
{
    ReturnStatus status;	/* result returned by Fs_Remove */

    status = Fs_Remove(pathName);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
