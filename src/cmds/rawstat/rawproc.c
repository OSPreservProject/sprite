/* 
 * rawproc.c --
 *
 *	Print raw format PROC statistics.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rawstat/RCS/rawproc.c,v 1.3 90/09/24 14:40:31 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "stdio.h"
#include "sysStats.h"
#include "kernel/procMigrate.h"


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

PrintRawProcMigStat()
{
    Proc_MigStats stats;		/* statistics buffer */
    Proc_MigStats *X = &stats;
    int status;

    /*
     * Get a copy of the trace table.  Make sure it's zeroed in case the
     * kernel provides us with a shorter (older) structure.
     */

    bzero((Address) &stats, sizeof(stats));
    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATS,
		       (Address) &stats);
    if (status != SUCCESS) {
	return;
    }
    if (stats.statsVersion != PROC_MIG_STATS_VERSION) {
	return;
    }

    printf("proc_MigStats\n");


    ZeroPrint("statsVersion   %8u\n", X->statsVersion);
    ZeroPrint("foreign        %8u\n", X->foreign);
    ZeroPrint("remote         %8u\n", X->remote);
    ZeroPrint("exports        %8u\n", X->exports);
    ZeroPrint("execs          %8u\n", X->execs);
    ZeroPrint("imports        %8u\n", X->imports);
    ZeroPrint("errors         %8u\n", X->errors);
    ZeroPrint("evictions      %8u\n", X->varStats.evictions);
    ZeroPrint("evictions:squared      %8u\n", X->squared.evictions);
    ZeroPrint("returns        %8u\n", X->returns);
    ZeroPrint("pagesWritten   %8u\n", X->varStats.pagesWritten);
    ZeroPrint("pagesWritten:squared   %8u\n", X->squared.pagesWritten);

    ZeroPrint("timeToMigrate  %8u\n", X->varStats.timeToMigrate);
    ZeroPrint("timeToMigrate:squared  %8u\n", X->squared.timeToMigrate);
    ZeroPrint("timeToExec     %8u\n", X->varStats.timeToExec);
    ZeroPrint("timeToExec:squared     %8u\n", X->squared.timeToExec);
    ZeroPrint("timeToEvict    %8u\n", X->varStats.timeToEvict);
    ZeroPrint("timeToEvict:squared    %8u\n", X->squared.timeToEvict);
    ZeroPrint("totalEvictTime    %8u\n", X->varStats.totalEvictTime);
    ZeroPrint("totalEvictTime:squared    %8u\n", X->squared.totalEvictTime);
    ZeroPrint("totalCPUTime   %8u\n", X->varStats.totalCPUTime);
    ZeroPrint("totalCPUTime:squared   %8u\n", X->squared.totalCPUTime);
    ZeroPrint("remoteCPUTime  %8u\n", X->varStats.remoteCPUTime);
    ZeroPrint("remoteCPUTime:squared  %8u\n", X->squared.remoteCPUTime);
    ZeroPrint("evictionCPUTime  %8u\n", X->varStats.evictionCPUTime);
    ZeroPrint("evictionCPUTime:squared  %8u\n", X->squared.evictionCPUTime);
    ZeroPrint("rpcKbytes      %8u\n", X->varStats.rpcKbytes);
    ZeroPrint("rpcKbytes:squared      %8u\n", X->squared.rpcKbytes);
    ZeroPrint("migrationsHome %8u\n", X->migrationsHome);
    ZeroPrint("evictCalls     %8u\n", X->evictCalls);
    ZeroPrint("evictsNeeded   %8u\n", X->evictsNeeded);
    ZeroPrint("evictionsToUs   %8u\n", X->evictionsToUs);
    ZeroPrint("processes   %8u\n", X->processes);
}
