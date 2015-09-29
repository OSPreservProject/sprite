/*
 * Driver for the new (Dec 87) pseudo-device implementation.
 *
 * The programs output goes to standard out, while the
 * statistics taken go to error output or another file.
 */

#include "sprite.h"
#include "status.h"
#include "stdio.h"
#include "fs.h"
#include "fsCmd.h"
#include "sysStats.h"
#include "rpc.h"
#include "proc.h"
#include "vm.h"
#include "kernel/sched.h"
#include "kernel/fsStat.h"
#include "kernel/vm.h"
#include "option.h"

Boolean useSelect = FALSE;
Boolean copy = FALSE;
Boolean readP = FALSE;
Boolean writeP = FALSE;
Boolean ioctlP = FALSE;
Boolean selectP = FALSE;
Boolean zero = FALSE;
Boolean switchBuf = FALSE;
Boolean writeBehind = FALSE;
int requestBufSize = 2048;
int size = 128;
int reps = 10;
int numClients = -1;			/* For multi-program synchronization,
					 * this is the number of slaves with
					 * which to synchronize */
int delay = 0;				/* Delay loop for server to eat CPU */
Boolean slave = FALSE;
extern char *pdev;

Option optionArray[] = {
	{OPT_INT, "srvr", (Address)&numClients,
		"Server for -srvr (int) clients"},
	{OPT_TRUE, "clnt", (Address)&slave,
		"Client process"},
	{OPT_INT, "reps", (Address)&reps,
		"Number of repetitions"},
	{OPT_INT, "size", (Address)&size,
		"Amount of data to transfer"},
	{OPT_TRUE, "selectTest", (Address)&selectP,
		"Select, makes server block client"},
	{OPT_TRUE, "fork", (Address)&copy,
		"Fork client process (with -clnt)"},
	{OPT_TRUE, "read", (Address)&readP,
		"Read test"},
	{OPT_TRUE, "write", (Address)&writeP,
		"Write test"},
	{OPT_TRUE, "writeBehind", (Address)&writeBehind,
		"Enable write behind (with -srvr)"},
	{OPT_TRUE, "ioctl", (Address)&ioctlP,
		"IOControl test"},
	{OPT_INT, "delay", (Address)&delay,
		"Loop for -delay <int> cylces before replying"},
	{OPT_TRUE, "switchBuf", (Address)&switchBuf,
		"Test switching request buffers (use with -ioctl)"},
	{OPT_STRING, "pdev", (Address)&pdev,
		"Name of the pseudo device"},
	{OPT_INT, "bufsize", (Address)&requestBufSize,
		"Size of the pseudo device request buffer"},
	{OPT_TRUE, "forceSelect", (Address)&useSelect,
		"Make Master use select with 1 client"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

Time startTime, endTime;
Sched_Instrument startSchedStats, endSchedStats;

main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;
    Proc_PID child;
    Proc_ResUsage usage;
    FILE *outStream;
    register int i;
    ClientData serverData, clientData;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);
    if (!slave && (numClients < 0)) {
	fprintf(stderr, "Server: %s [options] -srvr numClients\n",  argv[0]);
	fprintf(stderr, "Client: %s [options] -clnt\n", argv[0]);
	Opt_PrintUsage(argv[0], numOptions, optionArray);
	exit(1);
    }
    /*
     * Check for multi-program master/slave setup.
     */
    if (numClients > 0) {
	ServerSetup(numClients, &serverData);
	slave = FALSE;
    }
    if (slave) {
	ClientSetup(&clientData);
    }
    /*
     * Get first sample of time and idle ticks.
     */
    status = Sys_Stats(SYS_SCHED_STATS, 0, &startSchedStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_Stats");
	exit(status);
    }

    status = Sys_GetTimeOfDay(&startTime, NULL, NULL);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_GetTimeOfDay");
	exit(status);
    }
    if (slave) {
	if (copy) {
	    status = Proc_Fork(TRUE, &child);
	    if (status != PROC_CHILD_PROC && status != SUCCESS) {
		Stat_PrintMsg(status, "Fork failed");
		exit(status);
	    }
	}
	if (readP) {
	    ClientRead(clientData, size, reps);
	}
	if (writeP) {
	    ClientWrite(clientData, size, reps);
	}
	if (ioctlP) {
	    ClientIOControl(clientData, size, reps);
	}
	if (copy) {
	    if (status != PROC_CHILD_PROC) {
		status = Proc_Wait(0, NULL, TRUE, NULL, NULL, NULL, NULL, &usage);
	    } else {
		exit(SUCCESS);
	    }
	}
	/*
	 * Take ending statistics and print user, system, and elapsed times.
	 */
	Sys_GetTimeOfDay(&endTime, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	Time_Subtract(endTime, startTime, &endTime);
	printf("Slave (reps = %d) ", reps);
	PrintTimes(stdout, &usage, &endTime);

	ClientDone(clientData);
	/*
	 * Print FS statistics.
	 */
    } else {
	/*
	 * Drop into the top level service loop.  ServeOne is a faster
	 * version that doesn't use select because there is only
	 * one client.
	 */
	if (numClients > 1 || useSelect) {
	    Serve(serverData);
	} else {
	    ServeOne(serverData);
	}
	/*
	 * Take ending statistics and print user, system, and elapsed times.
	 */
	Sys_GetTimeOfDay(&endTime, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, &endSchedStats);
	Time_Subtract(endTime, startTime, &endTime);
	printf("Master: ");
	PrintTimes(stdout, &usage, &endTime);

	if (delay > 0) {
	    int cnt;
	    Sys_GetTimeOfDay(&startTime, NULL, NULL);
	    for (cnt=0 ; cnt<50 ; cnt++) {
		for (i=delay<<1 ; i>0 ; i--) ;
	    }
	    Sys_GetTimeOfDay(&endTime, NULL, NULL);
	    Time_Subtract(endTime, startTime, &endTime);
	    Time_Divide(endTime, 50, &endTime);
	    printf("%d.%06d seconds service delay\n",
		endTime.seconds, endTime.microseconds);
	}
    }
    exit(status);
}
