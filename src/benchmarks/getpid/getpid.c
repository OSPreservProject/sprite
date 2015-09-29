/* 
 * getpid.c --
 *
 *	This file is a benchmark program to measure the cost of a
 *	minimal kernel call (getpid).  It should be invoked as follows:
 *	
 *	getpid count
 *
 *	Where count is the number of calls to make.  It makes that
 *	many calls, then prints out the average time per call.
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
static char rcsid[] = "$Header: /sprite/src/benchmarks/getpid/RCS/getpid.c,v 1.1 89/08/31 13:19:47 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

main(argc, argv)
    int argc;
    char **argv;
{
    int count, i;
    struct rusage begin ,end;
    struct timeval start, stop;
    struct timezone tz;
    int micros;
    double timePer;

    if (argc != 2) {
	fprintf(stderr, "Usage: getpid count\n");
	exit(1);
    }

    count = atoi(argv[1]);

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif

    for (i = 0; i < count; i++) {
        (void) getpid();
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
