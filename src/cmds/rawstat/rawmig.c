/* 
 * rawmig.c --
 *
 *	Print raw format MIGRATION statistics.
 *
 * Copyright (C) 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rawstat/RCS/rawmig.c,v 1.2 90/09/24 14:40:32 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "stdio.h"
#include "mig.h"


/*
 *----------------------------------------------------------------------
 *
 * PrintRawMigStat --
 *
 *	Prints Mig_Stats structure maintained by migration daemon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Opens connection to daemon.
 *
 *----------------------------------------------------------------------
 */

PrintRawMigStat()
{
    Mig_Stats stats;		/* statistics buffer */
    Mig_Stats *X = &stats;
    Mig_ArchStats *A;
    int status;
    int i;
    int j;
    double overflowFactor;

    if (Mig_GetStats(&stats) < 0) {
	return;
    }
#ifdef MIG_STATS_VERSION
    if (stats.version != MIG_STATS_VERSION) {
	return;
    }
#endif
    printf("Migration daemon stats\n");


    ZeroPrint("version   %8u\n", X->version);
    ZeroPrint("checkpointInterval        %8u\n", X->checkpointInterval);
    ZeroPrint("firstRun         %8u\n", X->firstRun);
    ZeroPrint("restarts        %8u\n", X->restarts);
    ZeroPrint("intervals          %8u\n", X->intervals);
    ZeroPrint("maxArchs        %8u\n", X->maxArchs);
    ZeroPrint("getLoadRequests         %8u\n", X->getLoadRequests);
    ZeroPrint("totalRequests         %8u\n", X->totalRequests);
    ZeroPrint("totalObtained         %8u\n", X->totalObtained);
    ZeroPrint("numRepeatRequests         %8u\n", X->numRepeatRequests);
    ZeroPrint("numRepeatAssignments         %8u\n", X->numRepeatAssignments);
    ZeroPrint("numFirstAssignments         %8u\n", X->numFirstAssignments);

    for (i = 0; i < X->maxArchs; i++) {
	A = &X->archStats[i];
	if (strcmp(A->arch, "sun4") && strcmp(A->arch, "ds3100")) {
	    continue;
	}
	printf("Migration daemon %s stats\n", A->arch);

	ZeroPrint("numClients         %8u\n", A->numClients);
	ZeroPrint("gotAll         %8u\n", A->gotAll);

	for (j = 0; j <= MIG_MAX_HOSTS_DIST; j++) {
	    printf("request%d         %8u\n", j, A->requestDist[j]);
	    printf("obtained%d         %8u\n", j, A->obtainedDist[j]);
	}
	ZeroPrint("nonIdleTransitions         %8u\n", A->nonIdleTransitions);

	ZeroPrint("requested         %8u\n", A->counters.requested);
	ZeroPrint("requested:square         %8u\n", A->squared.requested);
	ZeroPrint("obtained         %8u\n", A->counters.obtained);
	ZeroPrint("obtained:square         %8u\n", A->squared.obtained);
	ZeroPrint("evicted         %8u\n", A->counters.evicted);
	ZeroPrint("evicted:square         %8u\n", A->squared.evicted);
	ZeroPrint("reclaimed         %8u\n", A->counters.reclaimed);
	ZeroPrint("reclaimed:square         %8u\n", A->squared.reclaimed);
	ZeroPrint("timeUsed         %8u\n", A->counters.timeUsed);
	ZeroPrint("timeUsed:square         %8u\n", A->squared.timeUsed);
	ZeroPrint("timeToEviction         %8u\n", A->counters.timeToEviction);
	ZeroPrint("timeToEviction:square         %8u\n",
		  A->squared.timeToEviction);

	/*
	 * For "pseudo-double" variables -- two integers -- we need to store
	 * them in this program as real double variables.  The problem is,
	 * we can't just convert 0x80000000 from an unsigned int to float, because
	 * our C compiler goofs and makes the result negative.  Therefore, some
	 * tricks to take the constant and turn it into a number
	 * we can use to multiply by the overflow counter.  Shift it right, then
	 * multiply it by the equivalent number once it's a "double".  
	 */
	overflowFactor = (((unsigned) 0x80000000) >> 1) * 2.0;
    
#define TO_DOUBLE(array) \
	(array[MIG_COUNTER_LOW] + array[MIG_COUNTER_HIGH] * overflowFactor)

	printf("hostIdleObtained         %8.0f\n",
		  TO_DOUBLE(A->counters.hostIdleObtained));
	printf("hostIdleObtained:square         %8.0f\n",
		  TO_DOUBLE(A->squared.hostIdleObtained));
	printf("hostIdleEvicted         %8.0f\n",
		  TO_DOUBLE(A->counters.hostIdleEvicted));
	printf("hostIdleEvicted:square         %8.0f\n",
		  TO_DOUBLE(A->squared.hostIdleEvicted));
	printf("idleTimeWhenActive         %8.0f\n",
		  TO_DOUBLE(A->counters.idleTimeWhenActive));
	printf("idleTimeWhenActive:square         %8.0f\n",
		  TO_DOUBLE(A->squared.idleTimeWhenActive));
	
	for (j = 0; j < MIG_NUM_STATES; j++) {
	    printf("state%d         %8u\n", j, A->counters.hostCounts[j]);
	    printf("state%d:square         %8u\n", j, A->squared.hostCounts[j]);
	}
    }
}


