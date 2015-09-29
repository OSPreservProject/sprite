/* 
 * rmdir.c --
 *
 *	Procedure to map from Unix rmdir system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: rmdir.c,v 1.1 88/06/19 14:31:54 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"
#include <sys/file.h>


/*
 *----------------------------------------------------------------------
 *
 * rmdir --
 *
 *	Procedure to map from Unix rmdir system call to Sprite Fs_RemoveDir.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Othewise UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
rmdir(pathName)
    char *pathName;		/* The name of the directoy to remove */
{
    ReturnStatus status;	/* result returned by Fs_RemoveDir */

    status = Fs_RemoveDir(pathName);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
