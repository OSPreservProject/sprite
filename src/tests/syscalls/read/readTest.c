/* 
 * fileTest.c --
 *
 *	Test the read, write, open, close and lseek system call.
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


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *
 * Results:
 *	Exits with a zero exit status if everything works properly,
 *      non-zero otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
main(void)
{







    exit((errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}



static void
testClose()
{
    int fd;


    fd = open(tempfile) ...


    write(fd ....)
	if (lseek(.... )) ...







    if (close(fd)) {
	(void) fprintf(stderr, "close of %s failed: %s\n",
	    tempfile, strerror(errno));
	++errors;
	return;
    }
    if (close(fd) != -1) {
	(void) fprintf(stderr,
	    "Close of already closed descriptor succeeded\n");
	++errors;
	return;
    } else if (errno != EBADF) {
	    (void) fprintf(stderr,
		"Bad errno (%d): %s\n", errno, strerror(errno));
	    ++errors;
	    return;
	}
    }
    if (write(fd, buffer, sizeof(buffer)) != -1) {
	(void) fprintf(stderr, "Write to closed file succeeded.\n");
	++errors;
	return;
    } else if (errno != EBADF) {
	    (void) fprintf(stderr,
		"Bad errno (%d): %s\n", errno, strerror(errno));
	    ++errors;
	}
    }
    if (read(fd, buffer, sizeof(buffer)) != -1) {
	(void) fprintf(stderr, "Read from closed file succeeded.\n");
	++errors;
	return;
    } else if (errno != EBADF) {
	    (void) fprintf(stderr,
		"Bad errno (%d): %s\n", errno, strerror(errno));
	    ++errors;
	}
    }
    return;
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

