/* 
 * Quad_AddUns.c --
 *
 *	Quad_AddUns libc routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/quad/RCS/Quad_AddUns.c,v 1.1 91/03/18 12:18:49 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <quad.h>


/*
 *----------------------------------------------------------------------
 *
 * Quad_AddUns --
 *
 *	Add an unsigned quad to another unsigned quad.  The temporary 
 *	variable is needed so that a u_quad can be added to itself.
 *
 * Results:
 *	The sum of the 2 arguments.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
void
Quad_AddUns(uQuad1, uQuad2, resultPtr)
    u_quad uQuad1;		/* in */
    u_quad uQuad2;		/* in */
    u_quad *resultPtr;		/* out */
{
    unsigned long newLeastSig;	/* new least significant word */

    newLeastSig = uQuad1.val[QUAD_LEAST_SIG] + uQuad2.val[QUAD_LEAST_SIG]; 
    resultPtr->val[QUAD_MOST_SIG] = uQuad1.val[QUAD_MOST_SIG]
		+ uQuad2.val[QUAD_MOST_SIG];
    if (newLeastSig < uQuad1.val[QUAD_LEAST_SIG]
		&& newLeastSig < uQuad2.val[QUAD_LEAST_SIG]) {
	resultPtr->val[QUAD_MOST_SIG]++; 
    }
    resultPtr->val[QUAD_LEAST_SIG] = newLeastSig; 
}
