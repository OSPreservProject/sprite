/* 
 * Net_InetChecksum2.c --
 *
 *	Compute an internet checksum, including the header.
 *
 * Copyright 1987 Regents of the University of California
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_InetHdrChecksum.c,v 1.4 90/01/24 11:53:42 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_InetChecksum2 --
 *
 *	This routine is an optimization for calculating checksums for
 *	the UDP and TCP output routines. It is similar to Net_InetChecksum 
 *	except the checksum includes the IP pseudo-header.
 *
 *	The comments of Net_InetChecksum apply here.
 *
 * Results:
 *	The 1's complement checksum in network byte-order.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned short
Net_InetChecksum2(len, bufPtr, pseudoHdrPtr)
    register int 	len;		/* The # of bytes in bufPer. */
    Address		bufPtr;		/* Data to checksum. */
    Net_IPPseudoHdr	*pseudoHdrPtr;	/* IP pseudo-header to include in the
					 * checksum. */
{
    register unsigned short *wordPtr;
    register unsigned int sum = 0;


    /*
     * First compute the checksum for the IP pseudo-header.
     */
    wordPtr = (unsigned short *) pseudoHdrPtr;
    if (sizeof(*pseudoHdrPtr) == 20) {
	/*
	 * This is a hack because cpp can't use the sizeof operator 
	 * in #if statements.
	 */
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

    } else {
	register int i;

	i = sizeof(*pseudoHdrPtr)/sizeof(unsigned short);
	do {
	    sum += *wordPtr++;
	    i--;
	} while (i > 0);
    }

    /*
     * Sum the data in an unrolled loop. Once we have less than 
     * 32 bytes to sum then it must be done in smaller loops.
     */

    wordPtr = (unsigned short *) bufPtr;
    while (len >= 32) {
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;
	sum += *wordPtr++;

	len -= 32;
    }
    while (len >= 2) {
	sum += *wordPtr++;
	len -= 2;
    }

    if (len == 1) {
#if BYTE_ORDER == LITTLE_ENDIAN
	sum += (*wordPtr) & 0x00ff;
#else
	sum += (*wordPtr) & 0xff00;
#endif
    }

    /*
     * The most signficant bits of "sum" contains the carries from
     * the overflow of the summing. Add this overflow back into
     * the least significant 16 bits of the sum and do it a second
     * time in case there's a carry from the first time.
     */
    if (sum > 0xffff) {
	sum = ((sum >> 16) & 0xffff) + (sum & 0xffff);

	/*
	 * See if there was a carry from the addition. The overflow will
	 * be at most 1.
	 */
	if (sum > 0xffff) {
	    sum++;
	}
    }

    return((~sum & 0xffff));
}

