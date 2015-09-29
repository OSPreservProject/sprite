/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: osinit.c,v 1.28 89/12/18 15:41:25 rws Exp $ */
#include "os.h"
#include "opaque.h"
#undef NULL
#ifdef NDBM
#include <ndbm.h>
#else
#include <dbm.h>
#endif
#undef NULL
#include <stdio.h>
#include "Xos.h"
#ifndef MAXPATHLEN
/*
 * just to get MAXPATHLEN.  Define it elsewhere if you need to
 * avoid these files.
 */
#include <sys/param.h>
#endif

#ifndef SYSV
#include <sys/resource.h>
#endif

#ifndef ADMPATH
#define ADMPATH "/usr/adm/X%smsgs"
#endif

#ifdef NDBM
DBM     *rgb_dbm = (DBM *)NULL;
#else
int	rgb_dbm = 0;
#endif
extern char *display;
#ifndef SYSV
int limitDataSpace = -1;
int limitStackSpace = -1;
#endif

OsInit()
{
    static Bool been_here = FALSE;
    char fname[MAXPATHLEN];

#ifdef macII
    set42sig();
#endif

    /* hack test to decide where to log errors */

    if (!been_here) {
	if (write (2, fname, 0)) 
	{
	    long t; 
	    char *ctime();
	    FILE *err;
	    fclose(stdin);
	    fclose(stdout);
	    sprintf (fname, ADMPATH, display);
	    /*
	     * uses stdio to avoid os dependencies here,
	     * a real os would use
 	     *  open (fname, O_WRONLY|O_APPEND|O_CREAT, 0666)
	     */
	    if (!(err = fopen (fname, "a+")))
		err = fopen ("/dev/null", "w");
	    if (err && (fileno(err) != 2)) {
		dup2 (fileno (err), 2);
		fclose (err);
	    }
#ifdef SYSV		/* yes, even though it is 4.2bsd.... */
	    {
	    static char buf[BUFSIZ];
	    setvbuf (stderr, buf, _IOLBF, BUFSIZ);
	    }
#else
	    setlinebuf(stderr);
#endif
	    time (&t);
	    fprintf (stderr, "start %s", ctime(&t));
	}

	if (getpgrp (0) == 0)
	    setpgrp (0, getpid ());


#ifndef SYSV
#if !defined(AIXrt) && !defined(AIX386)
	if (limitDataSpace >= 0)
	{
	    struct rlimit	rlim;

	    if (!getrlimit(RLIMIT_DATA, &rlim))
	    {
		if ((limitDataSpace > 0) && (limitDataSpace < rlim.rlim_max))
		    rlim.rlim_cur = limitDataSpace;
		else
		    rlim.rlim_cur = rlim.rlim_max;
		(void)setrlimit(RLIMIT_DATA, &rlim);
	    }
	}
	if (limitStackSpace >= 0)
	{
	    struct rlimit	rlim;

	    if (!getrlimit(RLIMIT_STACK, &rlim))
	    {
		if ((limitStackSpace > 0) && (limitStackSpace < rlim.rlim_max))
		    rlim.rlim_cur = limitStackSpace;
		else
		    rlim.rlim_cur = rlim.rlim_max;
		(void)setrlimit(RLIMIT_STACK, &rlim);
	    }
	}
#endif
#endif
	been_here = TRUE;
    }

    if (!rgb_dbm)
    {
#ifdef NDBM
	rgb_dbm = dbm_open(rgbPath, 0, 0);
#else
	if (dbminit(rgbPath) == 0)
	    rgb_dbm = 1;
#endif
	if (!rgb_dbm)
	    ErrorF( "Couldn't open RGB_DB '%s'\n", rgbPath );
    }
}
