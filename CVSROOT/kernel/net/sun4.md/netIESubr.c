/* netIESubr.c -
 *
 * Subroutines for the intel device driver.  These routines are to convert 
 * 68000 addresses to/from Intel address and offsets into Intel memory.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "list.h"
#include "vm.h"
#include "netIE.h"
#include "net.h"
#include "netInt.h"


/*
 *----------------------------------------------------------------------
 *
 * NetIEAddrFrom68000Addr --
 *
 *	Change 24-bit 68000 address to Intel 24-bit address.
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
NetIEAddrFrom68000Addr(addr)
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
 * NetIEAddrTo68000Addr --
 *
 *	Change 24-bit Intel address to a 68000 address.
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
NetIEAddrTo68000Addr(addr)
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
 * NetIEOffsetFrom68000Addr --
 *
 *	Change 24-bit 68000 address to Intel 16-bit offset into
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
NetIEOffsetFrom68000Addr(addr)
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
 * NetIEOffsetTo68000Addr --
 *
 *	Change 16 bit Intel offset into Intel memory to 24 bit 68000 address.
 *
 * Results:
 *	The 68000 address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NetIEOffsetTo68000Addr(offset)
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
 * NetIEShortFrom68000Short --
 *
 *	Change a short on the 68000 to a short on the intel.
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
