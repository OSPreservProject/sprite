/* 
 * Net_UltraAddrToString.c --
 *
 *	Convert an UltraNet address to a string.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_UltraAddrToString.c,v 1.2 90/09/11 14:43:43 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include <stdio.h>
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_UltraAddrToString --
 *
 *	Convert Ultranet address to printable representation.
 *
 * Results:
 *	Address of the string buffer.
 *
 * Side effects:
 *	The buffer is overwritten.
 *
 *----------------------------------------------------------------------
 */

char *
Net_UltraAddrToString(ultraAddrPtr, buffer)
    register Net_UltraAddress *ultraAddrPtr;
    char *buffer;
{
    int		group;
    int		unit;

    Net_UltraAddressGet(ultraAddrPtr, &group, &unit);
    sprintf(buffer, "%d/%d", group, unit);
    return(buffer);
}

