/* 
 * rawstat.c --
 *
 *	Print out kerel statitics in a raw format;
 *	<Structure name>
 *	<field> <value>
 *	<field> <value>
 *	<field> <value>
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rawstat/RCS/rawstat.c,v 1.10 90/09/24 14:40:30 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "fsCmd.h"
#include "stdio.h"
#include "option.h"
#include "vm.h"
#include "sysStats.h"
#include "kernel/vm.h"
#include "kernel/fs.h"
#include "kernel/fsStat.h"
#include "kernel/sched.h"
#include "kernel/procMigrate.h"

Boolean doAllFsStats = FALSE;
Boolean doAllVmStats = FALSE;
Boolean doAllRpcStats = FALSE;
Boolean doAllProcStats = FALSE;
Boolean doAllRecovStats = FALSE;
Boolean doAllMigStats = FALSE;
Boolean noIdle	= FALSE;
Boolean zero = FALSE;
Boolean all = FALSE;

Option optionArray[] = {
    {OPT_TRUE, "fs", (Address)&doAllFsStats, "Print ALL file system stats"},
    {OPT_TRUE, "vm", (Address)&doAllVmStats, "Print ALL vm stats"},
    {OPT_TRUE, "rpc", (Address)&doAllRpcStats, "Print ALL rpc stats"},
    {OPT_TRUE, "proc", (Address)&doAllProcStats, "Print ALL proc stats"},
    {OPT_TRUE, "recov", (Address)&doAllRecovStats, "Print ALL recov stats"},
    {OPT_TRUE, "mig", (Address)&doAllMigStats, "Print ALL mig stats"},
    {OPT_TRUE, "noidle", (Address)&noIdle, "Don't print idle ticks info"},
    {OPT_TRUE, "zero", (Address)&zero, "Print zero valued stats"},
    {OPT_TRUE, "all", (Address)&all, "Print all stats"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

Fs_Stats fsStats;
Vm_Stat	vmStats;
Fs_TypeStats fsTypeStats;

main(argc, argv)
    int argc;
    char *argv[];
{
    int status = SUCCESS;
    int virtualHost, physicalHost;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);

    if (all) {
	doAllFsStats = TRUE;
	doAllVmStats = TRUE;
	doAllRpcStats = TRUE;
	doAllProcStats = TRUE;
	doAllRecovStats = TRUE;
	doAllMigStats = TRUE;
    }

    system("echo RAWSTAT `hostname` `date`");

    if (!noIdle) {
	PrintRawIdle();
    }

    if (doAllFsStats) {
	status = Fs_Command(FS_RETURN_STATS, sizeof(Fs_Stats), &fsStats);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Fs_Command(FS_RETURN_STATS)");
	} else if (fsStats.statsVersion == FS_STAT_VERSION) {
	    PrintRawFsCltName(&fsStats.cltName);
	    PrintRawFsSrvName(&fsStats.srvName);
	    PrintRawFsGen(&fsStats.gen);
	    PrintRawFsBlockCache(&fsStats.blockCache);
	    PrintRawFsAlloc(&fsStats.alloc);
	    PrintRawFsNameCache(&fsStats.nameCache);
	    PrintRawFsHandle(&fsStats.handle);
	    PrintRawFsPrefix(&fsStats.prefix);
	    PrintRawFsLookup(&fsStats.lookup);
	    PrintRawFsObject(&fsStats.object);
	    PrintRawFsRecovery(&fsStats.recovery);
	    PrintRawFsConsist(&fsStats.consist);
	    PrintRawFsWriteBack(&fsStats.writeBack);
	    PrintRawRemoteIO(&fsStats.rmtIO);
	    PrintRawFsMig(&fsStats.mig);
	} else {
	    fprintf(stderr,
		    "Wrong version of Fs_Stats: kernel is %d, while ours is %d.\n",
		    fsStats.statsVersion, FS_STAT_VERSION);
	}
    }
    if (doAllVmStats) {
	int pageSize;
	status = Vm_Cmd(VM_GET_STATS, &vmStats);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Vm_Cmd failed");
	    exit(status);
	}
	Vm_PageSize(&pageSize);
	PrintRawVmStat(&vmStats);
	printf("\tpagesize %d\n", pageSize);
    }
    if (doAllRpcStats) {
	PrintRawRpcCltStat();
	PrintRawRpcSrvStat();
	PrintSrvCount();
	PrintCallCount();
    }
    if (doAllProcStats) {
	PrintRawProcMigStat();
    }
    if (doAllMigStats) {
	PrintRawMigStat();
    }
    if (doAllRecovStats) {
	PrintRawRecovStat();
    }
    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRawIdle --
 *
 *	Prints the raw idle ticks of the machine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintRawIdle()
{
    Sched_Instrument schedStats;
    struct perProcessor *X = &schedStats.processor[0];
    ReturnStatus status;

    status = Sys_Stats(SYS_SCHED_STATS, 0, &schedStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_Stats");
	exit(status);
    }
	printf("numContextSwitches %8u\n", X->numContextSwitches);
	printf("numInvoluntarySwitches %8u\n", X->numInvoluntarySwitches);
	printf("numFullCS      %8u\n", X->numFullCS);
	printf("idleTime       %8d.%06d\n", X->idleTime.seconds,
					    X->idleTime.microseconds);
	printf("idleTicksLow   %8u\n", X->idleTicksLow);
	printf("idleTicksOverflow %8u\n", X->idleTicksOverflow);
	printf("idleTicksPerSecond %8u\n", X->idleTicksPerSecond);
	printf("noInput %8d.%06d\n", schedStats.noUserInput.seconds,
	       schedStats.noUserInput.microseconds);
	printf("\n");

}

/*
 *----------------------------------------------------------------------
 *
 * ZeroPrint --
 *
 *	Prints a field if non-zero or if the global variable zero is set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ZeroPrint(format, value)
    char *format;
    int value;
{
    if (zero || value != 0) {
	printf(format, value);
    }
}

