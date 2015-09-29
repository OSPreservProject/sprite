/* 
 * Quad_CompareUns.c --
 *
 *	Quad_CompareUns libc routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/quad/RCS/Quad_CompareUns.c,v 1.1 91/03/18 12:19:57 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <quad.h>


/*
 *----------------------------------------------------------------------
 *
 * Quad_CompareUns --
 *
 *	General comparison of two unsigned quads.
 *
 * Results:
 *	1 if the first value is greater than the second, 0 if they're 
 *	the same, -1 if the first value is less.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Quad_CompareUns(q1Ptr, q2Ptr)
    u_quad *q1Ptr, *q2Ptr;	/* pointers to the two arguments */
{
    if (q1Ptr->val[QUAD_MOST_SIG] > q2Ptr->val[QUAD_MOST_SIG]) {
	return 1;
    } else if (q1Ptr->val[QUAD_MOST_SIG] < q2Ptr->val[QUAD_MOST_SIG]) {
	return -1;
    } else if (q1Ptr->val[QUAD_LEAST_SIG] > q2Ptr->val[QUAD_LEAST_SIG]) {
	return 1;
    } else if (q1Ptr->val[QUAD_LEAST_SIG] < q2Ptr->val[QUAD_LEAST_SIG]) {
	return -1;
    } else {
	return 0;
    }
}
