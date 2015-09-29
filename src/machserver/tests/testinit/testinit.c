/* 
 * testinit.c --
 *
 *	Test initial user program.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/testinit/RCS/testinit.c,v 1.13 92/03/23 15:10:43 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <cfuncproto.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <test.h>
#include <unistd.h>

#define CONSOLE	"/dev/console"	/* name of console device */

extern void msleep();
static void SetID _ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	For now, just call a shell.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
int
main(argc, argv)
    int argc;
    char **argv;
{
    pid_t pid;
    char *shellName = "cmds.sprited/psh";
    int i;
    union wait exitStatus;

    Test_PutDecimal(argc);
    Test_PutMessage(" arguments: \n");
    for (i = 0; i < argc; i++) {
	Test_PutMessage(argv[i]);
	Test_PutMessage("\n");
    }
    if (argc >= 2) {
	shellName = argv[1];
    }

    /* 
     * Open the console, which will enable use of stdio.  If that fails, 
     * bail out.
     */
    if ((open(CONSOLE, O_RDONLY, 0) != 0)
	    || (open(CONSOLE, O_WRONLY, 0) != 1)
	    || (open(CONSOLE, O_WRONLY, 0) != 2)) {
	Test_PutMessage("Couldn't open console: ");
	Test_PutMessage(strerror(errno));
	Test_PutMessage(".\n");
	exit(2);
    }

/* 
 * Redefine the Test_ calls to use stdio.
 */
#include <testRedef.h>

    /* 
     * cd to a test directory, then start up the shell or whatever program 
     * we're suppposed to run.
     */
    if (chdir("/users/kupfer") < 0) {
	Test_PutMessage("Can't chdir to /users/kupfer: ");
	Test_PutMessage(strerror(errno));
	Test_PutMessage("\n");
	goto bailOut;
    }

    pid = fork();
    if (pid < 0) {
	Test_PutMessage("Fork failed: ");
	Test_PutMessage(strerror(errno));
	Test_PutMessage("\n");
	goto bailOut;
    } 
    if (pid == 0) {
	char *args[2];

	/* Change the user ID to be non-root. */
	SetID();

	args[0] = shellName;
	args[1] = 0;
#if 0
	Test_PutMessage("I'm the child (pid ");
	Test_PutHex(getpid());
	Test_PutMessage(")\n");
#endif
	if (execv(shellName, args) < 0) {
	    Test_PutMessage("Couldn't exec `");
	    Test_PutMessage(shellName);
	    Test_PutMessage("': ");
	    Test_PutMessage(strerror(errno));
	    Test_PutMessage("\n");
	    exit(1);
	}
	exit(0);
    } else {
#if 0
	Test_PutMessage("Child is pid ");
	Test_PutHex(pid);
	Test_PutMessage("\n");
#endif
	pid = wait(&exitStatus);
	Test_PutMessage("process ");
	Test_PutHex(pid);
	Test_PutMessage(" finished, code ");
	Test_PutDecimal(exitStatus.w_retcode);
	if (exitStatus.w_termsig != 0) {
	    Test_PutMessage(", signal ");
	    Test_PutDecimal(exitStatus.w_termsig);
	}
	Test_PutMessage(".\n");
    }

 bailOut:
    (void)Sys_Shutdown(SYS_HALT|SYS_KILL_PROCESSES|SYS_WRITE_BACK, NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * SetID --
 *
 *	Set the user ID of the process to something less potentially 
 *	harmful than root.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SetID()
{
    struct passwd *passwdEntPtr; /* password entry for designated user ID */
    char *user = "kupfer";
    uid_t expectedUid = 891;

    passwdEntPtr = getpwnam(user);
    if (passwdEntPtr == NULL) {
	fprintf(stderr, "Warning: couldn't find passwd entry for %s.\n",
		user);
    } else if (passwdEntPtr->pw_uid != expectedUid) {
	fprintf(stderr, "Warning: expected uid %d for %s, got %d.\n",
		expectedUid, user, passwdEntPtr->pw_uid);
    }

    if (setuid(expectedUid) < 0) {
	fprintf(stderr, "Couldn't change user ID to %d: %s\n",
		expectedUid, strerror(errno));
    }
}
