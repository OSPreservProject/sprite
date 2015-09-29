/* 
 * mpping --
 *
 *	A multi-processor test program where a bunch of processes
 *	pass a token between themselves in a ring-like fashion.
 *	Command-line syntax:
 *
 *	mpping numProcs loopCount
 *
 *	"numProcs" is the number of processes to use (anything >= 1,
 *	defaults to 4).  "loopCount" is the number of iterations to
 *	run between time printouts (defaults to 1000).
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
static char rcsid[] = "$Header: proto.c,v 1.2 88/03/11 08:39:08 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <errno.h>
#include <stdio.h>
#include <proc.h>
#include <signal.h>
#include <status.h>
#include <string.h>
#include <sys/time.h>

/*
 * The data structure below represents the ring around which the token
 * gets passed.  The token gets passed by each process setting the
 * "myTurn" field for the next process.
 */

typedef struct {
    int pid;		/* Identifier of this process. */
    int myTurn;		/* Non-zero means previous process in ring has
			 * signalled this process;  this process must
			 * wait until this becomes non-zero. */
    int signalMe;	/* Non-zero means that this process got tired
			 * of spinning on myTurn and went to sleep.  The
			 * predecessor process should wake it up with a
			 * SIGUSR1. */
    int pad[5];		/* Fill out cache line so that each process's
			 * entry is in a different line. */
} Process;

#define MAX_PROCESSES 100

volatile Process procs[MAX_PROCESSES];
int numProcs = 4;

int numToTime = 1000;	/* Number of iterations to time at once. */

/*
 * Forward declarations for procedures defined in this file:
 */

extern void Ping();
extern int SigUsr1();

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main procedure:  fork off children.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates children, which do the pinging.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char **argv;
{
    int i, myIndex;
    ReturnStatus status;
    struct timeval start, stop;
    struct timezone tz;
    int micros;
    double timePer;
    static struct sigvec vec = {SigUsr1, 0, 0};

    if (argc > 1) {
	numProcs = atoi(argv[1]);
	if ((numProcs <= 0) || (numProcs > MAX_PROCESSES)) {
	    fprintf(stderr, "Number of processes must be between %d and %d.\n",
		    1, MAX_PROCESSES);
	    exit(1);
	}
    }
    if (argc > 2) {
	numToTime = atoi(argv[2]);
	if (numToTime <= 0) {
	    fprintf(stderr, "Loop count must be greater than zero.\n");
	    exit(1);
	}
    }
    if (argc > 3) {
	fprintf(stderr, "Usage: %s [numProcs [loopCount]]\n", argv[0]);
	exit(1);
    }

    /*
     * Fork off child processes and initialize ping array.
     */

    for (i = numProcs-1; i > 0; i--) {
	procs[i].myTurn = procs[i].signalMe = 0;
	status = Proc_Fork(1, &procs[i].pid);
	if (status == PROC_CHILD_PROC) {
	    myIndex = i;
	    goto mainLoop;
	}
	if (status != SUCCESS) {
	    fprintf(stderr, "Error 0x%x in forking.", status);
	    goto killProcs;
	}
    }
    myIndex = 0;
    procs[0].pid = getpid();
    procs[0].myTurn = 1;
    procs[0].signalMe = 0;

    /*
     * Main loop:  ping continually.  Process 0 times the execution
     * and prints out the times.
     */

    mainLoop:
    if (sigvec(SIGUSR1, &vec, (struct sigvec *) NULL) != 0) {
	fprintf(stderr, "Could enable signal handler: %s\n",
		strerror(errno));
	goto killProcs;
    }
    sigblock(sigmask(SIGUSR1));
    i = 1;
    if (myIndex == 0) {
	gettimeofday(&start, &tz);
    }
    while (1) {
	Ping(myIndex);
	if ((myIndex == 0) && (i >= numToTime)) {
	    i = 1;
	    gettimeofday(&stop, &tz);
	    micros = (stop.tv_sec - start.tv_sec)*1000000
		    + (stop.tv_usec - start.tv_usec);
	    timePer = micros;
	    printf("Time per iteration: %.2f microseconds\n",
		    timePer/numToTime);
	    start = stop;
	} else {
	    i += 1;
	}
    }

    /*
     * Desperation:  if there's a major problem, come here to kill off
     * all of the processes.
     */

    killProcs:
    procs[myIndex].pid = 0;
    for (i = 0; i < numProcs; i++) {
	if (procs[i].pid != 0) {
	    kill(procs[i].pid, SIGTERM);
	}
    }
    exit(1);
}

/*
 *----------------------------------------------------------------------
 *
 * Ping --
 *
 *	Wait for this process to be pinged once, then ping the next
 *	process in the chain.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This process gets delayed, and the next process get woken up.
 *
 *----------------------------------------------------------------------
 */

void
Ping(index)
    int index;		/* Index of this process in the table "procs". */
{
    register Process *p;
    int i;
#ifdef DEBUG
    char msg[200];
#endif

    p = &procs[index];

    /*
     * Wait for a while to get pinged.  If nothing happens, then go to
     * sleep until re-awoken.
     */

    while (1) {
	for (i = 0; i < 200; i++) {
	    if (p->myTurn) {
		goto pinged;
	    }
	}
	p->signalMe = 1;
	if (!p->myTurn) {
#ifdef DEBUG
	    sprintf(msg, "Process %d (%x) going to sleep.\n", index, p->pid);
	    write(1, msg, strlen(msg));
#endif
	    sigpause(0);
	}
    }

    /*
     * We got the token.  Now give it to the next process.  Wake the
     * process up if it went to sleep.
     */

    pinged:
#ifdef DEBUG
    sprintf(msg, "Process %d (%x) got token.\n", index, p->pid);
    write(1, msg, strlen(msg));
#endif
    p->myTurn = 0;
    index += 1;
    if (index >= numProcs) {
	index = 0;
    }
    p = &procs[index];
    p->myTurn = 1;
    if (p->signalMe) {
#ifdef DEBUG
	sprintf(msg, "Waking up process %d (%x).\n", index, p->pid);
	write(1, msg, strlen(msg));
#endif
	p->signalMe = 0;
	kill(p->pid, SIGUSR1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SigUsr1 --
 *
 *	This is the interrupt handler for SIGUSR1 signals.  It doesn't
 *	do anything (all that matters is that we got the signal).
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
SigUsr1()
{
    return 0;
}
