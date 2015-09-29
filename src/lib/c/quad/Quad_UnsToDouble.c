/* 
 * Quad_UnsToDouble.c --
 *
 *	Quad_UnsToDouble libc routine.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/quad/RCS/Quad_UnsToDouble.c,v 1.1 91/03/18 12:21:15 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <quad.h>


/*
 *----------------------------------------------------------------------
 *
 * Quad_UnsToDouble --
 *
 *	Convert an unsigned quad to a double.
 *
 * Results:
 *	Returns a double that is approximately equal to the given 
 *	integer. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double
Quad_UnsToDouble(q)
    u_quad q;
{
    return ((double)0xffffffff + 1) * q.val[QUAD_MOST_SIG]
		+ q.val[QUAD_LEAST_SIG];
}
