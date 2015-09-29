/*
 * alloca.h --
 *
 *	Declarations of alloca() for the symmetry.  Since the i386 has
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
 * $Header: /sprite/src/lib/include/sun3.md/RCS/alloca.h,v 1.1 89/07/21 14:44:23 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

extern char *alloca();

#endif /* _ALLOCA_H */

