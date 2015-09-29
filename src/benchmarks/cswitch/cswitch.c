/* 
 * cswitch.c --
 *
 *	Simple program to test context switching by sending short
 *	messages back and forth through a pipe.  Invocation:
 *	
 *	cswitch count
 *
 *	Count tells how many times a one-byte message should be sent
 *	back and forth between two processes.
 *
 * Copyright 1987, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/benchmarks/cswitch/RCS/cswitch.c,v 1.1 89/08/31 13:19:37 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <sys/time.h>

main(argc,argv)
    int argc;
    char **argv;
{
    int count, i, toSlave[2], fromSlave[2];
    int pid;
    char buffer[10];
    struct timeval begin ,end;
    int micros;
    double timePer;
    extern int errno;

    if (argc != 2 ) {
	fprintf(stderr, "Ping takes one argument: count.\n");
	return;
    }
    count = 0;
    sscanf(argv[1], "%d", &count);
    pipe(toSlave);
    pipe(fromSlave);
    pid = fork();
    if (pid == 0) {
	write(fromSlave[1], "a", 1);
	while (1) {
	    read(toSlave[0], buffer, 1);
	    if (buffer[0] != 'a') {
		exit(0);
	    }
	    write(fromSlave[1], "a", 1);
	}
    }
    if (pid == -1) {
	fprintf(stderr, "Couldn't fork slave process: error %d.\n",
		errno);
	return;
    }
    read(fromSlave[0], buffer, 1);
    gettimeofday(&begin, (struct timezone *) NULL);
    for (i = 0; i < count; i++) {
	write(toSlave[1], "a", 1);
	read(fromSlave[0], buffer, 1);
    }
    gettimeofday(&end, (struct timezone *) NULL);
    micros = 1000000*(end.tv_sec - begin.tv_sec)
	    + end.tv_usec - begin.tv_usec;
    write(toSlave[1], "b", 1);
    timePer = micros;
    timePer /= count * 1000;

    printf("Elapsed time per ping (2 context switches): %.2f milliseconds.\n",
	    timePer);
}
