/* netIESubr.c -
 *
 * Subroutines for the intel device driver.  These routines are to convert 
 * SUN addresses to/from Intel address and offsets into Intel memory.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "list.h"
#include "vm.h"
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"


/*
 *----------------------------------------------------------------------
 *
 * NetIEAddrFromSUNAddr --
 *
 *	Change 24-bit SUN address to Intel 24-bit address.
 *
 * Results:
 *	The modified address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NetIEAddrFromSUNAddr(addr)
    int	addr;
{
    union {
	int 	i;
	char	ch[4];
    } addrTo, addrFrom;

    addrFrom.i = addr;

    addrTo.ch[0] = addrFrom.ch[3];
    addrTo.ch[1] = addrFrom.ch[2];
    addrTo.ch[2] = addrFrom.ch[1];
    addrTo.ch[3] = 0;

    return(addrTo.i);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEAddrToSUNAddr --
 *
 *	Change 24-bit Intel address to a SUN address.
 *
 * Results:
 *	The modified address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NetIEAddrToSUNAddr(addr)
    int	addr;
{
    union {
	int 	i;
	char	ch[4];
    } addrTo, addrFrom;

    addrFrom.i = addr;

    addrTo.ch[0] = 0;
    addrTo.ch[1] = addrFrom.ch[2];
    addrTo.ch[2] = addrFrom.ch[1];
    addrTo.ch[3] = addrFrom.ch[0];

    return(addrTo.i);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEOffsetFromSUNAddr --
 *
 *	Change 24-bit SUN address to Intel 16-bit offset into
 *	Intel memory.
 *
 * Results:
 *	The Intel offset .
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NetIEOffsetFromSUNAddr(addr)
    int	addr;
{
    union {
	short 	s;
	char	ch[2];
    } offsetTo, offsetFrom;

    offsetFrom.s = (short) (addr - netIEState.memBase);

    offsetTo.ch[0] = offsetFrom.ch[1];
    offsetTo.ch[1] = offsetFrom.ch[0];

    return(offsetTo.s);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEOffsetToSUNAddr --
 *
 *	Change 16 bit Intel offset into Intel memory to 24 bit SUN address.
 *
 * Results:
 *	The SUN address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NetIEOffsetToSUNAddr(offset)
    int	offset;
{
    union {
	short 	s;
	char	ch[2];
    } offsetTo, offsetFrom;

    offsetFrom.s = offset;

    offsetTo.ch[0] = offsetFrom.ch[1];
    offsetTo.ch[1] = offsetFrom.ch[0];

    return(netIEState.memBase + offsetTo.s);
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEShortFromSUNShort --
 *
 *	Change a short on the SUN to a short on the intel.
 *
 * Results:
 *	The Intel short .
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NetIEShortSwap(num)
    int	num;
{
    union {
	short 	s;
	char	ch[2];
    } shortFrom, shortTo;

    shortFrom.s = (short) (num);

    shortTo.ch[0] = shortFrom.ch[1];
    shortTo.ch[1] = shortFrom.ch[0];

    return(shortTo.s);
}
