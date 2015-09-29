/* 
 * debug.c --
 *
 *	Command to start a process but have it immediately enter
 *	the debugger, before executing any instructions.
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
static char rcsid[] = "$Header: /sprite/src/cmds/debug.prog/RCS/debug.c,v 1.1 88/10/28 09:04:26 ouster Exp Locker: mendel $ SPRITE (Berkeley)";
#endif not lint

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sig.h>
/*
 * Secret trap door into exec to make it put the process into debug state.
 */

extern int _execDebug;

main(argc, argv)
    int  argc;
    char *argv[];
{
    int	pid;
    Sig_Action	newAction;
    if (argc < 2) {
	fprintf(stderr, "Usage: %s command arg arg ...\n", argv[0]);
	exit(1);
    }
    pid = getpid();
    printf("Process %X: \"%s\"\n", pid, argv[1]);

    /*
     * Besure that SIG_DEBUG is set at the DEFAULT_ACTION.
     */
    newAction.action = SIG_DEFAULT_ACTION;
    newAction.sigHoldMask = 0;
    Sig_SetAction(SIG_DEBUG, &newAction, (Sig_Action *) NULL);
    _execDebug = 1;
    execvp(argv[1], &(argv[1]));

    /*
     * The exec failed.
     */

    fprintf(stderr, "Couldn't execute \"%s\": %s\n", argv[1], strerror(errno));
    exit(1);
}
