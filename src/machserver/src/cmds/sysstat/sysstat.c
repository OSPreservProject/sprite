/*
 * sysStat.c --
 *
 *	Statistics generation for system calls, and a hook for other
 *	system-related commands.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/cmds/sysstat/RCS/sysstat.c,v 1.8 92/07/13 21:16:17 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include <sysStats.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <option.h>
#include <host.h>
#include <kernel/sync.h>
#include <sys.h>
#include <sprited/sys.h>
#include <sprited/sysCallNums.h>
#include <test.h>

/*
 * Variables for options.
 */

int countCalls = 0;
int serverProcInfo = 0;
int resetCount = 0;
int printVersion = 0;
int enableProfiling = -1;
int doMallocDebug = 0;
#ifdef DO_SYNC_STATS
int doSyncStat = 0;
#endif

Option optionArray[] = {
    {OPT_TRUE, "c", (Address)&countCalls,
     "Print the number of system calls invoked."},
#ifdef DO_SYNC_STATS
    {OPT_TRUE, "l", (Address)&doSyncStat, "Print lock (Sync) statistics"},
#endif
    {OPT_TRUE, "m", (Address)&doMallocDebug,
	 "Print on the console malloc debug information."},
    {OPT_INT, "p", (Address)&enableProfiling,
     "Set or clear the flag that controls system call profiling"},
    {OPT_TRUE, "R", (Address)&resetCount,
     "Reset the system call and Proc_ServerProc instrumentation."},
    {OPT_TRUE, "s", (Address)&serverProcInfo,
     "Print on the console instrumentation for Proc_ServerProcs."},
    {OPT_TRUE, "t", (Address)&countCalls,
     "Print the number of system calls invoked and the time they took."},
    {OPT_TRUE, "v", (Address)&printVersion,
     "Print compilation timestamp for the kernel (DEFAULT)."},
#ifdef DO_SCHED_STATS
    {OPT_TRUE, "x", (Address)&doSchedStat, "Print scheduling statistics"},
#endif
};

/*
 * Constants used by tracing routines:
 * 	PROC_NUM_EVENTS - the number of valid trace events for proc.\
 */

#define PROC_NUM_EVENTS 5


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Driver.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */


main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status = SUCCESS;
    char    	  version[128];
    Sys_MachineInfo	machineInfo;
#ifdef DO_SYNC_STATS
    int			numProcessors;
#endif
    int exitStatus = 0;

    (void) Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray),
		     OPT_ALLOW_CLUSTERING);
    if (! (printVersion || countCalls || resetCount
	   || enableProfiling || serverProcInfo)) {
	printVersion = 1;
    }
    if (printVersion) {
	if (Sys_Stats(SYS_GET_VERSION_STRING, sizeof(version), version) ==
	    SUCCESS) {
	    int virtualHost, physicalHost;
	    Host_Entry *hostPtr;

	    if ((Proc_GetHostIDs(&virtualHost, &physicalHost) != SUCCESS) ||
		((hostPtr = Host_ByID(physicalHost)) == (Host_Entry *)NULL)) {
		(void) printf("%s\n", version);
	    } else {
		char *cPtr;
		/*
		 * Nuke domain suffix and print hostname with kernel version.
		 */
		for (cPtr = hostPtr->name ; *cPtr ; cPtr++) {
		    if (*cPtr == '.') {
			*cPtr = '\0';
			break;
		    }
		}
		(void) printf("%-20s %s\n", hostPtr->name, version);
	    }
	    (void) fflush(stdout);
	}
    }

    if (countCalls) {
	status = PrintNumCalls();
    }
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't get stats about Sprite calls: %s\n",
		Stat_GetMsg(status));
	exitStatus = 1;
    }

    if (resetCount) {
	status = Sys_Stats(SYS_SYS_CALL_STATS, 0, (Address) NULL);
    }
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't reset stats about Sprite calls: %s\n",
		Stat_GetMsg(status));
	exitStatus = 1;
    }

    if (serverProcInfo) {
	status = Sys_Stats(SYS_PROC_SERVERPROC_TIMES, 0, (Address) NULL);
    }
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't display Proc_ServerProc stats: %s.\n",
		Stat_GetMsg(status));
	exitStatus = 1;
    }

    if (enableProfiling != -1) {
	status = Sys_Stats(SYS_SYS_CALL_STATS_ENABLE, enableProfiling,
			   (Address)NULL);
    }
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't %s profiling for Sprite calls: %s\n",
		(enableProfiling ? "enable" : "disable"),
		Stat_GetMsg(status));
	exitStatus = 1;
    }

    if (doMallocDebug) {
	Test_MemCheck("dummyFile");
    }

    status = Sys_GetMachineInfo(sizeof(Sys_MachineInfo), &machineInfo);
    if (status != SUCCESS) {
	printf("Sys_GetMachineInfo failed: %s.\n", Stat_GetMsg(status));
	exit(1);
    }
#ifdef DO_SYNC_STATS
    numProcessors = machineInfo.processors;
    if (doSyncStat) {
	PrintSyncStats(numProcessors);
    }
#endif
#ifdef DO_SCHED_STATS
    if (doSchedStat) {
	PrintSchedStats(numProcessors);
    }
#endif

    exit(exitStatus);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintNumCalls --
 *
 *	Print the number of system calls invoked by processes since the last
 *	time the counter was reset.
 *
 * Results:
 *	The return status from Sys_Stats is returned.
 *
 * Side effects:
 *	The number of calls is output.
 *
 *----------------------------------------------------------------------
 */


int
PrintNumCalls()
{
    Sys_CallCount *countsPtr;	/* Buffer to hold counters */
    ReturnStatus status;
    int i;
    register Sys_CallCount *callPtr; /* pointer into countsPtr */
    char *callName;
    int msec;			/* average milliseconds per call */

    /*
     * Get a copy of the array of counters.
     */

    countsPtr = (Sys_CallCount *)malloc((unsigned)(SYS_NUM_CALLS *
						   sizeof(Sys_CallCount)));
    status = Sys_Stats(SYS_SYS_CALL_STATS, SYS_NUM_CALLS, (Address)countsPtr);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error from PrintNumCalls");
	return(status);
    }

    /* 
     * For each call, print the count, the total and average times to
     * execute the call, average times spent in copyin and copyout, and the 
     * call name.
     */
    printf("%6s %8s %10s\t %12s %12s %s\n", "calls", "time", "avg (ms)",
	   "copyin (ms)", "copyout (ms)", "name");
    for (i = 0, callPtr = countsPtr; i < SYS_NUM_CALLS; i++, callPtr++) {
	callName = Sys_CallName(i);
	if (callName == NULL && callPtr->numCalls == 0) {
	    continue;
	}
	(void)printf("%6d %4d.%03d ", callPtr->numCalls, 
		     callPtr->timeSpent.seconds,
		     (callPtr->timeSpent.microseconds + 500) / 1000);
	if (callPtr->numCalls != 0) {
	    Time_Divide(callPtr->timeSpent, callPtr->numCalls,
			&callPtr->timeSpent);
	    Time_Divide(callPtr->copyInTime, callPtr->numCalls,
			&callPtr->copyInTime);
	    Time_Divide(callPtr->copyOutTime, callPtr->numCalls,
			&callPtr->copyOutTime);
	}
	msec = (callPtr->timeSpent.seconds * 1000 +
		callPtr->timeSpent.microseconds / 1000);
	(void)printf("%6d.%03d\t",
		     msec, callPtr->timeSpent.microseconds % 1000);
	(void)printf("%5d.%06d ", callPtr->copyInTime.seconds,
		     callPtr->copyInTime.microseconds);
	(void)printf("%5d.%06d ", callPtr->copyOutTime.seconds,
		     callPtr->copyOutTime.microseconds);
	/* 
	 * Limit the call name so that the output line fits in 80 columns. 
	 */
	if (callName == NULL || strlen(callName) < 21) {
	    (void)printf("%s\n", callName == NULL ? "<no name>" :
			 callName);
	} else {
	    (void)printf("%-21s\n", callName);
	}
    }
    return(0);
}


#ifdef DO_SYNC_STATS
PrintSyncStats(numProcessors)
    int	numProcessors;	
{
    Sync_Instrument	syncStat[MACH_MAX_NUM_PROCESSORS];
    ReturnStatus	status;
    int			i;

    status = Sys_Stats(SYS_SYNC_STATS, sizeof(syncStat), syncStat);
    if (status != SUCCESS) {
	return;
    }
    printf("Sync Statistics\n");
    for (i = 0; i < numProcessors; i++) {
	printf("Processor %d\n", i);
	printf("numWakeups = %d\n", syncStat[i].numWakeups);
	printf("numWakeupCalls = %d\n", syncStat[i].numWakeupCalls);
	printf("numSpuriousWakeups = %d\n", syncStat[i].numSpuriousWakeups);
	printf("numLocks = %d\n", syncStat[i].numLocks);
	printf("numUnlocks = %d\n", syncStat[i].numUnlocks);
	printf("Misses on sched_Mutex in idle loop = %d\n",
		    syncStat[i].sched_MutexMiss);
    }
}
#endif /* DO_SYNC_STATS */


#ifdef DO_SCHED_STATS
PrintSchedStats(numProcessors)
    int numProcessors;
{
    Sched_Instrument schedStat;
    Time idleTime;
    ReturnStatus	status;
    int			i;

    status = Sys_Stats(SYS_SCHED_STATS, 0, &schedStat);
    if (status != SUCCESS) {
	return;
    }
    printf("Sched Statistics\n");
    for (i = 0; i < numProcessors; i++) {
	printf("Processor %d\n", i);
	printf("numContextSwitches = %d\n", 
		    schedStat.processor[i].numContextSwitches);
	printf("numFullSwitches    = %d\n", schedStat.processor[i].numFullCS);
	printf("numInvoluntary     = %d\n", 
		    schedStat.processor[i].numInvoluntarySwitches);
#ifdef notdef
	printf("onDeckSelf	   = %d\n",
		    schedStat.processor[i].onDeckSelf);
	printf("onDeckOther	   = %d\n",
		    schedStat.processor[i].onDeckOther);
#endif notdef
	printf("Idle Time          = %d.%06d seconds\n",
		   schedStat.processor[i].idleTime.seconds,
		   schedStat.processor[i].idleTime.microseconds);
        printf("Idle ticks = %d * 2^32 + %d.\n", 
		    schedStat.processor[i].idleTicksOverflow,
		    schedStat.processor[i].idleTicksLow);
	printf("Idle ticks/sec = %d.\n", 
		    schedStat.processor[i].idleTicksPerSecond);
    }
}
#endif /* DO_SCHED_STATS */
