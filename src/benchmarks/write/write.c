/* 
 * write.c --
 *
 *	This program is a stand-alone benchmark to measure
 *	the speed of writing a file.  It should be invoked
 *	as follows:
 *
 *	write numK count
 *
 *	The program will write numK bytes to its standard
 *	output, seeking back to zero and repeating count
 *	times.  Then it will print out the write bandwidth.
 *	If omitted, count defaults to 1.
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
static char rcsid[] = "$Header: /sprite/src/benchmarks/write/RCS/write.c,v 1.5 89/09/13 20:48:26 ouster Exp Locker: brent $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

static char buffer[16384];

main(argc, argv)
int argc;
char **argv;
{
    int total, repeats, numK, i;
    double rate, micros;
    struct rusage begin ,end;
    struct timeval start, stop;
    struct timezone tz;

    if (argc < 2) {
	fprintf(stderr, "Usage: %s kBytes [count]\n", argv[0]);
	exit(1);
    }
    numK = atoi(argv[1]);

    if (argc == 2) {
	repeats = 1;
    } else {
	repeats = atoi(argv[2]);
    }
    total = 0;

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif

    for ( ; repeats > 0; repeats--) {
	lseek(1, 0, 0);
	for (i = numK; i > 0; ) {
	    if (i > 16) {
		total += write(1, buffer, 16384);
		i -= 16;
	    } else {
		total += write(1, buffer, 1024*i - 1);
		i = 0;
	    }
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
    rate = total/(micros/1000000.0);
    fprintf(stderr, "%d bytes written at %.0f bytes/sec.\n", total, rate);
}
