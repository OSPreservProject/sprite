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
    int 	i;
    int		total;

    printf("Sync Statistics\n");
    total = 0;
    for (i = 0; i < mach_NumProcessors; i++) {
	total += sync_Instrument[i].numWakeups;
    }
    printf("numWakeups = %d\n", total);
    total = 0;
    for (i = 0; i < mach_NumProcessors; i++) {
	total += sync_Instrument[i].numWakeupCalls;
    }
    printf("numWakeupCalls = %d\n", total);
    total = 0;
    for (i = 0; i < mach_NumProcessors; i++) {
	total += sync_Instrument[i].numSpuriousWakeups;
    }
    printf("numSpuriousWakeups = %d\n", total);
    total = 0;
    for (i = 0; i < mach_NumProcessors; i++) {
	total += sync_Instrument[i].numLocks;
    }
    printf("numLocks = %d\n", total);
    total = 0;
    for (i = 0; i < mach_NumProcessors; i++) {
	total += sync_Instrument[i].numUnlocks;
    }
    printf("numUnlocks = %d\n", total);
}
