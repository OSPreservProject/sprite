/* 
 * chown.c --
 *
 *	Procedures to map from Unix chown and fchown system calls
 *	to Sprite system call.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: chown.c,v 1.3 88/08/25 14:41:09 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * chown --
 *
 *	Procedure to map from Unix chown system call to Sprite 
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
chown(path, owner, group)
    char *path;
    int owner;
    int group;
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attributes;	/* struct containing all file attributes.
				 * only ownership is looked at. */

    attributes.uid = owner;
    attributes.gid = group;
    status = Fs_SetAttr(path,  FS_ATTRIB_LINK, &attributes, FS_SET_OWNER);
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
 * fchown --
 *
 *	Procedure to map from Unix fchown system call to Sprite 
 *	Fs_SetAttrID system call.
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
fchown(fd, owner, group)
    int fd;
    int owner;
    int group;
{
    ReturnStatus status;	/* result returned by Sprite system calls */
    Fs_Attributes attributes;	/* struct containing all file attributes,
				 * only ownship info is looked at. */

    attributes.uid = owner;
    attributes.gid = group;
    status = Fs_SetAttrID(fd, &attributes, FS_SET_OWNER);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
