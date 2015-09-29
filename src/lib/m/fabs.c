/* 
 * fabs.c --
 *
 *	Source code for the "fabs" library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/m/RCS/fabs.c,v 1.1 88/10/23 14:57:23 ouster Exp $ SPRITE (Berkeley)";
#endif not lint


/*
 *----------------------------------------------------------------------
 *
 * fabs --
 *
 *	Return the absolute value of a floating-point number
 *
 * Results:
 *	Absolute value of x.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double
fabs(x)
    double x;
{
    if (x < 0) {
	return -x;
    }
    return x;
}
