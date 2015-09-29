/* 
 * Quad_PutUns.c --
 *
 *	Implemenation of Quad_PutUns library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/quad/RCS/Quad_PutUns.c,v 1.1 91/03/18 12:21:00 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <quad.h>


/*
 *----------------------------------------------------------------------
 *
 * Quad_PutUns --
 *
 *	Format the given unsigned quad and write it to the given stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Quad_PutUns(stream, q)
    FILE *stream;
    u_quad q;
{
    if (q.val[QUAD_MOST_SIG] == 0) {
	fprintf(stream, "%lu", q.val[QUAD_LEAST_SIG]);
    } else {
	fprintf(stream, "%.12g", Quad_UnsToDouble(q));
    }
}
