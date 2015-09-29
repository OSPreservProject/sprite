/* 
 * printMachDepStats.c --
 *
 *	Routine to print out machine dependent vm statistics.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.2 89/01/07 04:12:18 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */


/*
 *----------------------------------------------------------------------
 *
 * PrintMachDepStats --
 *
 *	Prints out machine dependent vm statistics.
 *	Not implemented on the spur.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed to the stream.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
PrintMachDepStats(statsPtr, stream)
    VmMachDepStat	*statsPtr;  	/* Ptr at stats structure. */
    FILE		*stream;	/* Stream to print to. */
{
}

