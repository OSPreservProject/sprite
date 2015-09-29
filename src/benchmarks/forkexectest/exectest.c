/* $Header: /sprite/src/benchmarks/forkexectest/RCS/exectest.c,v 1.2 92/04/10 15:48:38 kupfer Exp $ */

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
    char	fileName[128];
    int		numReps, size;
    struct timeval startTime, endTime;
    int		pid;


    gettimeofday(&startTime,0);
    for (i = 0; i < 100; i++) { 
	system("true");
    }
    gettimeofday(&endTime,0);
    fixtime(&startTime,&endTime);
    printf("system test %d forks time %4d.%03d\n", 100, 
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

