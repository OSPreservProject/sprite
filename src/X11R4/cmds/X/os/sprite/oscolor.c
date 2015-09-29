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
/* $XConsortium: oscolor.c,v 1.17 89/12/18 15:41:43 rws Exp $ */
#ifdef NDBM
#include <ndbm.h>
#else
#include <dbm.h>
#endif
#include "rgb.h"
#include "os.h"

/* Looks up the color in the database.  Note that we are assuming there
 * is only one database for all the screens.  If you have multiple databases,
 * remove the dbminit() in OsInit(), and open the appropriate database each
 * time. Or implement a database package that allows you to have more than
 * one database open at a time.
 */
#ifdef NDBM
extern DBM *rgb_dbm;
#else
extern int rgb_dbm;
#endif

extern void CopyISOLatin1Lowered();

/*ARGSUSED*/
int
OsLookupColor(screen, name, len, pred, pgreen, pblue)
    int		screen;
    char	*name;
    unsigned	len;
    unsigned short	*pred, *pgreen, *pblue;

{
    datum		dbent;
    RGB			rgb;
    char	*lowername;

    if(!rgb_dbm)
	return(0);

    /* convert name to lower case */
    lowername = (char *)ALLOCATE_LOCAL(len + 1);
    if (!lowername)
	return(0);
    CopyISOLatin1Lowered ((unsigned char *) lowername, (unsigned char *) name,
			  (int)len);

    dbent.dptr = lowername;
    dbent.dsize = len;
#ifdef NDBM
    dbent = dbm_fetch(rgb_dbm, dbent);
#else
    dbent = fetch (dbent);
#endif

    DEALLOCATE_LOCAL(lowername);

    if(dbent.dptr)
    {
	bcopy(dbent.dptr, (char *) &rgb, sizeof (RGB));
	*pred = rgb.red;
	*pgreen = rgb.green;
	*pblue = rgb.blue;
	return (1);
    }
    return(0);
}

