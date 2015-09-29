/* 
 * suicide.c --
 *
 *	Test program that sends a signal to itself.  The point of this 
 *	program is (1) to ensure that the signal recipient is doing a 
 *	Sprite request when the signal is posted and (2) to see what 
 *	happens after the signal is processed.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/suicide/RCS/suicide.c,v 1.1 92/03/12 20:55:32 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int sigNum = SIGTERM;		/* which signal to send */

/* 
 * Additional options.
 */
#define NO_TRICKS	0	/* just send the signal */
#define USE_SIGSETMASK	1	/* block the signal with sigsetmask */
#define USE_SIGBLOCK	2	/* block the signal with sigblock */
#define CATCH_SIGNAL	3	/* catch the signal */
int option = NO_TRICKS;

void Handler();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Send the signal and then print something afterwards.
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
    int argCh;
    extern int optind;
    int sigMask;
    char *usage = "Usage: suicide [-b|-m|-h] [sigNum]\n";

    while ((argCh = getopt(argc, argv, "bmh")) != EOF) {
	switch (argCh) {
	case 'b':
	    option = USE_SIGBLOCK;
	    break;
	case 'm':
	    option = USE_SIGSETMASK;
	    break;
	case 'h':
	    option = CATCH_SIGNAL;
	    break;
	default:
	    fprintf(stderr, usage);
	    exit(1);
	    break;
	}
    }
    if (optind < argc) {
	sigNum = atoi(argv[optind]);
    }
    if (sigNum <= 0  || sigNum >= NSIG) {
	fprintf(stderr, "%d isn't a valid signal number.\n", sigNum);
	exit(1);
    }

    sigMask = sigmask(sigNum);
    switch (option) {
    case USE_SIGBLOCK:
	printf("sigblock(0x%x) => 0x%x\n", sigMask, sigblock(sigMask));
	break;
    case USE_SIGSETMASK:
	printf("sigsetmask(0x%x) => 0x%x\n", sigMask, sigsetmask(sigMask));
	break;
    case CATCH_SIGNAL:
	if (signal(sigNum, Handler) == BADSIG) {
	    perror("can't register handler");
	    exit(1);
	}
    }

    if (kill(getpid(), sigNum) < 0) {
	perror("kill");
	exit(1);
    }

    printf("My process id is %x.\n", getpid());
    exit(0);
}

void
Handler()
{
    printf("got signal\n");
}
