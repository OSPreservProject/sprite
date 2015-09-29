/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)utmp.h	5.1 (Berkeley) 5/30/85
 */

#ifndef _UTMP
#define _UTMP

/*
 * Structure of utmp and wtmp files.
 *
 */

#define _PATH_UTMP	"/etc/utmp"
#define  UT_NAMESIZE	8
#define  UT_LINESIZE	8
#define  UT_HOSTSIZE	16

struct utmp {
	char	ut_line[UT_LINESIZE];	/* tty name */
	char	ut_name[UT_NAMESIZE];	/* user id */
	char	ut_host[UT_HOSTSIZE];	/* host name, if remote */
	long	ut_time;		/* time on */
};

#endif /* _UTMP */
