/* 
 * dup.c --
 *
 *	Procedure to map from Unix dup system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: dup.c,v 1.1 88/06/19 14:31:13 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * dup --
 *
 *	Procedure to map from Unix dup system call to Sprite Fs_GetNewID.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the new file descriptor is returned.
 *
 * Side effects:
 *	A new open file descriptor is allocated for the process.
 *
 *----------------------------------------------------------------------
 */

int
dup(oldStreamID)
    int oldStreamID;		/* original stream identifier */
{
    ReturnStatus status;	/* result returned by Fs_GetNewID */
    int newStreamID = FS_ANYID;	/* new stream identifier */

    status = Fs_GetNewID(oldStreamID, &newStreamID);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(newStreamID);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * dup2 --
 *
 *	Procedure to map from Unix dup2 system call to Sprite Fs_GetNewID.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the new file descriptor is returned.
 *
 * Side effects:
 *	A new open file descriptor is allocated for the process.
 *
 *----------------------------------------------------------------------
 */

int
dup2(oldStreamID, newStreamID)
    int oldStreamID;		/* original stream identifier */
    int newStreamID;		/* new stream identifier */
{
    ReturnStatus status;	/* result returned by Fs_GetNewID */

    status = Fs_GetNewID(oldStreamID, &newStreamID);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
