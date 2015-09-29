/* 
 * execvp.c --
 *
 *	Source code for the execvp library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/exec/RCS/execvp.c,v 1.5 91/07/22 10:42:58 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <string.h>
#include <stdlib.h>
#include <errno.h>

/*
 * Library imports:
 */

extern char **environ;
extern execve();


/*
 *-----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Function to actually execute a program. If the exec didn't succeed
 *	because the file isn't in a.out format, attempt to execute
 *	it as a bourne shell script.
 *
 * Results:
 *	None.  Doesn't even return unless the exec failed.
 *
 * Side Effects:
 *	A program may be execed over this one.
 *
 *-----------------------------------------------------------------------
 */

static void
DoExec(file, argv)
    char *file;			/* File to execute. */
    char **argv;		/* Arguments to the program. */
{
    execve(file, argv, environ);
    if (errno == ENOEXEC) {
	/*
	 * Attempt to execute the file as a shell script using
	 * the Bourne shell)
	 */
	register int i;
#define MAX_ARGS 1000
	char *newArgv[MAX_ARGS+1];

	for (i = 0; argv[i] != 0; i++) {
	    /* Empty loop body */
	}
	if (i >= MAX_ARGS) {
	    return;
	}
	newArgv[0] = "sh";
	newArgv[1] = file;
	for (i = 1; argv[i] != 0; i++) {
	    newArgv[i+1] = argv[i];
	}
	newArgv[i+1] = 0;
	execve("/sprite/cmds/sh", newArgv, environ);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * execvp --
 *
 *	Execute a process, using the current environment variable,
 *	instead of an explicitly-supplied one.  Also, imitate the
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

int
execvp(name, argv)
    char *name;			/* Name of file containing program to exec. */
    char **argv;		/* Array of arguments to pass to program. */
{
    char *path;
#define MAX_NAME_SIZE 1000
    char fullName[MAX_NAME_SIZE+1];
    register char *first, *last;
    int nameLength, size, noAccess;

    noAccess = 0;

    if (index(name, '/') != 0) {
	/*
	 * If the name specifies a path, don't search for it on the search path,
	 * just try and execute it.
	 */
	DoExec(name, argv);
	return -1;
    }

    path = getenv("PATH");
    if (path == 0) {
	path = "/sprite/cmds";
    }
    nameLength = strlen(name);
    for (first = path; ; first = last+1) {

	/*
	 * Generate the next file name to try.
	 */

	for (last = first; (*last != 0) && (*last != ':'); last++) {
	    /* Empty loop body. */
	}
	size = last-first;
	if ((size + nameLength + 2) >= MAX_NAME_SIZE) {
	    continue;
	}
	(void) strncpy(fullName, first, size);
	if (last[-1] != '/') {
	    fullName[size] = '/';
	    size++;
	}
	(void) strcpy(fullName + size, name);

	DoExec(fullName, argv);
	if (errno == EACCES) {
	    noAccess = 1;
	} else if (errno != ENOENT) {
	    break;
	}
	if (*last == 0) {
	    /*
	     * Hit the end of the path. We're done.
	     * If there existed a file by the right name along the search path,
	     * but its permissions were wrong, return FS_NO_ACCESS. Else return
	     * whatever we just got back.
	     */
	    if (noAccess) {
		errno = EACCES;
	    }
	    break;
	}
    }
    return -1;
}
