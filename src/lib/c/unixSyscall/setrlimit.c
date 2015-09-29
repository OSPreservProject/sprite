/* 
 * setrlimit.c --
 *
 *	Procedure to fake Unix setrlimit call.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: setrlimit.c,v 1.1 88/06/19 14:31:58 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * setrlimit --
 *
 *	Fake the setrlimit call by always returning success.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
setrlimit()
{
    return(UNIX_SUCCESS);
}
