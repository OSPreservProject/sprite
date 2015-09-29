/*
 * A standard harness with which to benchmark programs.
 * This sets up statistics stuff, then forks and execs a
 * program to benchmark.  When the program completes, its
 * resource usage is recorded, as well as other filesystem
 * related statistics.
 *
 * The programs output goes to standard out, while the
 * statistics taken go to error output or another file.
 */

#include "sprite.h"
#include "status.h"
#include "sys/ioctl.h"
#include "sys/file.h"
#include "fs.h"
#include "fsCmd.h"
#include "sysStats.h"
#include "proc.h"
#include "vm.h"
#include "kernel/sched.h"
#include "kernel/fsStat.h"
#include "kernel/vm.h"
#include "kernel/net.h"
#include "option.h"
#include "sig.h"
#include "signal.h"
#include "stdio.h"
#ifdef spur
#include "ccMachStat.h"
#endif
Boolean flushCache = FALSE;
Boolean clean = FALSE;
Boolean exec = FALSE;
Boolean histogram = FALSE;
int numClients = -1;			/* For multi-program synchronization,
					 * this is the number of slaves with
					 * which to synchronize */
Boolean slave = FALSE;
char *outFile = "bench.out";
extern char *pdev;
Boolean	dontSyncCache = FALSE;
Boolean	useSignals = FALSE;
int	pause = 0;
Boolean waitForSignal = FALSE;

Option optionArray[] = {
	{OPT_STRING, "o", (Address)&outFile,
		"Output file name\n"},
	{OPT_STRING, "p", (Address)&pdev,
		"Name of the master pseudo device\n"},
	{OPT_TRUE, "f", (Address)&flushCache,
		"Flush cache before benchmark"},
	{OPT_TRUE, "x", (Address)&clean,
		"Turn off all tracing"},
	{OPT_TRUE, "h", (Address)&histogram,
		"Leave histograms on (ok with -x)"},
	{OPT_TRUE, "S", (Address)&slave,
		"Slave bench program"},
	{OPT_INT, "M", (Address)&numClients,
		"Master for -M (int) clients"},
	{OPT_TRUE, "d", (Address)&dontSyncCache,
		"Dont sync the cache when done"},
	{OPT_TRUE, "s", (Address)&useSignals,
"Use signals to rendevous the master and slave instead of pseudo-devices.\n"},
	{OPT_INT, "P", (Address)&pause,
		"Seconds to pause before get final stats"},
	{OPT_REST, "c", (Address)&exec,
		"(Follow with command to benchmark)"},
	{OPT_TRUE, "w", (Address) &waitForSignal,
		"Master waits for USR1 signal before starting slaves.\n"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

Fs_Stats fsStartStats, fsEndStats;
Vm_Stat	vmStartStats, vmEndStats;
Sys_DiskStats	diskStartStats[10], diskEndStats[10];

Time startTime, endTime;
Sched_Instrument startSchedStats, endSchedStats;
Net_EtherStats	netStartStats, netEndStats;

#ifdef spur
MachStats startMachStats, endMachStats;
#endif

#define NUM_PCBS	256
Proc_ControlBlock pcbs1[NUM_PCBS];
Proc_ControlBlock pcbs2[NUM_PCBS];
Proc_PCBArgString argStrings1[NUM_PCBS];
Proc_PCBArgString argStrings2[NUM_PCBS];
int numPCB1, numPCB2;

extern void ServerSetup();
extern void Serve();

extern void ClientSetup();
extern void ClientDone();

extern void PrintTimes();
extern void PrintIdleTime();
extern void PrintFsStats();
extern void PrintVmStats();
extern void PrintDiskStats();

extern int errno;

Boolean	gotSig = FALSE;
Boolean startClients = FALSE;

main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;
    Proc_PID child;
    Proc_ResUsage usage;
    FILE *outStream;
    int i;
    ClientData serverData, clientData;
    int	mastPID;
    int	pidFD;


    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);
    if (!exec && (numClients < 0)) {
	fprintf(stderr, "Master: %s [-xfh] -M numSlaves\n", argv[0]);
	fprintf(stderr, 
		"Slave: %s [-xfh] -S -c commandPathName flags...\n", argv[0]);
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if ((numClients < 0) && waitForSignal) {
	fprintf(stderr,"The -w flag can only be used with the -M flag.\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if (waitForSignal) {
	int HandleUSR1();
	(void) signal(SIGUSR1, HandleUSR1);
    }
    if (clean) {
	int newValue;
	newValue = 0;
	Fs_Command(FS_SET_CACHE_DEBUG, sizeof(int), (Address) &newValue);
	newValue = 0;
	Fs_Command(FS_SET_TRACING, sizeof(int), (Address) &newValue);
	newValue = 0;
	Fs_Command(FS_SET_RPC_DEBUG, sizeof(int), (Address) &newValue);
	newValue = 0;
	Fs_Command(FS_SET_RPC_TRACING, sizeof(int), (Address) &newValue);
	if (!histogram) {
	    newValue = 0;
	    (void) Fs_Command(FS_SET_RPC_SERVER_HIST, sizeof(int),
				(Address) &newValue);
	    newValue = 0;
	    (void) Fs_Command(FS_SET_RPC_CLIENT_HIST, sizeof(int),
				(Address) &newValue);
	}
    }
    if (flushCache) {
	int numLockedBlocks = 0;
	Fs_Command(FS_EMPTY_CACHE, sizeof(int), (Address) &numLockedBlocks);
        if (numLockedBlocks > 0) {
            fprintf(stderr, "Flush found %d locked blocks left\n",
                                      numLockedBlocks);
        }
    }
    outStream = fopen(outFile, "w+");
    if (outStream == NULL) {
	fprintf(stderr, "\"%s\": ", outFile);
	Stat_PrintMsg(status, "Can't open");
	exit(status);
    }
    /*
     * Copy command line to output file.
     */
    fprintf(outStream, "%s ", argv[0]);
    if (clean) {
	fprintf(outStream, "-x ");
    }
    if (histogram) {
	fprintf(outStream, "-h ");
    }
    if (flushCache) {
	fprintf(outStream, "-f ");
    }
    if (clean) {
	fprintf(outStream, "-x ");
    }
    for (i=1 ; i<argc ; i++) {
	fprintf(outStream, "%s ", argv[i]);
    }
    fprintf(outStream, "\n");
    if (useSignals) {
	if (numClients > 0) {
	    int		bytesWritten;
	    Sig_Action	newAction, oldAction;
	    int		Handler();

	    newAction.action = SIG_HANDLE_ACTION;
	    newAction.handler = Handler;
	    newAction.sigHoldMask = 0;
	    Sig_SetAction(30, &newAction, &oldAction);
	    /*
	     * Get our PID and store it in a file.
	     */
	    Proc_GetIDs(&mastPID, NULL, NULL, NULL);
	    pidFD = open("/tmp/db.pid", 
				O_WRONLY | O_CREAT | O_TRUNC,
				0666, &pidFD);
	    if (pidFD == NULL) {
		fprintf(stderr,
			"Master: Couldn't open pid file, status <%x>\n",
			errno);
		exit(errno);
	    }
	    bytesWritten = write(pidFD, &mastPID, sizeof(int));
	    while (!gotSig) {
		Sig_Pause(0);
	    }
	    gotSig = FALSE;
	    printf("Got start signal\n");
	    fflush(stdout);
	}
	if (slave) {
	    int	bytesRead;
	    /*
	     * Read the master's pid out of the pid file and send him a 
	     * signal.
	     */
	    do {
		pidFD = open("/tmp/db.pid", O_RDONLY, 0);
		if (pidFD < 0) {
		    fprintf(stderr,
	    "Slave: Couldn't open pid file, status <%x>, pausing 5 seconds\n",
			errno);
		    Sync_WaitTime(5, 0);
		    continue;
		}
		mastPID = 0;
		bytesRead = read(pidFD, &mastPID, sizeof(int));
		status = Sig_Send(30, mastPID, FALSE);
		if (status == SUCCESS) {
		    /*
		     * Pause for one second to let the master receive
		     * our signal and gather stats.
		     */
		    Sync_WaitTime(1, 0);
		    break;
		}
		fprintf(stderr,
	    "Slave: couldn't signal parent, status <%x>, pausing 5 seconds\n",
		    status);
		close(pidFD);
		Sync_WaitTime(5, 0);
		continue;
	    } while (TRUE);
	}
    } else {
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
    }
    /*
     * Get first sample of filesystem stats, vm stats, time, and idle ticks.
     */
    status = Fs_Command(FS_RETURN_STATS, sizeof(Fs_Stats),
			(Address) &fsStartStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error getting FS stats");
	exit(status);
    }
    status = Sys_Stats(SYS_VM_STATS, 0, (Address) &vmStartStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error getting VM stats");
	exit(status);
    }
    status = Sys_Stats(SYS_DISK_STATS, 10, diskStartStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error getting Disk stats");
	exit(status);
    }
    /*
     * Clear low and high water marks for the file system cache.
     */
    status = Vm_Cmd(VM_RESET_FS_STATS, 0);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error resetting fs low and high water marks\n");
    }
    status = Sys_Stats(SYS_NET_ETHER_STATS, 0, (Address) &netStartStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error getting NET stats");
	exit(status);
    }
    /*
     * Snapshot the process table so we can apportion CPU usage
     * to various processes.
     */
    numPCB1 = GetProcTable(NUM_PCBS, pcbs1, argStrings1);

    status = Sys_GetTimeOfDay(&startTime, NULL, NULL);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_GetTimeOfDay");
	exit(status);
    }
    status = Sys_Stats(SYS_SCHED_STATS, 0, (Address) &startSchedStats);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in Sys_Stats");
	exit(status);
    }
#ifdef spur
    status = InitMachStats( (Address) &startMachStats, MODE_PERF_COUNTER_OFF);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Error in InitMachStats");
        exit(status);
    }
#endif
    if (slave || numClients < 0) {
	status = Proc_Fork(FALSE, &child);
	if (status == PROC_CHILD_PROC) {
	    /*
	     * Put ourselves in our own family.
	     */
	    (void) Proc_SetFamilyID(PROC_MY_PID, child);
	    (void) Ioc_SetOwner (0, child, IOC_OWNER_FAMILY);
	    /*
	     * Exec the program to benchmark.  Opt_Parse has left the command
	     * starting at argv[1], hence the following argv++.
	     */
	    argv++;
	    status = Proc_Exec(argv[0], argv, FALSE);
	    if (status != SUCCESS) {
		fprintf(stderr,"Exec of \"%s\" failed",argv[0]);
		Stat_PrintMsg(status, "");
		fflush(stdout);
		exit(status);
	    }
	} else if (status == SUCCESS) {
	    /*
	     * Wait for the benchmark to complete.
	     */
	    status = Proc_Wait(0, NULL, TRUE, NULL,
			      NULL, NULL, NULL, &usage);
	    if (status != SUCCESS) {
		Stat_PrintMsg(status, "Error in Proc_Wait");
		exit(status);
	    }
	    /*
	     * Take ending statistics and print user, system, and elapsed times.
	     */
#ifdef spur
	    status = GetMachStats( (Address) &endMachStats);
	    if (status != SUCCESS) {
		Stat_PrintMsg(status, "Error in GetMachStats");
		exit(status);
	    };
#endif
	    Sys_GetTimeOfDay(&endTime, NULL, NULL);
	    Sys_Stats(SYS_SCHED_STATS, 0, (Address) &endSchedStats);
	    Time_Subtract(endTime, startTime, &endTime);
	    PrintTimes(stderr, &usage, &endTime);
	    if (!dontSyncCache) {
		Sys_Shutdown(SYS_WRITE_BACK, "");	/* sync cache */
	    }
	    if (slave) {
		if (useSignals) {
		    status = Sig_Send(30, mastPID, FALSE);
		    if (status != SUCCESS) {
			fprintf(stderr,
			    "Slave: couldn't signal parent (2), status <%x>\n",
			    status);
		    }
		} else {
		    ClientDone(clientData);
		}
	    }
	    numPCB2 = GetProcTable(NUM_PCBS, pcbs2, argStrings2);
	    Fs_Command(FS_RETURN_STATS, sizeof(Fs_Stats), (Address) &fsEndStats);
	    Sys_Stats(SYS_VM_STATS, 0, (Address) &vmEndStats);
	    Sys_Stats(SYS_DISK_STATS, 10, diskEndStats);
	    Sys_Stats(SYS_NET_ETHER_STATS, 0, (Address) &netEndStats);
	    /*
	     * Print FS statistics.
	     */
	    PrintTimes(outStream, &usage, &endTime);
	    PrintIdleTime(outStream, &startSchedStats, &endSchedStats, &endTime);
	    PrintFsStats(outStream, &fsStartStats, &fsEndStats, TRUE);
	    PrintDiskStats(outStream, diskStartStats, diskEndStats);
	    /*
	     * Print VM statistics.
	     */
	    PrintVmStats(outStream, &vmStartStats, &vmEndStats);
	    /*
	     * Print network statistics.
	     */
	    fprintf(outStream,
			"Network stats: Bytes received %d bytes sent %d\n",
			netEndStats.bytesReceived - netStartStats.bytesReceived,
			netEndStats.bytesSent - netStartStats.bytesSent);
	    /*
	     * Print out process info.
	     */
	    PrintProcStats(outStream, numPCB1, pcbs1, argStrings1,
				      numPCB2, pcbs2, argStrings2);
#ifdef spur
	    /*
	     * Print Machine dependent stats
	     */
	    PrintMachStats(outStream, &startMachStats, &endMachStats);
#endif
	} else {
	    Stat_PrintMsg(status, "Error in Proc_Fork");
	}
    } else {
	if (useSignals) {
	    while (!gotSig) { 
		Sig_Pause(0);
	    }
	    printf("Got end signal\n");
	    fflush(stdout);
	} else {
	    Serve(serverData);
	}
	if (pause) {
	    sleep(pause);
	}
#ifdef spur
	/*
	 * Take ending statistics and print user, system, and elapsed times.
	 */
	status = GetMachStats( (Address) &endMachStats);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "Error in GetMachStats");
	    exit(status);
	};
#endif
	Sys_GetTimeOfDay(&endTime, NULL, NULL);
	Sys_Stats(SYS_SCHED_STATS, 0, (Address) &endSchedStats);
	Time_Subtract(endTime, startTime, &endTime);
	PrintTimes(stderr, &usage, &endTime);

	if (!dontSyncCache) {
	    Sys_Shutdown(SYS_WRITE_BACK, "");	/* sync cache */
	}
	Fs_Command(FS_RETURN_STATS, sizeof(Fs_Stats), (Address) &fsEndStats);
	Sys_Stats(SYS_VM_STATS, 0, (Address) &vmEndStats);
	Sys_Stats(SYS_DISK_STATS, 10, diskEndStats);
	Sys_Stats(SYS_NET_ETHER_STATS, 0, (Address) &netEndStats);
	/*
	 * Print FS statistics.
	 */
	PrintTimes(outStream, &usage, &endTime);
	PrintIdleTime(outStream, &startSchedStats, &endSchedStats, &endTime);
	PrintFsStats(outStream, &fsStartStats, &fsEndStats, TRUE);
	PrintDiskStats(outStream, diskStartStats, diskEndStats);
	/*
	 * Print VM statitistics.
	 */
	PrintVmStats(outStream, &vmStartStats, &vmEndStats);
	/*
	 * Print network statistics.
	 */
	fprintf(outStream,
		    "Network stats: Bytes received %d bytes sent %d\n",
		    netEndStats.bytesReceived - netStartStats.bytesReceived,
		    netEndStats.bytesSent - netStartStats.bytesSent);
#ifdef spur
	/*
	** Print Machine dependent stats
	*/
	PrintMachStats(outStream, &startMachStats, &endMachStats);
#endif
    }
    exit(status);
}

int
Handler()
{
    gotSig = TRUE;
}

int
HandleUSR1()
{
    startClients = TRUE;
}
