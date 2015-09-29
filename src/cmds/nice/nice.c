/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)nice.c	5.2 (Berkeley) 1/12/86";
#endif not lint

#include <stdio.h>

#include <sys/time.h>
#include <sys/resource.h>

#ifdef sprite
#include "proc.h"
#include "status.h"
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	int nicarg = 10;

	if (argc > 1 && argv[1][0] == '-') {
		nicarg = atoi(&argv[1][1]);
		argc--, argv++;
	}
	if (argc < 2) {
		fputs("usage: nice [ -n ] command\n", stderr);
		exit(1);
	}
#ifndef sprite
	if (setpriority(PRIO_PROCESS, 0, 
	    getpriority(PRIO_PROCESS, 0) + nicarg) < 0) {
		perror("setpriority");
		exit(1);
	}
#else
	{
		int status, prio, pid;

		pid = getpid();
		status = Proc_GetPriority(pid, &prio);
		if (status != 0) {
			fprintf(stderr, "nice: 0x%x: %s\n", pid,
			    Stat_GetMsg(status));
			return (1);
		}
		status = Proc_SetPriority(pid, -((-prio*10)+nicarg)/10, 0);
		if (status != 0) {
			fprintf(stderr, "nice: 0x%x: %s\n", pid,
			    Stat_GetMsg(status));
			return (1);
		}
	}
#endif

	execvp(argv[1], &argv[1]);
	perror(argv[1]);
	exit(1);
}
