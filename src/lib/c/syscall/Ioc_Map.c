/* 
 * Ioc_Map.c --
 *
 *	Source code for the Ioc_Map library procedure.
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
static char rcsid[] = "$Header: Ioc_Map.c,v 1.1 88/06/19 14:29:19 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_Map --
 *	Map a device into user space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None (yet).
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_Map(streamID, numBytes, address)
    int streamID;
    int numBytes;
    Address address;
{
    register ReturnStatus status;
    Ioc_MapArgs map;

    map.numBytes = numBytes;
    map.address = address;

    status = Fs_IOControl(streamID, IOC_MAP, sizeof(Ioc_MapArgs),
			(Address)&map, 0, (Address)NULL);
    return(status);
}
