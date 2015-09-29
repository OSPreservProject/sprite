/* 
 * execl.c --
 *
 *	Source code for the execl library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/exec/RCS/execl.c,v 1.1 92/06/08 22:52:51 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdlib.h>
#include <varargs.h>

/*
 * Library imports:
 */

extern char **environ;
extern execve();
extern char **_ExecArgs();


/*
 *----------------------------------------------------------------------
 *
 * execl --
 *
 *	Execute a process, using the current environment variable,
 *	instead of an explicitly-supplied one, and with arguments
 *	in-line instead of in a separate array.
 *
 * Results:
 *	This procedure returns only if the exec fails.  In this case
 *	the return value is -1.
 *
 * Side effects:
 *	Overlays the current process with a new image.  See the man
 *	page for details.
 *
 *----------------------------------------------------------------------
 */

#ifndef lint
int
execl(va_alist)
    va_dcl			/* Name of file containing program to exec,
				 * followed by one or more arguments of type
				 * "char *", terminated by a zero argument. */
{
    char **argv;
    char *name;
    va_list args;

    va_start(args);
    name = va_arg(args, char *);
    argv = _ExecArgs(&args);
    execve(name, argv, environ);
    free((char *) argv);
    return -1;
}
#else
/* VARARGS2 */
/* ARGSUSED */
int
execl(file, arg1)
    char *file;
    char *arg1;
{
    return 0;
}
#endif
