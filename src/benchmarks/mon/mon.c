/*
 * mon.c --
 *	Benchmark to time monitor locks and condition variables.
 */
#include "sprite.h"
#include "option.h"
#include "syncMonitor.h"
#include "time.h"
#include "proc.h"
#include "stdio.h"
#include "sysStats.h"
#include "kernel/sched.h"

static Sync_Lock	lock;
#define	LOCKPTR (&lock)
static Sync_Condition	condition;

int	numIterations = 1000;
Boolean	parHigh = FALSE;
Boolean	childHigh = FALSE;

Sched_Instrument startSchedStats, endSchedStats;

Option optionArray[] = {
    {OPT_INT, "n", (Address)&numIterations, 
     "Number of iterations (Default 1000)."},
    {OPT_TRUE, "p", (Address)&parHigh,
    "Make parent high priority."},
    {OPT_TRUE, "c", (Address)&childHigh,
     "Make child high priority."},

};
int	numOptions = Opt_Number(optionArray);
int	i = 0;

main(argc, argv)
    int	 argc;
    char *argv[];
{
    register	int	numTimes;
    int			pid;
    Time		before, after;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);
    numTimes = numIterations;
    if (Proc_Fork(TRUE, &pid) == PROC_CHILD_PROC) {
	LOCK_MONITOR;

	if (childHigh) {
	    Proc_SetPriority(PROC_MY_PID, PROC_NO_INTR_PRIORITY, FALSE);
	}
	while (numTimes > 0) {
	    while (i == 0) {
		Sync_Wait(&condition, FALSE);
	    }
	    i = 0;
	    Sync_Broadcast(&condition);
	    numTimes--;
	}
	UNLOCK_MONITOR;
    } else {
	LOCK_MONITOR;

	if (parHigh) {
	    Proc_SetPriority(PROC_MY_PID, PROC_NO_INTR_PRIORITY, FALSE);
	}
	Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
	Sys_GetTimeOfDay(&before, NULL, NULL);
	while (numTimes > 0) {
	    while (i == 1) {
		Sync_Wait(&condition, FALSE);
	    }
	    i = 1;
	    Sync_Broadcast(&condition);
	    numTimes--;
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);

	UNLOCK_MONITOR;
	Time_Subtract(after, before, &after);
	printf("%d wait-broadcasts at %dus each\n", numIterations,
	    (after.seconds * 1000000 + after.microseconds) / numIterations);
	PrintIdleTime(stderr, &startSchedStats, &endSchedStats, &after);
    }
}
