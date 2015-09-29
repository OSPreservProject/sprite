/* 
 * Net_StringToUltraAddr.c --
 *
 *	Convert a string to an UltraNet address.
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
static char rcsid[] = "$Header: /sprite/src/lib/net/RCS/Net_StringToUltraAddr.c,v 1.2 88/11/21 09:28:33 mendel Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "net.h"

/*
 *----------------------------------------------------------------------
 *
 * Net_StringToUltraAddr --
 *
 *	This routine takes a string form of an UltraNet address and
 *	converts it to the Net_UltraAddress form. The string must be
 *	null-terminated.
 *
 * Results:
 *	The Ultranet address in the Net_UltraAddress form.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Net_StringToUltraAddr(buffer, ultraAddressPtr)
    register char *buffer;
    Net_UltraAddress *ultraAddressPtr;
{
    int		group;
    int		unit;
    int		n;

    *buffer = '\0';
    n = sscanf(buffer, "%d/%d", &group, &unit);
    if (n != 2) {
	return;
    }
    Net_UltraAddressSet(ultraAddressPtr, group, unit);
}

