/* 
 * frexp.c --
 *
 *	frexp math routine..
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/frexp.c,v 1.3 92/03/27 13:37:07 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <math.h>

/* 
 * math.h provides inline definitions for using the 68881 coprocessor 
 * on a sun3.  frexp is one of the functions that gets inlined.
 */

#if defined(__STDC__) && defined(sun3) && !defined(__STRICT_ANSI__) \
  && !defined(__SOFT_FLOAT__)

/* use the inline definition */

#else


/*
 *----------------------------------------------------------------------
 *
 * frexp --
 *
 *	Describe the given argument as two numbers, x and i, where 
 *	arg = x * (2**i).
 *
 * Results:
 *	Returns the multiplier "x", which is always less than 1.0 in 
 *	absolute value.
 *
 * Side effects:
 *	Stores the integer exponent "i" at the address pointed to.
 *
 *----------------------------------------------------------------------
 */

double
frexp(x, i)
    double x;
    int *i;
{
    int neg, j;

    j = 0;
    neg = 0;
    if (x < 0) {
	x = -x;
	neg = 1;
    }
    if (x > 1.0) {
	while (x > 1) {
	    ++j;
	    x /= 2;
	}
    } else if (x < 0.5) {
	while(x < 0.5) {
	    --j;
	    x *= 2;
	}
    }
    *i = j;
    return neg ? -x : x;
}

#endif /* inline test */
