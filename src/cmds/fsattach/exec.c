/* 
 * exec.c --
 *
 *	Routines that provide a nice interface for execing routines with a
 *	variable number of parameters.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/fsattach/RCS/exec.c,v 1.3 89/06/07 22:14:25 jhh Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "fsattach.h"
#include <varargs.h>


static char 	*argArray[MAX_EXEC_ARGS];
static int	argCount;
static char	*routineName;
/*
 *----------------------------------------------------------------------
 *
 * StartExec --
 *
 *	Initialize data structures for doing an exec. The routine name must
 *	point to an area of storage that is not modified until DoExec()
 *	is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arg count is set to 1, routine name stored and first arg stored.
 *
 *----------------------------------------------------------------------
 */

void
StartExec(routine, firstArg)
    char	*routine;
    char	*firstArg;
{
    routineName = routine;
    argArray[0] = firstArg;
    argCount = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * AddExecArgs --
 *
 *	The list of arguments is added to the array to be passed to exec.
 *	The storage used by the arguments must not be modified until
 *	DoExec is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The argument array and arg count are modified.
 *
 *----------------------------------------------------------------------
 */
#ifdef lint
/*VARARGS*/
/*ARGSUSED*/
void
AddExecArgs(foo)
    char	*foo;
{}

#else

void
AddExecArgs(va_alist)
    va_dcl

{
    char	*string;

    va_list	args;

    va_start(args);
    for(string = va_arg(args, char *);
	string != NULL; 
	string = va_arg(args, char *), argCount++) {

	if (argCount >= MAX_EXEC_ARGS) {
	    (void) fprintf(stderr, "Too many args to exec.\n");
	    return;
	}
	argArray[argCount] = string;
    }
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Calls Exec with the arguments built up so far.
 *
 * Results:
 *	Process id of child process, -1 if there was an error.
 *
 * Side effects:
 *	Child process is forked.
 *
 *----------------------------------------------------------------------
 */

int
DoExec()
{
    int			pid;
    int			i;

    if (argCount >= MAX_EXEC_ARGS) {
	(void) fprintf(stderr, "Too many args to exec.\n");
	return -1;
    }
    argArray[argCount] = NULL;
    if (verbose) {
	for (i = 0; i < argCount; i++) {
	    printf("%s ", argArray[i]);
	}
	putchar('\n');
    }
    if (printOnly) {
	return 0;
    }
    pid = fork();
    if (pid == 0) {
	(void) execvp(routineName, argArray);
	(void) fprintf(stderr, "Exec failed.\n");
	(void) perror("");
	(void) _exit(EXEC_FAILED);
    } 
    return pid;
}

