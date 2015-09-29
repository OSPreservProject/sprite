/* 
 * gettimeofday.c --
 *
 *	This file is a benchmark program to measure the cost of a
 *	minimal kernel call (gettimeofday).  It should be invoked as follows:
 *	
 *	gettimeofday [-n] count
 *
 *	Where count is the number of calls to make.  It makes that
 *	many calls, then prints out the average time per call.  If -n is 
 *	specified, then the gettimeofday calls don't actually ask for the 
 *	time.  The difference between a run with -n and a run without -n 
 *	should be the time to copy the current time-of-day to user space.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/benchmarks/gettimeofday/RCS/gettimeofday.c,v 1.3 92/05/21 20:11:10 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

main(argc, argv)
    int argc;
    char **argv;
{
    int count, i;
#ifdef GETRUSAGE
    struct rusage begin ,end;
#endif
    struct timeval start, stop, dummy;
    struct timeval *dummyPtr;
    int micros;
    double timePer;
    char *usage = "Usage: gettimeofday [-n] count\n";

    if (argc < 2) {
	fprintf(stderr, usage);
	exit(1);
    }

    if (argc > 2 && strcmp(argv[1], "-n") == 0) {
	dummyPtr = NULL;
	--argc;
	++argv;
    } else if (argc > 2) {
	fprintf(stderr, usage);
	exit(1);
    } else {
	dummyPtr = &dummy;
    }
    count = atoi(argv[1]);

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif

    for (i = 0; i < count; i++) {
        (void) gettimeofday(dummyPtr, (struct timezone *) NULL);
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
}
