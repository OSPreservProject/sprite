/* 
 * devKbdQueueMach.c --
 *
 *	Machine-dependent routines for the keyboard queue.  This handles
 *	the mapping between raw key codes and Ascii.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "devKbdQueue.h"
#include "kbdMap.h"
#include "kbdMapMach.h"


/*
 *----------------------------------------------------------------------
 *
 * DevKbdQueueMachInit --
 *
 *	Machine-dependent initialization for the keyboard queue.
 *	Since we don't have a console, we have nothing to set up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
DevKbdQueueMachInit()
{
    /*
     * Set up the choice of arrays to map raw key codes into Ascii.
     */

    devKbdShiftedAscii		= devKbdPmaxToShiftedAscii;
    devKbdUnshiftedAscii	= devKbdPmaxToUnshiftedAscii;
}


