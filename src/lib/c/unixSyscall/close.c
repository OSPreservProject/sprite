/* 
 * close.c --
 *
 *	Procedure to map from Unix close system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/close.c,v 1.3 89/01/06 08:03:57 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * close --
 *
 *	Procedure to map from Unix close system call to Sprite Fs_Close.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  UNIX_SUCCESS is returned upon success.
 *
 * Side effects:
 *	The steamId passed to close is no longer associated with an open 
 *	file.
 *
 *----------------------------------------------------------------------
 */

int
close(streamId)
    int streamId;		/* identifier for stream to close */
{
    ReturnStatus status;	/* result returned by Fs_Close */

    status = Fs_Close(streamId);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
