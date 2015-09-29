/* 
 * access.c --
 *
 *	Procedure to map from Unix access system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: access.c,v 1.2 88/08/10 11:38:58 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "proc.h"

#include "compatInt.h"
#include <errno.h>
#include <sys/file.h>

/*
 *----------------------------------------------------------------------
 *
 * access --
 *
 *	Procedure for Unix access call. 
 *
 * Results:
 *	UNIX_SUCCESS if the access mode was valid.
 *	UNIX_FAILURE if the access mode was not valid.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
access(pathName, mode)
    char *pathName;		/* The name of the file to open */
    int	 mode;			/* access mode to test for */
{
    int spriteMode;
    ReturnStatus status;

    if (mode == F_OK) {
	spriteMode = FS_EXISTS;
    } else {
	spriteMode = ((mode&R_OK)?FS_READ:0) | ((mode&W_OK)?FS_WRITE:0) |
	    ((mode&X_OK)?FS_EXECUTE:0);
    }

    status = Fs_CheckAccess(pathName, spriteMode, TRUE);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
