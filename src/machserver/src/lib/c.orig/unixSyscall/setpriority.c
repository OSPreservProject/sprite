/* 
 * setpriority.c --
 *
 *	Procedure to map from Unix setpriority system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: setpriority.c,v 1.2 88/07/29 17:40:52 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * setpriority --
 *
 *	Procedure to map from Unix setpriority system call to Sprite 
 *	Proc_SetPriority. 
 *
 * Results:
 *	Always return success for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
setpriority(which, who, prio)
    int	which, who, prio;
{
    return(UNIX_SUCCESS);
}
