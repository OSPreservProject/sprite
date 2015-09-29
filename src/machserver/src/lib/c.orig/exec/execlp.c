/* 
 * execlp.c --
 *
 *	Source code for the execlp library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/exec/RCS/execlp.c,v 1.5 89/01/27 21:09:39 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdlib.h>
#include <varargs.h>

/*
 * Library imports:
 */

extern char **environ;
extern execvp();
extern char **_ExecArgs();


/*
 *----------------------------------------------------------------------
 *
 * execlp --
 *
 *	Execute a process, using the current environment variable,
 *	instead of an explicitly-supplied one, and with arguments
 *	in-line instead of in a separate array.  Also, imitate the
 *	shell's actions in trying each directory in a search path
 *	(given by the "PATH" environment variable).
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
execlp(va_alist)
    va_dcl			/* Name of file containing program to exec,
				 * followed by one or more arguments of type
				 * "char *", terminated by a zero argument. */
{
    char *name;
    char **argv;
    va_list args;

    va_start(args);
    name = va_arg(args, char *);
    argv = _ExecArgs(&args);
    execvp(name, argv);
    free((char *) argv);
    return -1;
}
#else
/* VARARGS2 */
/* ARGSUSED */
int
execlp(file, arg1)
    char *file;
    char *arg1;
{
    return 0;
}
#endif
