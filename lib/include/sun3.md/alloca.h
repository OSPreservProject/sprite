/*
 * alloca.h --
 *
 *	Declarations of alloca() for the 68k.  Since the 68k has
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
 * $Header: /sprite/src/lib/include/sun3.md/RCS/alloca.h,v 1.2 91/10/18 18:54:13 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

#include <cfuncproto.h>

extern _VoidPtr alloca();

#endif /* _ALLOCA_H */

