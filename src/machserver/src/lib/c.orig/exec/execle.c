/* 
 * execle.c --
 *
 *	Source code for the execle library procedure.
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
static char rcsid[] = "$Header: execle.c,v 1.4 88/07/28 17:41:50 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdlib.h>
#include <varargs.h>

/*
 * Library imports:
 */

extern execve();
extern char **_ExecArgs();


/*
 *----------------------------------------------------------------------
 *
 * execle --
 *
 *	Execute a process, using an explicitly-supplied environment, and
 *	with arguments in-line (as individual parameters) instead of in
 *	a separate array.
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
execle(va_alist)
    va_dcl			/* Name of file containing program to exec,
				 * followed by one or more arguments of type
				 * "char *", terminated by a zero argument,
				 * followed by a "char **" environment
				 * pointer. */
{
    char *name;
    char **argv;
    va_list args;

    va_start(args);
    name = va_arg(args, char *);
    argv = _ExecArgs(&args);
    execve(name, argv, va_arg(args, char **));
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
