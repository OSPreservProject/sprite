/* 
 * bcopy.c --
 *
 *	Benchmark program to measure memory bandwidth.
 *	Invoke with no arguments.  It will make repeated
 *	calls to bcopy with different size buffers and
 *	print memory bandwidth as a function of buffer
 *	size.
 *	
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
static char rcsid[] = "$Header: /sprite/src/benchmarks/bcopy/RCS/bcopy.c,v 1.2 90/03/30 14:45:55 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

/*
 * Different amounts to copy at once:
 */

int sizes[] =
	{10, 50, 100, 500, 1000, 5000, 10000, 50000, 100000, 200000, -1};

#define MAX_AT_ONCE 500000
char src[MAX_AT_ONCE], dst[MAX_AT_ONCE];

main(argc, argv)
    int argc;
    char **argv;
{
    register int *sizePtr;
    struct rusage begin, end;
    struct timeval start, stop;
    int micros, totalBytes, bytesSoFar, count;
    double mbSec;

    if (argc > 1) {
	totalBytes = 1000000*atoi(argv[1]);
    } else {
	totalBytes = 5000000;
    }

    for (sizePtr = sizes; *sizePtr > 0; sizePtr++) {
#ifdef GETRUSAGE
	getrusage(RUSAGE_SELF, &begin);
#else
	gettimeofday(&start, (struct timezone *) NULL);
#endif

	for (bytesSoFar = 0; bytesSoFar < totalBytes;
		bytesSoFar += count) {
	    count = totalBytes - bytesSoFar;
	    if (count > *sizePtr) {
		count = *sizePtr;
	    }
	    bcopy(src, dst, count);
	}
#ifdef GETRUSAGE
	getrusage(RUSAGE_SELF, &end);
	micros = (end.ru_utime.tv_sec
		- begin.ru_utime.tv_sec)*1000000
		+ (end.ru_utime.tv_usec - begin.ru_utime.tv_usec);
#else
	gettimeofday(&stop, (struct timezone *) NULL);
	micros = 1000000*(stop.tv_sec - start.tv_sec)
		+ stop.tv_usec - start.tv_usec;
#endif

	mbSec = totalBytes;
	mbSec /= micros;
	printf("Transfer size: %d, Mbytes/sec: %.2f\n", *sizePtr,
		mbSec);
    }
}
