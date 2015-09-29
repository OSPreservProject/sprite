/* $Header: /sprite/src/benchmarks/template/RCS/t.c,v 1.2 92/04/10 15:31:37 kupfer Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
main(arv)
{
	register int i;
	int	junk1, junk2;
	int starttime;
	char *m = malloc(1024);
	int	fd = fileno(stdin);
	struct timeval stp, etp;
	bzero(m, 1024);
	    gettimeofday(&stp,0);
	    for (i = 0; i < 1000*1000; i++) {
		getpid();
	    }
	    gettimeofday(&etp,0);
	    fixtime(&stp,&etp);
	    printf("time %4d.%03d\n", etp.tv_sec, etp.tv_usec/1000);
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

