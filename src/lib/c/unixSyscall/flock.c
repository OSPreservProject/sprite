/* 
 * flock.c --
 *
 *	Procedure to map from Unix flock system call to Sprite.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/flock.c,v 1.2 89/01/06 08:03:34 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include <bit.h>

#include "compatInt.h"
#include <sys/file.h>
#include <errno.h>
#include <stdlib.h>


/*
 *----------------------------------------------------------------------
 *
 * flock --
 *
 *	Procedure to map from Unix flock system call to Sprite Ioc_Lock/Unlock.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
flock(descriptor, operation)
    int descriptor;		/* descriptor for stream to lock */
    int operation;		/* flags for locking descriptor */
{
    ReturnStatus status;
    int spriteLockOp = 0;

    if (operation & LOCK_EX) {
	spriteLockOp |= IOC_LOCK_EXCLUSIVE;
    } else if (operation & LOCK_SH) {
	spriteLockOp |= IOC_LOCK_SHARED;
    }
    if (operation & LOCK_NB) {
	spriteLockOp |= IOC_LOCK_NO_BLOCK;
    }
    if (operation & LOCK_UN) {
	status = Ioc_Unlock(descriptor, spriteLockOp);
    } else {
	status = Ioc_Lock(descriptor, spriteLockOp);
    }
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
 * Unix_CloseLock --
 *
 *	Release any locks held by this process on the given descriptor
 *	before it is closed. Called by close().
 *
 *	This is superceeded by the cleanup done in the Sprite kernel.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Unix_CloseLock (fd)
    int fd;
{
    return;
}

