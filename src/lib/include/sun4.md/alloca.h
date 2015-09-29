/*
 * alloca.h --
 *
 *	Declarations of alloca() for the sun4.  Since the sun4 has
 *      register window, this can't be handled in the normal way.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/sun4.md/RCS/alloca.h,v 1.3 90/03/15 11:55:31 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

#ifdef sun4
#define alloca(p)  ((char *) __builtin_alloca(p))
#endif

#endif /* _ALLOCA_H */

