/* 
 * ftruncate.c --
 *
 *	Procedure to map from Unix ftruncate system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: ftruncate.c,v 1.2 88/07/29 17:39:26 ouster Exp $ SPRITE (Berkeley)";
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
 *	UNIX_SUCCESS is returned.
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
    return(Ioc_Truncate(fd, (int) length));
}
