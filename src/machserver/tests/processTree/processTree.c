/* 
 * processTree.c --
 *
 *	Test program to do lots of forks and execs.  Basically the program 
 *	spawns a small tree of processes, in which a process forks off some 
 *	children, waits a random interval, then exits.
 *	
 *	Usage: processTree [ numSecs ]
 *	
 *	where "numSecs" is the minimum number of seconds to run.  (The 
 *	default is to run forever.)  After numSecs seconds the top-level 
 *	process stops creating new processes, and eventually the program 
 *	exits. 
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/tests/processTree/RCS/processTree.c,v 1.5 92/04/29 22:32:45 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <errno.h>
#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <test.h>
#include <unistd.h>

#define USE_STDIO		/* use printf instead of Test_PutTime */

#define NUM_CHILDREN_1ST	2 /* number of immediate children to keep 
				   * running */
#define MAX_CHILDREN_2ND	10 /* maximum number of children to be 
				    * created by a first-generation child */
#define MAX_PRINTFS		5 /* maximum number of printfs for a 
				   * process to do before exiting */
#define MAX_SLEEP_TIME		20 /* maximum number of seconds to sleep 
				    * between printf's */

extern int FromInterval();

/* Forward references: */

static void BeAChild();
static int Lookup();
static pid_t MakeChild();
static void PrintfLoop();
static void PrintTime();
static void ReapChildren();
static void SleepSome();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Run forever; start a new top-level child when one exits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    char **argv;
{
    int child;
    pid_t children[NUM_CHILDREN_1ST];
    pid_t deadDescendant;
    time_t stopTime = 0;	/* if non-zero, don't create new processes 
				 * after this time */

    /* 
     * Initialization.
     */
    for (child = 0; child < NUM_CHILDREN_1ST; ++child) {
	children[child] = MakeChild(TRUE);
    }
    if (argc > 1) {
	stopTime = time((time_t *)0) + atoi(argv[1]);
    }

    /* 
     * When a direct descendant dies, start a replacement, up until the
     * stopping time (if any).  If we're running as the initial process, we
     * inherit 2nd generation processes as well.  When those die, take no
     * additional action.
     */
    
    for (;;) {
	deadDescendant = wait((union wait *)0);
	if (deadDescendant < 0) {
	    if (stopTime == 0 || time((time_t *)0) < stopTime) {
		perror("Wait at top level failed");
		exit(1);
	    }
	    exit(0);
	}
	child = Lookup(deadDescendant, children);
	if (child != -1) {
	    if (stopTime == 0 || time((time_t *)0) < stopTime) {
		children[child] = MakeChild(TRUE);
	    }
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * MakeChild --
 *
 *	Make a child process.
 *
 * Results:
 *	In the parent, returns the pid of the child.  In the child, never 
 *	returns.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static pid_t
MakeChild(moreChildren)
    Boolean moreChildren;	/* should the child make addition children? */
{
    pid_t childPid;

    childPid = fork();
    switch (childPid) {
    case -1:
	perror("Can't create child");
	exit(1);
	break;
    case 0:
	BeAChild(moreChildren);
	exit(0);
	break;
    }

    return childPid;
}


/*
 *----------------------------------------------------------------------
 *
 * BeAChild --
 *
 *	Create some number of child processes if asked to do so.  Then do a
 *	random number of printfs, sleeping a random time between printfs
 *	and checking for dead children.  Then return.
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
BeAChild(makeChildren)
    Boolean makeChildren;
{
    int numChildren;
    int child;

    /* 
     * Reinitialize the random number generator, using our pid.  Otherwise, 
     * all the processes run in lock step.
     */
    srandom(getpid());

    /* 
     * Even though we're supposed to make children, we don't want any 
     * grandchildren. 
     */
    if (makeChildren) {
	numChildren = FromInterval(1, MAX_CHILDREN_2ND);
	for (child = 0; child < numChildren; ++child) {
	    (void)MakeChild(FALSE);
	}
    }

    PrintfLoop(makeChildren);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintfLoop --
 *
 *	Do a random number of printfs over a random time interval.  Each 
 *	time through the loop, reap any children that have died.
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
PrintfLoop(expectChildren)
    Boolean expectChildren;	/* should there be children to reap? */
{
    int numMessages = FromInterval(1, MAX_PRINTFS);
    int msg;			/* message (printf) number */

    for (msg = 0; msg < numMessages; ++msg) {
	PrintTime();
	SleepSome();
	if (expectChildren) {
	    ReapChildren();
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrintTime --
 *
 *	Backspace over the previous time message and write the current 
 *	time.
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
PrintTime()
{
    time_t now;
#ifdef USE_STDIO
    char buf[26];
#endif

    now = time((time_t *)0);
#ifdef USE_STDIO
    strcpy(buf, ctime(&now));
    buf[24] = '\0';
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%s",
	   buf);
    fflush(stdout);
#else
    Test_PutTime(now, TRUE);
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * SleepSome --
 *
 *	Pick a random number of seconds and sleep for that long.
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
SleepSome()
{
    int numSecs;

    numSecs = FromInterval(1, MAX_SLEEP_TIME);
    sleep(numSecs);
}


/*
 *----------------------------------------------------------------------
 *
 * ReapChildren --
 *
 *	Reap any dead children.  If there aren't any currently, return.
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
ReapChildren()
{
    pid_t childPid;

    for (;;) {
	childPid = wait3((union wait *)0, WNOHANG, (struct rusage *)0);
	/* no children died */
	if (childPid == 0) {
	    break;
	}
	/* all our children died before we did */
	if (childPid < 0 && errno == ECHILD) {
	    break;
	}
	if (childPid < 0) {
	    perror("Wait for child failed");
	    exit(1);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Lookup --
 *
 *	Find the given child in the table of children.
 *
 * Results:
 *	Returns the table index for the child, or -1 if the child isn't in 
 *	the table.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
Lookup(child, childTable)
    pid_t child;
    pid_t *childTable;		/* table with NUM_CHILDREN_1ST entries */
{
    int index;

    for (index = 0; index < NUM_CHILDREN_1ST; ++index) {
	if (childTable[index] == child) {
	    return index;
	}
    }

    return -1;
}

