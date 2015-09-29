/* 
 * swap.c --
 *
 *	Library routine for manipulating byte order.
 *
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/net/RCS/swap.c,v 1.2 89/12/11 13:42:57 rab Exp $ SPRITE (Berkeley)";
#endif

#include "machparam.h"


/* 
 *----------------------------------------------------------------------
 *
 * ntohs --
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
ntohs(shortInt)
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
    return out.s;
#else
    return shortInt;
#endif

}

/* 
 *----------------------------------------------------------------------
 *
 * ntohl --
 *
 *	Convert an integer in network byte order to an integer in 
 *	host byte order.
 *
 * Results:
 *	The integer in host byte order.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long 
ntohl(longInt)
    unsigned long longInt;	/* 32bit integer in network byte order. */
{
#if BYTE_ORDER == LITTLE_ENDIAN
    union {
	unsigned long l;
	unsigned char  c[4];
    } in, out;

    in.l = longInt;
    out.c[0] = in.c[3];
    out.c[1] = in.c[2];
    out.c[2] = in.c[1];
    out.c[3] = in.c[0];
    return out.l;
#else
    return longInt;
#endif
}

/* 
 *----------------------------------------------------------------------
 *
 * htons --
 *
 *	Convert a short integer in host byte order to an short integer in 
 *	network byte order.
 *
 * Results:
 *	The short integer in network byte order.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned short 
htons(shortInt)
	unsigned short shortInt; 	/* A short int in Host byte order. */
{
#if BYTE_ORDER == LITTLE_ENDIAN
    union swab {
	unsigned short s;
	unsigned char  c[2];
    } in, out;

    in.s = shortInt;
    out.c[0] = in.c[1];
    out.c[1] = in.c[0];
    return out.s;
#else
    return shortInt;
#endif
}

/* 
 *----------------------------------------------------------------------
 *
 * htonl --
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

unsigned long 
htonl(longInt)
	unsigned long longInt;	/* 32bit integer in host byte order. */
{
#if BYTE_ORDER == LITTLE_ENDIAN
    union {
	unsigned long l;
	unsigned char  c[4];
    } in, out;

    in.l = longInt;
    out.c[0] = in.c[3];
    out.c[1] = in.c[2];
    out.c[2] = in.c[1];
    out.c[3] = in.c[0];
    return out.l;
#else
    return longInt;
#endif
}

