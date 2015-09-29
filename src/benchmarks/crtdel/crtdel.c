/* 
 * crtdel.c --
 *
 *	This program is a stand-alone benchmark that measures
 *	the cost of doing the following:  create a file,
 *	write some bytes to it, close the file, open in read
 *	mode, read all the bytes, close the file, delete the
 *	file.  It should be invoked as follows:
 *
 *	crtdel fileName kBytes count
 *
 *	"fileName" is the name of the file, "kBytes" tells
 *	how many kbytes to write to it, and "count" tells how
 *	many times to repeat the experiment.
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
static char rcsid[] = "$Header: /sprite/src/benchmarks/crtdel/RCS/crtdel.c,v 1.3 89/09/15 11:22:00 brent Exp $ SPRITE (Berkeley)";
#endif not lint


#include <stdio.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>

static char buffer[16384];

main(argc, argv)
int argc;
char **argv;
{
    int cnt, repeats, kBytes, fd, count, i;
    double msPer, micros;
    struct rusage begin ,end;
    struct timeval start, stop;
    struct timezone tz;

    if (argc != 4) {
	fprintf(stderr, "Usage:  %s fileName kBytes count\n",
		argv[0]);
	exit(1);
    }
    kBytes = atoi(argv[2]);
    repeats = atoi(argv[3]);

#ifdef GETRUSAGE
    getrusage(RUSAGE_SELF, &begin);
#else
    gettimeofday(&start, (struct timezone *) NULL);
#endif

    for (count = 0 ; count < repeats; count++) {
	fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0700);
	if (fd < 0) {
	    fprintf(stderr, "Couldn't create %s.\n", argv[1]);
	    exit(1);
	}
	for (i = kBytes; i > 0; ) {
	    if (i > 16) {
		if (write(fd, buffer, 16384) < 0) {
		    fprintf(stderr, "Error while writing.\n");
		    exit(1);
		}
		i -= 16;
	    } else {
		if (write(fd, buffer, 1024*i - 1) < 0) {
		    fprintf(stderr, "Error while writing.\n");
		    exit(1);
		}
		i = 0;
	    }
	}
	if (close(fd) != 0) {
	    fprintf(stderr, "Error closing %s after writing.\n",
		    argv[1]);
	    exit(1);
	}
	fd = open(argv[1], O_RDONLY, 0700);
	if (fd < 0) {
	    fprintf(stderr, "Couldn't open %s to read it back.\n",
		    argv[1]);
	    exit(1);
	}
	while (1) {
	    cnt = read(fd, buffer, 16384);
	    if (cnt < 16384) break;
	}
	if (close(fd) != 0) {
	    fprintf(stderr, "Error closing %s after reading.\n",
		    argv[1]);
	    exit(1);
	}
	if(unlink(argv[1]) != 0) {
	    fprintf(stderr, "Error removing %s.\n", argv[1]);
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
    printf("Time per iteration: %.2f milliseconds\n", msPer);
}
