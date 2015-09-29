/* 
 * Gate_ByInetAddr.c --
 *
 *	Source code for the Gate_ByInetAddr library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/gate/RCS/Gate_ByInetAddr.c,v 1.1 92/06/04 22:03:20 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <gate.h>
#include <gateInt.h>


/*
 *-----------------------------------------------------------------------
 *
 * Gate_ByInetAddr --
 *
 *	Find information about the gateway with the given internet address.
 *
 * Results:
 *	A Gate_Entry structure, or NULL if no gateway exists in the database
 *	with the given internet address.  The Gate_Entry is statically
 *	allocated, and may be modified on the next call to any Gate_
 *	procedure.
 *
 * Side Effects:
 *	The gateway database file is opened if it wasn't already.
 *
 *-----------------------------------------------------------------------
 */

Gate_Entry *
Gate_ByInetAddr(inetAddr)
    register Net_InetAddress	inetAddr;	/* Address to match. */
{
    register Gate_Entry	    	*entry;

    if (Gate_Start() == 0 && Net_InetAddrCmp(inetAddr, 0)) {
	while (1) {
	    entry = Gate_Next();
	    if (entry == (Gate_Entry *) NULL) {
		return (Gate_Entry *) NULL;
	    }
	    if (!Net_InetAddrCmp(entry->inetAddr, inetAddr)) {
		return (entry);
	    }
	}
    }
    return ((Gate_Entry *)NULL);
}

