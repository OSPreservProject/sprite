/* 
 * creat.c --
 *
 *	Procedure to map from Unix creat system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: creat.c,v 1.1 88/06/19 14:31:13 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * creat --
 *
 *	Procedure to map from Unix creat system call to Sprite Fs_Open,
 *	with appropriate parameters.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  A file descriptor is returned upon success.
 *
 * Side effects:
 *	Creating a file sets up state in the filesystem until the file is
 *	closed.  
 *
 *----------------------------------------------------------------------
 */

int
creat(pathName, permissions)
    char *pathName;		/* The name of the file to create */
    int permissions;		/* Permission mask to use on creation */
{
    int streamId;		/* place to hold stream id allocated by
				 * Fs_Open */
    ReturnStatus status;	/* result returned by Fs_Open */

    status = Fs_Open(pathName, FS_CREATE|FS_TRUNC|FS_WRITE, permissions,
		    &streamId);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(streamId);
    }
}
