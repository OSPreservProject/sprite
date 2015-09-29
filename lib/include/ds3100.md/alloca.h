/*
 * alloca.h --
 *
 *	Declarations of alloca() for the ds3100.  Since the ds3100 has
 *      a normal stack alloca is just a normal procedure.  For other
 *      machines such as the sparc it has to be a fancy builtin macro.
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
 * $Header: /sprite/src/lib/include/ds3100.md/RCS/alloca.h,v 1.2 89/11/12 01:32:27 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

/* If compiled with GNU C, use the built-in alloca */
#ifdef __GNUC__
#define alloca __builtin_alloca
#else
extern char *alloca();
#endif

#endif /* _ALLOCA_H */

