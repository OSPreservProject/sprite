/*
 * memmove.c --
 *
 * Just like memcpy, but memmove is guaranteed to work even if
 * the regions overlap.
 *
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif

#include <string.h>

char *
memmove(t, f, n)
	register char *t, *f;
	register int n;
{
	register char *p = t;

	if (t <= f) {
	    while (--n >= 0) {
		*t++ = *f++;
	    }
	} else {
	    t += n;
	    f += n;
	    while (--n >= 0) {
		*--t = *--f;
	    }
	}
	return (p);
}
