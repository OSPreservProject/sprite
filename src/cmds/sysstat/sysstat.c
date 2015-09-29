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
static char rcsid[] = "$Header: /sprite/src/cmds/sysstat/RCS/sysstat.c,v 1.8 92/06/02 13:21:38 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <spriteTime.h>
#include <fs.h>
#include <sysStats.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <option.h>
#include <host.h>
#include <kernel/sysSysCall.h>
#include <kernel/trace.h>
#include <kernel/sync.h>
#include <kernel/sched.h>
#include <kernel/mach.h>

#include "syscalls.h"

/*
 * Variables for options.
 */

int countCalls = 0;
#define JUST_CALLS	1
#define CALLS_AND_TIMES	2

int resetCount = 0;
int printVersion = 0;
int doSyncStat = 0;
int doSchedStat = 0;
int enableProfiling = -1;

Option optionArray[] = {
    {OPT_CONSTANT(JUST_CALLS), "c", (Address)&countCalls,
     "Print the number of system calls invoked."},
    {OPT_TRUE, "l", (Address)&doSyncStat, "Print lock (Sync) statistics"},
    {OPT_INT, "p", (Address)&enableProfiling,
     "Set or clear the flag that controls system call profiling"},
    {OPT_TRUE, "R", (Address)&resetCount,
     "Reset the count of system calls to 0."},
    {OPT_CONSTANT(CALLS_AND_TIMES), "t", (Address)&countCalls,
     "Print the number of system calls invoked and the time they took."},
    {OPT_TRUE, "v", (Address)&printVersion,
     "Print compilation timestamp for the kernel (DEFAULT)."},
    {OPT_TRUE, "x", (Address)&doSchedStat, "Print scheduling statistics"},
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
    int			numProcessors;
    int exitStatus = 0;

    (void) Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray),
		     OPT_ALLOW_CLUSTERING);
    if (! (printVersion || countCalls || resetCount)) {
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
	status = PrintNumCalls(countCalls);
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

    status = Sys_GetMachineInfo(sizeof(Sys_MachineInfo), &machineInfo);
    if (status != SUCCESS) {
	printf("Sys_GetMachineInfo failed: %s.\n", Stat_GetMsg(status));
	exit(1);
    }
    numProcessors = machineInfo.processors;
    if (doSyncStat) {
	PrintSyncStats(numProcessors);
    }
    if (doSchedStat) {
	PrintSchedStats(numProcessors);
    }

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

ReturnStatus
PrintNumCalls(callType)
    int callType;		/* just calls, or times too */
{
    Address buffer;	/* Buffer to hold counters */
    int bufSize;
    ReturnStatus status;
    int i;
    int numForeign = 0;
    int numLocal = 0;
    register int *numCalls;	/* array of counts (one per call) */
    Time *totalTimes;		/* array of times (one per call) */
    int msec;
    int numAlloc;		/* number of entries to allocate space for */

    /*
     * Allocate an array to hold the numbers from the kernel.  This will 
     * either be an array of counters or an array of counters followed by 
     * an array of Time values.
     */
    numAlloc = sysCallArraySize / sizeof(SysCallInfo);
    bufSize = numAlloc * (callType == JUST_CALLS
			  ? sizeof(int)
			  : sizeof(int) + sizeof(Time));
    buffer = malloc((unsigned) bufSize);

    /* 
     * Now get the numbers from the kernel.
     */
    status = Sys_Stats((callType == JUST_CALLS
			? SYS_SYS_CALL_STATS
			: SYS_SYS_CALL_TIMES),
		       numAlloc, buffer);
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't get %s: %s\n",
		(callType == JUST_CALLS ? "counters" : "counters and times"),
		 Stat_GetMsg(status));
	return(status);
    }

    /* 
     * Print out the results.  The format depends on whether we give the 
     * time as well as the count.
     */
    numCalls = (int *) buffer;
    totalTimes = (Time *)((int *)buffer + numAlloc);

    for (i = 0; i < numAlloc; i++) {
	if (callType == JUST_CALLS) {
	    (void) printf("%d\t%-30s", numCalls[i], sysCallArray[i].name);
	    if (sysCallArray[i].local) {
		numLocal += numCalls[i];
		(void) printf("local\n");
	    } else {
		numForeign += numCalls[i];
		(void) printf("remote\n");
	    }
	} else {
	    (void)printf("%d\t%d.%03d\t", numCalls[i], 
			 totalTimes[i].seconds,
			 (totalTimes[i].microseconds + 500) / 1000);
	    if (numCalls[i] != 0) {
		Time_Divide(totalTimes[i], numCalls[i],
			    &totalTimes[i]);
	    }
	    msec = (int)Time_ToMs(totalTimes[i]);
	    if (msec < 10 && !Time_EQ(totalTimes[i], time_ZeroSeconds)) {
		(void)printf("(%d.%03d ms avg)\t",
			     msec, totalTimes[i].microseconds % 1000);
	    } else {
		(void)printf("(%d ms avg)\t",
			     (int)(Time_ToMs(totalTimes[i]) + 0.5));
	    }
	    (void)printf("%-30s\n", sysCallArray[i].name);
	}
    }
    if (callType == JUST_CALLS) {
	(void) printf("\n\nTotal number of calls: %d local, %d remote.\n",
		      numLocal, numForeign);
	(void) printf("%d/%d = %6.2f%% remote.\n",
		      numForeign, numForeign + numLocal, 
		      ((double) numForeign) / (numForeign + numLocal) * 100.);
    }
    return(0);
}

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

PrintSchedStats(numProcessors)
    int numProcessors;
{
    Sched_Instrument schedStat;
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

