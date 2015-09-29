/* 
 * pipe.c --
 *
 *	Procedure to map from Unix pipe system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: pipe.c,v 1.1 88/06/19 14:31:40 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * pipe --
 *
 *	Procedure to map from Unix pipe system call to Sprite Fs_CreatePipe
 *	system call.
 *
 * Results:
 *	Error returned if error returned from Fs_CreatePipe. Otherwise
 *	UNIX_SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
pipe(filedes)
    int	filedes[2];			/* array of stream identifiers */	
{
    ReturnStatus status;	/* result returned by Fs_CreatePipe */

    status = Fs_CreatePipe(&(filedes[0]), &(filedes[1]));
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
