/* 
 * execv.c --
 *
 *	Source code for the execv library procedure.
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
static char rcsid[] = "$Header: execv.c,v 1.1 88/06/19 16:55:59 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

/*
 * Library imports:
 */

extern char **environ;
extern execve();


/*
 *----------------------------------------------------------------------
 *
 * execv --
 *
 *	Execute a process, using the current environment variable,
 *	instead of an explicitly-supplied one.
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

int
execv(name, argv)
    char *name;			/* Name of file containing program to exec. */
    char **argv;		/* Array of arguments to pass to program. */
{
    execve(name, argv, environ);
    return -1;
}
