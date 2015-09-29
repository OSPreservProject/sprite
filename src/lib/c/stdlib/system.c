/* 
 * system.c --
 *
 *	Source code for the "system" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/system.c,v 1.6 89/03/22 00:47:35 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

/*
 *----------------------------------------------------------------------
 *
 * system --
 *
 *	Pass a string off to "sh", and return the result of executing
 *	it.
 *
 * Results:
 *	The return value is the status returned by "sh".
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
system(command)
    char *command;		/* Shell command to execute. */
{
    int pid, pid2, result;
    struct sigvec quitHandler, intHandler;
    static struct sigvec newHandler = {SIG_IGN, 0, 0};
    union wait status;

    sigvec(SIGINT, &newHandler, &intHandler);
    sigvec(SIGQUIT, &newHandler, &quitHandler);

    pid = fork();
    if (pid == 0) {
	execlp("sh", "sh", "-c", command, 0);
	_exit(127);
    }
    while (1) {
	pid2 = wait(&status);
	if (pid2 == -1) {
	    result = -1;
	    break;
	}
	if (pid2 == pid) {
	    result = status.w_status;
	    break;
	}
    }
    sigvec(SIGINT, &intHandler, (struct sigvec *) 0);
    sigvec(SIGQUIT, &quitHandler, (struct sigvec *) 0);
    return result;
}
