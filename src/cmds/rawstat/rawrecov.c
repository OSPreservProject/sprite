/* 
 * rawproc.c --
 *
 *	Print raw format PROC statistics.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rawstat/RCS/rawrecov.c,v 1.2 89/10/24 10:14:52 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "stdio.h"
#include "sysStats.h"
#include "kernel/recov.h"


/*
 *----------------------------------------------------------------------
 *
 * PrintRawProcMigStat --
 *
 *	Prints proc_MigStats.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawRecovStat()
{
    Recov_Stats stats;		/* statistics buffer */
    Recov_Stats *X = &stats;
    int status;

    bzero((Address) &stats, sizeof(stats));
    status = Sys_Stats(SYS_RECOV_STATS, sizeof(stats), (Address) &stats);
    if (status != SUCCESS) {
	return;
    }

    printf("Recov_Stats\n");

    ZeroPrint("packets        %8u\n", X->packets);
    ZeroPrint("pings          %8u\n", X->pings);
    ZeroPrint("suppressed     %8u\n", X->pingsSuppressed);
    ZeroPrint("timeouts       %8u\n", X->timeouts);
    ZeroPrint("crashes        %8u\n", X->crashes);
    ZeroPrint("reboots        %8u\n", X->reboots);
    ZeroPrint("hostsMonitored %8u\n", X->numHostsPinged);
}
