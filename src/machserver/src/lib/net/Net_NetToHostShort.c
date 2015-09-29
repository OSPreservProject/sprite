/* 
 * Net_NetToHostShort.c --
 *
 *	Convert a short integer from network to host byte ordering.
 *
 * Copyright 1988 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_NetToHostShort.c,v 1.3 89/01/27 16:38:40 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "machparam.h"

/* 
 *----------------------------------------------------------------------
 *
 * Net_NetToHostShort --
 *
 *	Convert a short integer in network byte order to an short integer in 
 *	host byte order.
 *
 * Results:
 *	The short integer in host byte order.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
unsigned short 
Net_NetToHostShort(shortInt)
	unsigned short shortInt; 	/* A short int in network byte order. */
{

#if BYTE_ORDER == LITTLE_ENDIAN
	union swab {
		unsigned short s;
		unsigned char  c[2];
	} in, out;

	in.s = shortInt;
	out.c[0] = in.c[1];
	out.c[1] = in.c[0];

     return (out.s);
#else
    return (shortInt);
#endif
}
