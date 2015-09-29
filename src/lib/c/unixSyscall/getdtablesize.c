/* 
 * getdtablesize.c --
 *
 *	Procedure to map from Unix getdtablesize system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getdtablesize.c,v 1.1 88/06/19 14:31:20 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getdtablesize --
 *
 *	Fake procedure for Unix getdtablesize call. 
 *
 * Results:
 *	The number of descriptors used by Sprite is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getdtablesize()
{
    return(100);
}
