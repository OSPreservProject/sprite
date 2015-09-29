/* 
 * getpriority.c --
 *
 *	Procedure to map from Unix getpriority system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getpriority.c,v 1.2 88/07/29 17:39:30 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getpriority --
 *
 *	Procedure to map from Unix getpriority system call to Sprite 
 *	Proc_GetPriority. 
 *
 * Results:
 *	Always return 0 for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
getpriority(which, who)
    int	which, who;
{
    return(0);
}
