/* 
 * getgid.c --
 *
 *	Procedure to map from Unix getgid system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getgid.c,v 1.1 88/06/19 14:31:22 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getgid --
 *
 *	Procedure to map from Unix getgid system call to Sprite 
 *	Proc_GetGroupIDs. 
 *
 * Results:
 *	The sprite group id is returned for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getgid()
{
    return(155);
}

/*
 *----------------------------------------------------------------------
 *
 * getegid --
 *
 *	Procedure to map from Unix getgid system call to Sprite 
 *	Proc_GetGroupIDs. 
 *
 * Results:
 *	The sprite group id is returned for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getegid()
{
    return(155);
}
