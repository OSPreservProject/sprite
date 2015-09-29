/* 
 * execve.c --
 *
 *	Procedure to emulate the UNIX execve kernel call under Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/execve.c,v 1.2 88/10/28 08:58:01 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"

#include "compatInt.h"

/*
 * The variable below is a secret trap door that can be set
 * to non-zero to force the next exec to put the process into
 * the debugger before it executes its first instruction.
 */

int _execDebug = 0;


/*
 *----------------------------------------------------------------------
 *
 * execve --
 *
 *	Procedure to map from Unix execve system call to Sprite Proc_ExecEnv.
 *
 * Results:
 *	execve() should never return.  If it does, however, UNIX_ERROR is
 *	returned.
 *
 * Side effects:
 *	Any open streams are closed, then the process invoking execve() is
 *	terminated.
 *
 *----------------------------------------------------------------------
 */

int
execve(name, argv, envp)
    char *name;			/* name of file to exec */
    char *argv[];		/* array of arguments */
    char *envp[];		/* array of environment pointers */
{
    ReturnStatus status;	/* result returned by Sprite system calls  */

    status = Proc_ExecEnv(name, argv, envp, _execDebug);

    /*
     * We should never reach this point, regardless of status value.
     */

    errno = Compat_MapCode(status);
    return(UNIX_ERROR);
}
