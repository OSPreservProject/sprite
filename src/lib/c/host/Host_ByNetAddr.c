/* 
 * Host_ByNetAddr.c --
 *
 *	Source code for the Host_ByNetAddr library procedure.
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
static char rcsid[] = "$Header: /user6/voelker/src/hosttest/RCS/Host_ByNetAddr.c,v 1.1 92/03/26 19:46:13 voelker Exp Locker: voelker $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <host.h>
#include <hostInt.h>


/*
 *-----------------------------------------------------------------------
 *
 * Host_ByNetAddr --
 *
 *	Return information about the host with the given local network
 *	address.
 *
 * Results:
 *	A Host_Entry structure describing the host.  If no such host
 *	exists, NULL is returned.  The Host_Entry is statically
 *	allocated, and may be modified on the next call to any Host_
 *	procedure.
 *
 * Side Effects:
 *	The host database is opened if it wasn't already.
 *
 *-----------------------------------------------------------------------
 */

Host_Entry *
Host_ByNetAddr(addrPtr)
    Net_Address            *addrPtr;   	/* Pointer to the address in the
					 * format for the network */
{
    register Host_Entry	   *entry;
    register Host_NetInt   *interPtr;
    register int           i;
    register unsigned char *address = (unsigned char *) addrPtr;

    if (Host_Start() == 0 && (addrPtr->type != NET_ADDRESS_NONE)) {
	while (1) {
	    entry = Host_Next();
	    if (entry == (Host_Entry *) NULL) {
		break;
	    }
	    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
		if (!Net_AddrCmp(&entry->nets[i].netAddr, addrPtr)) {
		    return entry;
		}
	    }
	}
    }
    return (Host_Entry *) NULL;
}

