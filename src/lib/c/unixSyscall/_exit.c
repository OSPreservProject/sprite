/* 
 * _exit.c --
 *
 *	Procedure to map from Unix _exit system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/_exit.c,v 1.1 88/06/19 14:31:00 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * _exit --
 *
 *	Procedure to map from Unix _exit system call to Sprite Proc_RawExit.
 *
 * Results:
 *	_exit() should never return.  If it does, however, UNIX_ERROR is
 *	returned.
 *
 * Side effects:
 *	Any open streams are closed, then the process invoking _exit() is
 *	terminated.
 *
 *----------------------------------------------------------------------
 */

int
_exit(exitStatus)
    int exitStatus;		/* process's termination status */
{
    Proc_RawExit(exitStatus);
    /*
     * We should never reach this point, regardless of status value.
     */
    errno = Compat_MapCode(FAILURE);
    return(UNIX_ERROR);
}
