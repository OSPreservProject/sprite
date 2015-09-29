/* 
 * umask.c --
 *
 *	Procedure to map from Unix umask system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: umask.c,v 1.1 88/06/19 14:32:09 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"

/*
 * These are the bits to invert and pass to the Sprite system call.
 */

#define PERMISSION_MASK 0777


/*
 *----------------------------------------------------------------------
 *
 * umask --
 *
 *	Procedure to map from Unix umask system call to Sprite
 *	Fs_SetDefPerm.
 *
 * Results:
 *	On success, the former value of umask is returned.  If
 *	Fs_SetDefPerm returns an error,	UNIX_ERROR is returned and the
 *	actual error code is stored in errno.  
 *
 * Side effects:
 *	The default protection of files created by the current process
 *	is changed.
 *
 *----------------------------------------------------------------------
 */

int
umask(newMask)
    int newMask;
{
    ReturnStatus status;	/* result returned by Fs_SetDefPerm */
    int oldMask;

    /*
     * Sprite default permissions are the logical NOT of Unix permissions.
     */

    newMask = (~newMask) & PERMISSION_MASK;
    
    status = Fs_SetDefPerm(newMask, &oldMask);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	oldMask = (~oldMask) & PERMISSION_MASK;
	return(oldMask);
    }
}
