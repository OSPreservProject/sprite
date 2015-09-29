/* 
 * chmod.c --
 *
 *	Procedure to map from Unix chmod and fchmod system calls to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: chmod.c,v 1.2 88/08/25 14:40:53 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * chmod --
 *
 *	Procedure to map from Unix chmod system call to Sprite 
 *	Fs_SetAttr system call.
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
chmod(path, mode)
    char *path;
    int mode;
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attributes;	/* struct containing all file attributes,
				 * only mode is looked at. */

    attributes.permissions = mode;
    status = Fs_SetAttr(path,  FS_ATTRIB_FILE, &attributes, FS_SET_MODE);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * fchmod --
 *
 *	Procedure to map from Unix fchmod system call to Sprite 
 *	Fs_SetAttributesID system call.
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
fchmod(fd, mode)
    int fd;
    int mode;
{
    ReturnStatus status;	   /* result returned by Sprite system calls */
    Fs_Attributes attributes;      /* struct containing all file attributes,
				    * only mode is looked at. */

    attributes.permissions = mode;
    status = Fs_SetAttrID(fd, &attributes, FS_SET_MODE);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
