/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)popen.c	5.5 (Berkeley) 9/30/87";
#endif LIBC_SCCS and not lint

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>

#define	tst(a,b)	(*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1

extern	char *malloc();

static	int *popen_pid;
static	int nfiles;
static	int *child_status;


FILE *
popen(cmd,mode)
	char *cmd;
	char *mode;
{
	int p[2];
	int myside, hisside, pid;

	if (nfiles <= 0)
		nfiles = getdtablesize();
	if (popen_pid == NULL) {
		popen_pid = (int *)malloc((unsigned)
			(nfiles * sizeof *popen_pid));
		if (popen_pid == NULL)
			return (NULL);
		child_status = (int *)malloc((unsigned)
			(nfiles * sizeof *child_status));
		if (child_status == NULL)
			return (NULL);
		for (pid = 0; pid < nfiles; pid++) {
			popen_pid[pid] = -1;
			child_status[pid] = -1;
		}
	}
	if (pipe(p) < 0)
		return (NULL);
	myside = tst(p[WTR], p[RDR]);
	hisside = tst(p[RDR], p[WTR]);
	if ((pid = vfork()) == 0) {
		/* myside and hisside reverse roles in child */
		close(myside);
		if (hisside != tst(0, 1)) {
			dup2(hisside, tst(0, 1));
			close(hisside);
		}
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	}
	if (pid == -1) {
		close(myside);
		close(hisside);
		return (NULL);
	}
	popen_pid[myside] = pid;
	close(hisside);
	return (fdopen(myside, mode));
}

pclose(ptr)
	FILE *ptr;
{
	int omask;
	int child, pid, status;
	int i;
	int file;

	file = fileno(ptr);
	if (popen_pid != NULL) {
	    child = popen_pid[file];
	    popen_pid[file] = -1;
	} else {
	    child = -1;
	}
	fclose(ptr);
	if (child == -1)
		return (-1);
	if (child_status[file] != -1) {
	    status = child_status[file];
	    child_status[file] = -1;
	    return (status);
	}
	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGHUP));
	while ((pid = wait((union wait *) &status)) != child && pid != -1) {
	    for (i = 0; i < nfiles; i++) {
		if (popen_pid[i] == pid) { 
		    child_status[i] = status;
		    break;
		}
	    }
	}
	(void) sigsetmask(omask);
	return (pid == -1 ? -1 : status);
}
