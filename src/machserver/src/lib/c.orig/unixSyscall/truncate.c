/* 
 * truncate.c --
 *
 *	Procedure to map from Unix truncate system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: truncate.c,v 1.2 88/07/29 17:41:05 ouster Exp $ SPRITE (Berkeley)";
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
 *	UNIX_SUCCESS is returned.
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
    return(Fs_Truncate(path, (int) length));
}
