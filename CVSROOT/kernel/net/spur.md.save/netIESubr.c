/* netIESubr.c -
 *
 * Subroutines for the intel device driver.  These routines are to convert 
 * SPUR addresses to/from Intel address and offsets into Intel memory.
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
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "list.h"
#include "netIEInt.h"
#include "net.h"
#include "netInt.h"


/*
 *----------------------------------------------------------------------
 *
 * NetIEAddrFromSPURAddr --
 *
 *	Change 32-bit SPUR address to Intel LCC 16-bit address.
 *
 * Results:
 *	The modified address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned short
NetIEAddrFromSPURAddr(addr)
    Address	addr;
{

    if (addr < netIEState.deviceBase || 
       (addr > (netIEState.deviceBase + NET_IE_MEM_SIZE))) {
	Sys_Panic(SYS_FATAL,
		"NetIEAddrFromSPURAddr: Address out of buffer memory\n");
    }

    return((unsigned short) (addr - netIEState.deviceBase));
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEAddrToSPURAddr --
 *
 *	Change 16-bit Intel address to a SPUR address.
 *
 * Results:
 *	The modified address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Address
NetIEAddrToSPURAddr(addr)
    unsigned	short addr;
{


    return(NET_IE_SLOT_OFFSET(addr));
}


