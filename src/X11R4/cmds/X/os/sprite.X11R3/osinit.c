/*-
 * osinit.c --
 *	Initialization!
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /mic/X11R3/src/cmds/Xsp/os/sprite/RCS/osinit.c,v 1.8 89/10/25 18:06:49 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteos.h"
#include    "opaque.h"
#include    <dbm.h>
#include    <bit.h>

/*-
 *-----------------------------------------------------------------------
 * OsInit --
 *	Initialize this module. Not much to do...
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
OsInit()
{
    static havergb = 0;

    GrabDone = FALSE;
    List_Init(&allStreams);
    if (ClientsWithInputMask != (int *)0) {
	/*
	 * On Reset, all the clients and devices should have gone away,
	 * but that could still leave something bogus in the
	 * ClientsWithInputMask...
	 */
	Bit_Zero (NumActiveStreams, ClientsWithInputMask);
    }

    if(!havergb)
        if(dbminit (rgbPath) == 0)
	    havergb = 1;
        else
	    ErrorF( "Couldn't open RGB_DB '%s'\n", rgbPath );
}
