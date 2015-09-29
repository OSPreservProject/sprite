/* 
 * Gate_ByNetAddr.c --
 *
 *	Source code for the Gate_ByNetAddr library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/gate/RCS/Gate_ByNetAddr.c,v 1.1 92/06/04 22:03:20 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <gate.h>
#include <gateInt.h>


/*
 *-----------------------------------------------------------------------
 *
 * Gate_ByNetAddr --
 *
 *	Return information about the gateway with the given local network
 *	address.
 *
 * Results:
 *	A Gate_Entry structure describing the gateway.  If no such gateway
 *	exists, NULL is returned.  The Gate_Entry is statically
 *	allocated, and may be modified on the next call to any Gate_
 *	procedure.
 *
 * Side Effects:
 *	The gateway database is opened if it wasn't already.
 *
 *-----------------------------------------------------------------------
 */

Gate_Entry *
Gate_ByNetAddr(addrPtr)
    Net_Address 	*addrPtr;    	/* Pointer to the address in the
					 * format for the network */
{
    register Gate_Entry	*entry;
    register int        i;
    register unsigned char *address = (unsigned char *) addrPtr;

    if (Gate_Start() == 0) {
	while (1) {
	    entry = Gate_Next();
	    if (entry == (Gate_Entry *) NULL) {
		break;
	    }
	    if (!Net_AddrCmp(&entry->netAddr, addrPtr)) {
		return entry;
	    }
	}
    }
    return (Gate_Entry *) NULL;
}
