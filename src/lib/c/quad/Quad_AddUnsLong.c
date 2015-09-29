/* 
 * Quad_AddUnsLong.c --
 *
 *	Quad_AddUnsLong libc routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/quad/RCS/Quad_AddUnsLong.c,v 1.1 91/03/18 12:19:06 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <quad.h>


/*
 *----------------------------------------------------------------------
 *
 * Quad_AddUnsLong --
 *
 *	Add an unsigned long to an unsigned quad.
 *
 * Results:
 *	The sum of the two values.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Quad_AddUnsLong(uQuad, uLong, resultPtr)
    u_quad uQuad;		/* in */
    u_long uLong;		/* in */
    u_quad *resultPtr;		/* out */
{
    unsigned long newLeastSig;

    newLeastSig = uQuad.val[QUAD_LEAST_SIG] + uLong; 
    resultPtr->val[QUAD_MOST_SIG] = uQuad.val[QUAD_MOST_SIG];
    if (newLeastSig < uQuad.val[QUAD_LEAST_SIG] && newLeastSig < uLong) { 
	resultPtr->val[QUAD_MOST_SIG]++; 
    } 
    resultPtr->val[QUAD_LEAST_SIG] = newLeastSig; 
}
