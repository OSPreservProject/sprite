/* 
 * fsync.c --
 *
 *	Procedure to map from Unix fsync system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: fsync.c,v 1.3 88/07/25 09:15:40 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include "compatInt.h"
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * fsync --
 *
 *	Procedure to map from Unix fsync system call to Sprite 
 *	system call.
 *
 * Results:
 *	UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
fsync(fd)
    int fd;			/* Identifier for stream to flush to disk. */
{
    ReturnStatus status;

    status = Fs_WriteBackID(fd, -1, -1, 1);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
