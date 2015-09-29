/* 
 * loop.c --
 *
 *	Simple program to do something interesting forever.  One use is to
 *	help track down memory leaks in the Sprite server.  Another is to
 *	test signals support.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/loop/RCS/loop.c,v 1.2 92/03/12 20:49:02 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <test.h>
#include <time.h>
#include <unistd.h>

/* 
 * If sigpause is used instead of sleep, there are two ways to go.  One is 
 * to set a one-time alarm each time through the loop.  The other is to set 
 * up a periodic timer exactly once.  If this flag is defined, then a 
 * periodic timer is used.
 */
Boolean usePeriodTimer = FALSE;

int sleepSecs = 1;		/* number of seconds to pause */
int ignoreSigint = 0;		/* block SIGINT if non-zero */
int sigintFlag = 0;		/* did we get a SIGINT */
int useSigPause = 0;		/* use alarm/sigpause instead of sleep */

jmp_buf jmpBuf;			/* setjmp state */

#define NO_HANDLER	0
#define SET_FLAG	1	/* handler just sets a flag */
#define DO_LONGJMP	2	/* handler does a longjmp */
int useHandler = NO_HANDLER;	/* register a handler for SIGINT? */

void FlagHandler();
void LongjmpHandler();
void CatchAlarm();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	In an infinite loop, print the time and sleep a bit.
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
    char *argv[];
{
    time_t now;
    int argChar;
    extern char *optarg;
    char *usageString = "usage: loop [-i|-h type] [-p [-t]] [-s seconds]\n";
    struct itimerval timer;	/* interval timer */

    /* 
     * Parse the command line.
     */
    while ((argChar = getopt(argc, argv, "s:ih:pt")) != EOF) {
	switch (argChar) {
	case 'i':
	    ignoreSigint = 1;
	    break;
	case 'h':
	    switch (*optarg) {
	    case 'f':
		useHandler = SET_FLAG;
		break;
	    case 'l':
		useHandler = DO_LONGJMP;
		break;
	    default:
		Test_PutMessage("handler type is 'f' or 'l'.\n");
		exit(1);
		break;
	    }
	    break;
	case 'p':
	    useSigPause = 1;
	    break;
	case 's':
	    sleepSecs = atoi(optarg);
	    break;
	case 't':
	    usePeriodTimer = TRUE;
	    break;
	default:
	    Test_PutMessage(usageString);
	    exit(1);
	    break;
	}
    }
    if (ignoreSigint && useHandler != NO_HANDLER) {
	Test_PutMessage(usageString);
	exit(1);
    }

    /* 
     * Ignore interrupts if requested.
     */
    if (ignoreSigint) {
	if (signal(SIGINT, SIG_IGN) == BADSIG) {
	    perror("can't ignore SIGINT");
	    exit(1);
	}
    } else {
	switch (useHandler) {
	case SET_FLAG:
	    if (signal(SIGINT, FlagHandler) == BADSIG) {
		perror("can't register FlagHandler");
		exit(1);
	    }
	    break;
	case DO_LONGJMP:
	    if (signal(SIGINT, LongjmpHandler) == BADSIG) {
		perror("can't register LongjmpHandler");
		exit(1);
	    }
	    break;
	}
    }

    if (setjmp(jmpBuf)) {
	Test_PutMessage("\nlongjmp\n");
    }

    if (useSigPause) {
	if (signal(SIGALRM, CatchAlarm) == BADSIG) {
	    perror("can't register alarm handler");
	    exit(1);
	}
	if (usePeriodTimer) {
	    timer.it_interval.tv_sec = sleepSecs;
	    timer.it_interval.tv_usec = 0;
	    timer.it_value = timer.it_interval;
	    if (setitimer(ITIMER_REAL, &timer, (struct itimerval *)0) < 0) {
		perror("Can't set interval timer");
		exit(1);
	    }
	}
    }

    /* 
     * Loop, printing the time every so often.
     */
    if (sleepSecs <= 0) {
	Test_PutMessage("Going into infinite loop.\n");
    }
    for (;;) {
	if (sleepSecs > 0) {
	    now = time(0);
	    Test_PutTime(now, TRUE);
	    if (useSigPause) {
		if (!usePeriodTimer) {
		    alarm(sleepSecs);
		}
		(void)sigpause(0);
	    } else {
		sleep(sleepSecs);
	    }
	}
	if (sigintFlag) {
	    Test_PutMessage("\nsigint\n");
	    sigintFlag = 0;
	}
    }
}

void
FlagHandler(sigNum, code, contextPtr, addr)
    int sigNum, code;
    char *contextPtr, *addr;
{
#ifdef lint
    sigNum = sigNum;
    code = code;
    contextPtr = contextPtr;
    addr = addr;
#endif
#if 0
    printf("\n(%d, %d, 0x%x, 0x%x)\n", sigNum, code, contextPtr, addr);
#endif
    sigintFlag = 1;
}

void
LongjmpHandler()
{
    longjmp(jmpBuf, 1);
}

void
CatchAlarm()
{
}
