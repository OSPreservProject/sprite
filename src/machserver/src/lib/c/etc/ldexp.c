/*
 * ldexp.c --
 *
 *      Source code for the ldexp library function.
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
static char rcsid[] = 
    "$Header: /r3/kupfer/spriteserver/src/lib/c/etc/RCS/ldexp.c,v 1.2 91/12/12 21:25:33 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint
     
#include <stdio.h>
#include <errno.h>

/* Largest signed long int power of 2 */
#define MAXSHIFT        (8 * sizeof(long) - 2)   

#define	MAXFLOAT	1.7e308

double ldexp(value, exp)
     double value;
     int    exp; 
{


	extern double frexp();
	int	old_exp;

        if (exp == 0 || value == 0.0) /* nothing to do for zero */
                return (value);
        (void) frexp(value, &old_exp);
        if (exp > 0) {
                if (exp + old_exp > 1023) { /* overflow */
                        errno = ERANGE;
                        return (value < 0 ? -MAXFLOAT : MAXFLOAT);
                }
                for ( ; exp > MAXSHIFT; exp -= MAXSHIFT)
                        value *= (1L << MAXSHIFT);
                return (value * (1L << exp));
        }
        if (exp + old_exp < -1023) { /* underflow */
                errno = ERANGE;
                return (0.0);
        }
        for ( ; exp < -MAXSHIFT; exp += MAXSHIFT)
                value *= 1.0/(1L << MAXSHIFT); /* mult faster than div */
        return (value / (1L << -exp));
}
