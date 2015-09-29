/* 
 * Net_HostToNetInt.c --
 *
 *	Convert an integer from host to network byte ordering.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_HostToNetInt.c,v 1.2 89/01/27 16:36:42 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "machparam.h"

/* 
 *----------------------------------------------------------------------
 *
 * Net_HostToNetInt --
 *
 *	Convert an integer in host byte order to an integer in 
 *	net byte order.
 *
 * Results:
 *	The integer in net byte order.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned int 
Net_HostToNetInt(longInt)
	unsigned int longInt;	/* 32bit integer in host byte order. */
{
#if BYTE_ORDER == LITTLE_ENDIAN

	union {
		unsigned int l;
		unsigned char  c[4];
	} in, out;

	in.l = longInt;
	out.c[0] = in.c[3];
	out.c[1] = in.c[2];
	out.c[2] = in.c[1];
	out.c[3] = in.c[0];

        return (out.l);
#else
	return(longInt);
#endif
}

