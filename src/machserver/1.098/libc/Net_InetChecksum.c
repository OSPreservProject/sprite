/* 
 * Net_InetChecksum.c --
 *
 *	Compute an internet checksum.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_InetChecksum.c,v 1.5 90/09/11 14:43:46 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_InetChecksum --
 *
 *	Compute the 16-bit one's complement of the 1's complement sum of
 *	of all words in the buffer.
 *
 *	Note: It is assumed that the length of the buffer is at most
 *	128K bytes long. It also helps if the buffer is word-aligned.
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
Net_InetChecksum(len, bufPtr)
    register int len;		/* The number of bytes to checksum. */
    Address bufPtr;		/* What to checksum. */
{
    register unsigned short *wordPtr = (unsigned short *) bufPtr;
    register unsigned int sum = 0;

    
    /*
     * The basic algorithm 16-bit 1's complement addition is 
     *  1) add the two unsigned 16-bit quantities, 
     *  2) if there was a carry out of the high-order bit, 
     *	   it is added to the sum.
     * To detect a carry out of the high-order bit, the sum is stored
     * in a 32-bit word. As an optimization, we delay step 2 until
     * all the words have been added together. At that point, the
     * upper-half of the sum contains the sum of the carries from the
     * additions. This value is then added to the lower half and if that
     * operation causes a carry, then 1 is added to the sum.
     *
     * The optimization does place a limit on how many bytes can be
     * summed without causing an overflow of the 32-bit sum. In the worst
     * case, a maximum of 64K additions of 16-bit values can be added
     * without overflow.
     * 
     * The summation is done in an unrolled loop. Once we have less than 
     * 32 bytes to sum then it must be done in smaller loops.
     */

    if (len == 20) {
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
    }

    /*
     * The most signficant bits of "sum" contains the carries from
     * the overflow of the summing. Add this overflow back into
     * the least significant 16 bits of the sum and do it a second
     * time in case there's a carry from the first time.
     */
    if (sum > 0xffff) {
#if 0
	extern int main_Debug, tcp_out;

	if (tcp_out && main_Debug) {
	    fprintf(stderr, "Checksum 1: %x\n", sum);
	}
#endif /* 0 */

	sum = ((sum >> 16) & 0xffff) + (sum & 0xffff);
	/*
	 * See if there was a carry from the addition. The overflow will
	 * be at most 1.
	 */
#if 0
	if (tcp_out && main_Debug) {
	    fprintf(stderr, "Checksum 2: %x\n", sum);
	}
#endif /* 0 */
	if (sum > 0xffff) {
	    sum++;
	}
    }

    return((~sum & 0xffff));
}

