/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/dviselect/RCS/error.c,v 1.1 91/10/18 21:15:45 rab Exp Locker: rab $";
#endif

/*
 * Print an error message with an optional system error number, and
 * optionally quit.
 *
 * THIS CODE IS SYSTEM DEPENDENT UNLESS varargs WORKS WITH vprintf
 * OR _doprnt.  It should work properly under System V using vprintf.
 * (If you have vprintf, define HAVE_VPRINTF.)
 */

#include <stdio.h>
#include <varargs.h>

#ifdef lint

/* VARARGS3 ARGSUSED */
error(quit, e, fmt) int quit, e; char *fmt; {;}

/* VARARGS1 ARGSUSED */
panic(fmt) char *fmt; { exit(1); /* NOTREACHED */ }

#else lint

extern char *ProgName;
extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;

error(va_alist)
	va_dcl
{
	va_list l;
	int quit, e;
	char *fmt;

	(void) fflush(stdout);	/* sync error messages */
	(void) fprintf(stderr, "%s: ", ProgName);
	va_start(l);
	/* pick up the constant arguments: quit, errno, printf format */
	quit = va_arg(l, int);
	e = va_arg(l, int);
	if (e < 0)
		e = errno;
	fmt = va_arg(l, char *);
#if defined(sys5) || defined(HAVE_VPRINTF)
	(void) vfprintf(stderr, fmt, l);
#else
	_doprnt(fmt, l, stderr);
#endif
	va_end(l);
	if (e) {
		if (e < sys_nerr)
			(void) fprintf(stderr, ": %s", sys_errlist[e]);
		else
			(void) fprintf(stderr, ": Unknown error code %d", e);
	}
	(void) putc('\n', stderr);
	(void) fflush(stderr);	/* just in case */
	if (quit)
		exit(quit);
}

panic(va_alist)
	va_dcl
{
	va_list l;
	char *fmt;

	(void) fflush(stdout);
	(void) fprintf(stderr, "%s: panic: ", ProgName);
	va_start(l);
	/* pick up the constant argument: printf format */
	fmt = va_arg(l, char *);
#if defined(sys5) || defined(HAVE_VPRINTF)
	(void) vfprintf(stderr, fmt, l);
#else
	_doprnt(fmt, l, stderr);
#endif
	va_end(l);
	(void) putc('\n', stderr);
	(void) fflush(stderr);
	abort();
}

#endif /* lint */
