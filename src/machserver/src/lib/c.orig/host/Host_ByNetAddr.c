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
static char rcsid[] = "$Header: /sprite/src/lib/c/host/RCS/Host_ByNetAddr.c,v 1.3 88/12/15 09:25:24 ouster Exp $ SPRITE (Berkeley)";
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
Host_ByNetAddr(addrType, addrPtr)
    Host_NetType  	addrType;   	/* The type of network the host
					 * is on */
    char * 	  	addrPtr;    	/* Pointer to the address in the
					 * format for the network */
{
    register Host_Entry	*entry;

    if (addrType == HOST_INET) {
	struct in_addr	inetAddr;
	bcopy(addrPtr, (char *) &inetAddr, sizeof(inetAddr));
	return Host_ByInetAddr(inetAddr);
    }

    if (Host_Start() == 0) {
	switch (addrType) {
	    case HOST_ETHER: {
		int i;
		unsigned char *address = (unsigned char *) addrPtr;

		while (1) {
		    entry = Host_Next();
		    if (entry == (Host_Entry *) NULL) {
			break;
		    }
		    for (i = HOST_ETHER_ADDRESS_SIZE - 1; i >= 0; i--) {
			if (entry->netAddr.etherAddr[i] != address[i]) {
			    break;
			}
			if (i == 0) {
			    return entry;
			}
		    }
		}
		break;
	    }
	}
    }
    return (Host_Entry *) NULL;
}
