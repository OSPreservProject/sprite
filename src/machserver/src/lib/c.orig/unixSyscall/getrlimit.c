/* 
 * getrlimit.c --
 *
 *	Procedure to fake Unix getrlimit call.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getrlimit.c,v 1.1 88/06/19 14:31:28 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getrlimit --
 *
 *	Fake the getrlimit call by always returning success.
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
getrlimit()
{
    return(UNIX_SUCCESS);
}
