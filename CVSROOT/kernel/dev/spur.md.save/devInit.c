/*
 * devInit.c --
 *
 *	This has a weak form of autoconfiguration for mapping in various
 *	devices and calling their initialization routines.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "sysStats.h"

/*
 * ----------------------------------------------------------------------------
 *
 * Dev_Init --
 *
 *	Device initialization routines are called
 *	later by Dev_Config.
 *
 * Results:
 *     none
 *
 * Side effects:
 *     Some devices are initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Dev_Init()
{

}

/*
 *----------------------------------------------------------------------
 *
 * Dev_Config --
 *
 *	Call the initialization routines for various controllers and
 *	devices based on configuration tables.  This should be called
 *	after the regular memory allocater is set up so the drivers
 *	can use it to configure themselves.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Call the controller and device initialization routines.
 *
 *----------------------------------------------------------------------
 */
void
Dev_Config()
{
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_GatherDiskStats --
 *
 *	Determine which disks are idle.
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
Dev_GatherDiskStats()
{
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_GetDiskStats --
 *
 *	Return statistics about the different disks.
 *
 * Results:
 *	Number of statistics entries returned.
 *
 * Side effects:
 *	Entries in *diskStatPtr filled in.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Dev_GetDiskStats(diskStatArr, numEntries)
    register Sys_DiskStats *diskStatArr;	/* Where to store the disk 
						 * stats. */
    int			   numEntries;		/* The number of elements in 
						 * diskStatArr. */
{
}
