/*
 * A program to test and calibrate the idle time counter.
 */

#include "sprite.h"
#include "status.h"
#include "stdio.h"
#include "proc.h"
#include "sysStats.h"
#include "kernel/sched.h"
#include "option.h"

Time time = {10, 0};

Option optionArray[] = {
	{OPT_INT, "s", (Address)&time.seconds, "Seconds to sleep\n"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

Time startTime, endTime;
Sched_Instrument startSchedStats, endSchedStats;

main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;

    status = Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);
    fprintf(stderr, "Sleeping for %d seconds...", time.seconds);
    fflush(stderr);
    status = Sys_GetTimeOfDay(&startTime, NULL, NULL);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_GetTimeOfDay");
	exit(status);
    }
    status = Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_Stats");
	exit(status);
    }
    /*
     * Sleep for a while...
     */
    Sync_WaitTime(time);
    /*
     * Take ending statistics and print user, system, and elapsed times.
     */
    Sys_GetTimeOfDay(&endTime, NULL, NULL);
    Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
    Time_Subtract(endTime, startTime, &endTime);
    PrintIdleTime(stderr, &startSchedStats, &endSchedStats, &endTime);

    exit(status);
}
