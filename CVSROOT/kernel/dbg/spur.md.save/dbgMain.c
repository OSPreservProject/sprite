/* 
 * dbgMain.c --
 *
 *	Routines for the SPUR kernel debugger.
 *
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPR
ITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "dbg.h"
#include "dbgInt.h"

Boolean		dbg_UsingSyslog = FALSE;

Boolean	dbg_BeingDebugged = FALSE;		/* TRUE if are under control
						 * of kdbx.*/
Boolean	dbg_Rs232Debug = TRUE;			/* TRUE if are using the RS232
						 * line to debug, FALSE if are
						 * using the network. */
Boolean	dbg_UsingNetwork = FALSE;		/* TRUE if the debugger is
						 * using the network interface*/

/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_Init --
 *
 *     Initialize the debugger.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None..
 *
 * ----------------------------------------------------------------------------
 */
void
Dbg_Init()
{
}



/*
 * ----------------------------------------------------------------------------
 *
 * Dbg_getpkt --
 *
 *     Called by the debugging stub to get a packet.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None..
 *
 * ----------------------------------------------------------------------------
 */

void Dbg_getpkt(string)
	char	*string;
{
    if (dbg_Rs232Debug) {
	Dbg_Rs232getpkt(string);
    } else {
	Dbg_IPgetpkt(string);
    }
}



/*
 * ----------------------------------------------------------------------------
 *
 * putpkt --
 *
 *     Called by the debugging stub to put a packet.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void Dbg_putpkt(string)
	char	*string;
{
    if (dbg_Rs232Debug) {
	Dbg_Rs232putpkt(string);
    } else {
	Dbg_IPputpkt(string);
    }
}

