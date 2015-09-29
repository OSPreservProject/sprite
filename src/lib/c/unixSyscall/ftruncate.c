/* 
 * ftruncate.c --
 *
 *	Procedure to map from Unix ftruncate system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/ftruncate.c,v 1.3 91/09/12 21:38:11 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include "compatInt.h"
#include "fs.h"
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * ftruncate --
 *
 *	Procedure to map from Unix ftruncate system call to Sprite 
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
ftruncate(fd, length)
    int fd;
    unsigned long length;
{
    ReturnStatus status = SUCCESS;

    status = Ioc_Truncate(fd, (int) length);
    if (status == SUCCESS) {
	return(UNIX_SUCCESS);
    } else {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    }
}
