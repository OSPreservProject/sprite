/* 
 * chdir.c --
 *
 *	Procedure to map from Unix chdir system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: chdir.c,v 1.1 88/06/19 14:31:03 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * chdir --
 *
 *	Procedure to map from Unix chdir system call to Sprite Fs_ChangeDir.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
chdir(pathName)
    char *pathName;		/* directory to be made current directory */
{
    ReturnStatus status;	/* result returned by Fs_ChangeDir */

    status = Fs_ChangeDir(pathName);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
