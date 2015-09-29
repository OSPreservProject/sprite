/* 
 * exitTest.c --
 *
 *	Test the exit system call.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

extern int errno;
static int errors;

#ifdef __STDC__
static void testExit(int exitCode);
#else
static void testExit();
#endif

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Fork off several child processes that exit with different
 *      exit codes.
 *
 * Results:
 *	Exits with a zero exit status if everything works properly,
 *      non-zero otherwise.
 *
 * Side effects:
 *	Forks several sub-processes which immediately exit.
 *      Prints a message to stderr if there are any problems.
 *
 *----------------------------------------------------------------------
 */

void
main(void)
{

    testExit(EXIT_SUCCESS);
    testExit(EXIT_FAILURE);
    testExit(99);
    testExit(255);
    if (errors != 0) {
	exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * testExit --
 *
 *	Fork off a child process that exits with the given exit code.
 *      Wait for the child to exit.  Check the return status and make
 *      sure that it matches the exit status.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *      Prints a message to stderr nad increments `errors' if there is
 *      an error.
 *
 *----------------------------------------------------------------------
 */

static void
testExit(exitCode)
    int exitCode;
{
    int w;
    int child;
    union wait ws;

    switch (child = fork()) {

    case 0:
	exit(exitCode);
	(void) fprintf(stderr, "Exit returned: %s\n", strerror(errno));
	abort();
	(void) fprintf(stderr, "Abort returned: %s\n", strerror(errno));
	for (;;) {
	    continue;
	}

    case -1:
	(void) fprintf(stderr, "Fork failed: %s\n", strerror(errno));
	++errors;
	break;

    default:
	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	if (ws.w_retcode != exitCode) {
	    (void) fprintf(stderr, "Exit returned %d, instead of %d.\n",
		ws.w_retcode, exitCode);
	    ++errors;
	}
	break;
    }
    return;
}

