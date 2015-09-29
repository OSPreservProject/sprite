/* 
 * mkdir.c --
 *
 *	Procedure to map from Unix mkdir system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: mkdir.c,v 1.1 88/06/19 14:31:37 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"
#include <sys/file.h>


/*
 *----------------------------------------------------------------------
 *
 * mkdir --
 *
 *	Procedure to map from Unix mkdir system call to Sprite Fs_MakeDir.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Otherwise UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
mkdir(pathName, permissions)
    char *pathName;		/* The name of the directory to create */
    int permissions;		/* Permission mask to use on creation */
{
    ReturnStatus status;	/* result returned by Fs_Open */

    status = Fs_MakeDir(pathName, permissions);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
