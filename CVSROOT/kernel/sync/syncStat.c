/* 
 * syncStat.c --
 *
 *	Keep and print statistics for the sync module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "sync.h"


/*
 *----------------------------------------------------------------------
 *
 * Sync_PrintStat --
 *
 *	Print the sync module statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Do the prints.
 *
 *----------------------------------------------------------------------
 */
void
Sync_PrintStat()
{
    Sys_Printf("Sync Statistics\n");
    Sys_Printf("numWakeups = %d ", sync_Instrument.numWakeups);
    Sys_Printf("numWakeupCalls = %d ", sync_Instrument.numWakeupCalls);
    Sys_Printf("numSpuriousWakeups = %d ", sync_Instrument.numSpuriousWakeups);
    Sys_Printf("numLocks = %d ", sync_Instrument.numLocks);
    Sys_Printf("numUnlocks = %d ", sync_Instrument.numUnlocks);
    Sys_Printf("\n");
}
