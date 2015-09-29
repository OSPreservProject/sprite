/* 
 * read.c --
 *
 *	This program is a stand-alone benchmark to measure
 *	the speed of reading a file.  It should be invoked
 *	as follows:
 *
 *	read [count]
 *
 *	The program will read its standard input file count
 *	times from start to finish, then print out the speed
 *	of reading in bytes/sec.  If omitted, count defaults
 *	to 1.
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
static char rcsid[] = "$Header: /sprite/src/benchmarks/read/RCS/read.c,v 1.6 89/09/13 20:47:51 ouster Exp $ SPRITE (Berkeley)";
#endif not lint


#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

static char buffer[16384];

main(argc, argv)
int argc;
char **argv;
{
    int cnt, total, repeats;
    double rate, micros;
    struct rusage begin ,end;
    struct timeval start, stop;
    struct timezone tz;

    if (argc == 1) {
	repeats = 1;
    } else {
	repeats = atoi(argv[1]);
    }
    total = 0;

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif

    for ( ; repeats > 0; repeats--) {
	lseek(0, 0, 0);
	while (1) {
	    cnt = read(0, buffer, 16384);
	    total += cnt;
	    if (cnt < 16384) break;
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
    printf("%d bytes read at %.0f bytes/sec.\n", total, rate);
}
