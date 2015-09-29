/* 
 * getpagesize.c --
 *
 *	Procedure to map from Unix getpagesize system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getpagesize.c,v 1.1 88/06/19 14:31:26 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getpagesize --
 *
 *	Procedure to map Unix getpagesize call to Vm_GetPageSize. 
 *
 * Results:
 *	The page size used by Sprite is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getpagesize()
{
    int size;
    (void)Vm_PageSize(&size);
    return(size);
}
