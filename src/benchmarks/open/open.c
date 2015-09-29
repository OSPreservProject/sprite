/* 
 * open.c --
 *
 *	This program is a stand-alone benchmark that measures
 *	the cost of opening and closing a file.
 *
 *	open fileName count
 *
 *	where "fileName" is the name of the file to open and
 *	close and "count" tells how many open/close pairs to
 *	execute.
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
static char rcsid[] = "$Header: /sprite/src/benchmarks/open/open.c,v 1.1 89/09/13 20:48:43 ouster Exp $ SPRITE (Berkeley)";
#endif not lint


#include <stdio.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>

main(argc, argv)
int argc;
char **argv;
{
    int repeats, fd, count;
    double msPer, micros;
    struct rusage begin ,end;
    struct timeval start, stop;
    struct timezone tz;

    if (argc != 3) {
	fprintf(stderr, "Usage:  %s fileName count\n",
		argv[0]);
	exit(1);
    }
    repeats = atoi(argv[2]);

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif

    for (count = 0 ; count < repeats; count++) {
	fd = open(argv[1], O_RDONLY, 0);
	if (fd < 0) {
	    fprintf(stderr, "Couldn't open %s.\n", argv[1]);
	    exit(1);
	}
	if (close(fd) != 0) {
	    fprintf(stderr, "Error closing %s.\n",
		    argv[1]);
	    exit(1);
	}
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
    msPer = (micros/repeats/1000.0);
    printf("Time per open/close pair: %.2f milliseconds\n", msPer);
}
