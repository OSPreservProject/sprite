/* $Header: /sprite/src/benchmarks/fork/RCS/fork.c,v 1.2 92/04/07 12:22:39 kupfer Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#ifdef sprite
#include "proc.h"
#endif


main(argc, argv)
    int argc;
    char *argv[];
{
    register 	int 	i;
    struct timeval startTime, endTime;
    int		pid;
    int		numForks = 1000; /* number of times to fork */

    if (argc > 1) {
	numForks = atoi(argv[1]);
    }
    gettimeofday(&startTime,0);
    for (i = 0; i < numForks; i++) { 
	pid = fork();
	if (pid == 0) {
	    /*
	     * child.
	     */
	    _exit(0);
	} 
	/*
	 * Parent 
	 */
	wait(0);
    }
    gettimeofday(&endTime,0);
    fixtime(&startTime,&endTime);
    printf("fork test %d forks time %4d.%-03d\n", numForks, 
	endTime.tv_sec, endTime.tv_usec/1000);
}

fixtime(s, e)
        struct  timeval *s, *e;
{

        e->tv_sec -= s->tv_sec;
        e->tv_usec -= s->tv_usec;
        if (e->tv_usec < 0) {
                e->tv_sec--; e->tv_usec += 1000000;
        }
}
