/* 
 * utimes.c --
 *
 *	Procedure to map from Unix utimes system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: utimes.c,v 1.2 88/08/25 14:41:12 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"
#include <errno.h>
#include <sys/time.h>


/*
 *----------------------------------------------------------------------
 *
 * utimes --
 *
 *	Procedure to map from Unix utimes system call to Sprite 
 *	Fs_SetAttributes system call.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	The protection of the specified file is modified.
 *
 *----------------------------------------------------------------------
 */

int
utimes(path, tvp)
    char *path;
    struct timeval tvp[2];
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attributes;	/* struct containing all file attributes,
				 * only access/modify times looked at. */

    attributes.accessTime.seconds = tvp[0].tv_sec;
    attributes.accessTime.microseconds = tvp[0].tv_usec;
    attributes.dataModifyTime.seconds = tvp[1].tv_sec;
    attributes.dataModifyTime.microseconds = tvp[1].tv_usec;
    status = Fs_SetAttr(path,  FS_ATTRIB_FILE, &attributes, FS_SET_TIMES);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
