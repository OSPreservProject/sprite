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
    printf("Sync Statistics\n");
    printf("numWakeups = %d ", sync_Instrument.numWakeups);
    printf("numWakeupCalls = %d ", sync_Instrument.numWakeupCalls);
    printf("numSpuriousWakeups = %d ", sync_Instrument.numSpuriousWakeups);
    printf("numLocks = %d ", sync_Instrument.numLocks);
    printf("numUnlocks = %d ", sync_Instrument.numUnlocks);
    printf("\n");
}
