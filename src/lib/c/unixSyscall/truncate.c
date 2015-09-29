/* 
 * truncate.c --
 *
 *	Procedure to map from Unix truncate system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/truncate.c,v 1.3 91/09/12 23:38:50 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include "compatInt.h"
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * truncate --
 *
 *	Procedure to map from Unix truncate system call to Sprite 
 *	system call.
 *
 * Results:
 *	UNIX_SUCCESS is returned, or
 *      UNIX_ERROR with errno set appropriately.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
truncate(path, length)
    char *path;
    unsigned long length;
{
    ReturnStatus status = SUCCESS;

    status = Fs_Truncate(path, (int) length);
    if (status == SUCCESS) {
	return(UNIX_SUCCESS);
    } else {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    }
}
