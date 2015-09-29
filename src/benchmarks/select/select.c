/* 
 * select.c --
 *
 *	This file contains a benchmark program to time "select"
 *	calls.  It should be invoked in the following way:
 *
 *	select numPipes numReady count
 *
 *	The program will create numPipes pipes, put data in the
 *	first numReady of them, then do count selects on all of
 *	the pipes with no timeout.  It will print out the average
 *	time for each select operation.
 *
 * Copyright 1989 Regents of the University of California.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: protoPub.c,v 1.3 87/01/04 17:28:56 andrew Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

main(argc, argv)
    int argc;
    char **argv;
{
#define MAXPIPES 15
    int count, numPipes, numReady, i;
    int readMask, readMask2, result, numFds;
    int pipeStreams[2*MAXPIPES];
    struct timeval start, stop;
    struct rusage begin ,end;
    static struct timeval timeout = {0,0};
    struct timezone tz;
    int micros;
    double timePer;

    if (argc != 4) {
	fprintf(stderr, "Usage: select numPipes numReady count\n");
	exit(1);
    }

    numPipes = atoi(argv[1]);
    if (numPipes > MAXPIPES) {
	fprintf(stderr, "Don't ask for more than %d pipes\n", MAXPIPES);
	exit(1);
    }
    numReady = atoi(argv[2]);
    if ((numReady < 0) || (numReady > numPipes)) {
	fprintf(stderr,
	    "Number of ready pipes must be between 0 and %d\n",
	    numPipes);
	exit(1);
    }
    count = atoi(argv[3]);

    for (i = 0, readMask = 0, numFds = 0; i < numPipes; i++) {
	if (pipe(&pipeStreams[2*i]) == -1) {
	    fprintf(stderr, "Couldn't create pipes\n");
	    exit(1);
	}
	readMask |= 1 << pipeStreams[2*i];
	if (pipeStreams[2*i] >= numFds) {
	    numFds = pipeStreams[2*i] + 1;
	}
	if (i < numReady) {
	    char c;

	    c = 0;
	    if (write(pipeStreams[2*i + 1], &c, 1) != 1) {
		fprintf(stderr, "Couldn't write to pipe\n");
		exit(1);
	    }
	}
    }

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif
    for (i = 0; i < count; i++) {
	readMask2 = readMask;
	result = select(numFds, &readMask2, 0, 0, &timeout);
    }
#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &end);
    micros = (end.ru_utime.tv_sec + end.ru_stime.tv_sec
	    - begin.ru_utime.tv_sec - begin.ru_stime.tv_sec)*1000000
	    + (end.ru_utime.tv_usec - begin.ru_utime.tv_usec)
	    + (end.ru_stime.tv_usec - begin.ru_stime.tv_usec);
#else
    gettimeofday(&stop, (struct timezone *) NULL);
    micros = 1000000*(stop.tv_sec - start.tv_sec)
	    + stop.tv_usec - start.tv_usec;
#endif
    timePer = micros;
    printf("Time per iteration: %.2f microseconds\n", timePer/count);
    printf("Last result was %d\n", result);
}
