/* 
 * main.c --
 *
 *	Driver for the pdev-based migration/load-average daemon.
 *	Based on the CS 262 project by Thorsten von Eicken and Andreas
 *	Stolcke, spring 1989.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/daemons/migd/RCS/main.c,v 1.2 92/04/29 22:26:40 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fs.h>
#include <option.h>
#include <syslog.h>
#include <status.h>
#include <sysStats.h>
#include <sys/file.h>
#include <sys/types.h>
#include <host.h>
#include "migd.h"
#include "migPdev.h"
#include "global.h"



int debug = 0;

Option optionArray[] = {
	{OPT_INT, "D", (char *)&debug, "Set debugging level."},
	{OPT_FALSE, "s", (char *)&migd_Verbose,
		 "Disable extra information messages."},
	{OPT_TRUE, "L", (char *)&migd_LogToFiles,
		 "Log errors to files rather than to inherited stderr."},
	{OPT_TRUE, "F", (char *)&migd_DontFork,
		 "Don't fork when starting or when creating a global daemon."},
	{OPT_FALSE, "S", (char *)&migd_DoStats,
		 "Don't gather statistics."},
	{OPT_TRUE, "e", (char *)&migd_NeverEvict,
	     "Never evict foreign processes."},
	{OPT_TRUE, "G", (char *)&migd_NeverRunGlobal,
	     "Never run the global daemon on this host."},
	{OPT_INT, "C", (char *)&global_CheckpointInterval,
	     "Interval for checkpointing state."},
};
static int numOptions = sizeof(optionArray) / sizeof(Option);

int	migd_HostID = 0;	                  /* My host ID. */
char 	*migd_HostName;   			  /* My host name. */
static char hostNameBuffer[FS_MAX_NAME_LENGTH];   /* Buffer for host name. */

#ifndef SHARED_ERROR_NAME
#define SHARED_ERROR_NAME "/sprite/admin/migd/global-log"
#endif

#ifndef LOCAL_ERROR_NAME
#define LOCAL_ERROR_NAME "/sprite/admin/migd/%s.log"
#endif

char *migd_GlobalPdevName; 			/* Global pdev name,
						   initialized at runtime. */
char *migd_GlobalErrorName = SHARED_ERROR_NAME; /* Global error log. */
char *migd_LocalPdevName;			/* Host-specific pdev name. */

char *migd_ProgName;				/* Name invoked by, for
						   msgs. */
int  migd_Pid;					/* Our process ID, for msgs. */
int  migd_Version = MIG_STATS_VERSION;		/* Our version number. */

static int 	PrintMemStats();


/*
 *----------------------------------------------------------------------
 *
 * Debug1 --
 *
 *	For debugging: turns debug variables from non-zero to zero or
 * 	from zero to 1.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See above.
 *
 *----------------------------------------------------------------------
 */

static int
Debug1()
{
    migPdev_Debug = !migPdev_Debug;
    migd_Debug = !migd_Debug;
    global_Debug = !global_Debug;
}


/*
 *----------------------------------------------------------------------
 *
 * TheEnd --
 *
 *	We've been ordered to quit.  If this is
 * 	the first time, try to shut down in an orderly fashion.
 *	If not, just give up.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets migd_Quit so contacts will receive error notifications.
 *
 *----------------------------------------------------------------------
 */

static int
TheEnd(sigNum)
    int sigNum;
{
    DATE();
    if (migd_Quit) {
	fprintf(stderr, "Multiple signals received: exiting.\n");
	exit(1);
    }
    fprintf(stderr, "Signal %d received: aborting...\n", sigNum);
    if (migd_GlobalMaster) {
	migd_Quit = 1;
	Global_Quit();
    } else {
	exit(1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * FatalError --
 *
 *	Record a message in the syslog about a fatal error, then exit.
 *	Try not to take down the other daemons with us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes process to terminate.
 *
 *----------------------------------------------------------------------
 */

static int
FatalError(sigNum)
    int sigNum;			/* Signal being handled. */
{
    static int gotError = 0;

    if (gotError) {
	/*
	 * Recursive errors?  Give it up!
	 */
	_exit(1);
    }
    gotError = 1;
    if (sigNum == SIGTERM) {
	SYSLOG0(LOG_ERR,
		"terminated by order of global daemon... should be restarted soon.\n");
	if (migd_LogToFiles) {
	    fprintf(stderr,
		    "terminated by order of global daemon.\n");
	}
    } else {	
	SYSLOG1(LOG_ERR, "Received fatal signal %d: exiting.\n", sigNum);
	if (migd_LogToFiles) {
	    fprintf(stderr, "Received fatal signal %d: exiting.\n", sigNum);
	}
    }
    if (migd_GlobalMaster) {
	migd_Quit = 1;
	Global_End();
    } else {
	exit(1);
    }
}

#ifdef MEMTRACE
/*
 *----------------------------------------------------------------------
 *
 * PrintMemStats --
 *
 *	Prints a summary of the memory allocator statistics.
 *	Can be called as a signal handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Statistics are printed on the standard error stream.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
PrintMemStats(sigNum, sigCode)
    int		sigNum;		/* Ignored. */
    int		sigCode;	/* Ignored. */
{
    Mem_PrintStats();
    Mem_PrintInUse();
    Mem_DumpTrace(2104);
}
#endif /* MEMTRACE */


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
 *	None.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
int argc;
char *argv[];
{
    int status;
    Host_Entry *hostPtr;
    char buffer[FS_MAX_NAME_LENGTH];	/* For file names. */
#ifdef MEMTRACE
    Mem_TraceInfo traceInfo;
#endif  

    argc = Opt_Parse(argc, argv, optionArray, numOptions,
		     OPT_ALLOW_CLUSTERING);

#ifdef MEMTRACE
    Mem_SetPrintProc(fprintf, (ClientData)stderr);
    traceInfo.size=2104;
    traceInfo.flags=MEM_PRINT_TRACE|MEM_STORE_TRACE;
    Mem_SetTraceSizes(1, &traceInfo);
#endif    
    

    /*
     * "Fatal" signals should exit without signalling clients.
     * USR1 toggles debugging.  USR2 should clean up and
     * kill of the per-host daemons too.  
     * Other signals should be ignored completely.  
     */
#ifdef MEMTRACE
    signal(SIGUSR1, PrintMemStats);
#else
    signal(SIGUSR1, Debug1);
#endif
    signal(SIGUSR2, TheEnd);
#ifndef DEBUG
    signal(SIGTERM, FatalError);
#ifndef DEBUG_LIST_REMOVE
    signal(SIGQUIT, FatalError);
    signal(SIGABRT, FatalError);
#endif
    signal(SIGILL,  FatalError);
    signal(SIGFPE,  FatalError);
#endif /* DEBUG */ 
    signal(SIGPIPE, SIG_IGN);
    if (debug <= 2) {
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
    }

    migd_ProgName = rindex(argv[0], '/');
    if (migd_ProgName){
	migd_ProgName++;
    } else {
	migd_ProgName = argv[0];
    }
    openlog(migd_ProgName, LOG_PID, LOG_DAEMON);

    /*
     * Get our hostID and hostname.  Use the physical host just in case
     * someone wants to run us from another host.
     */

    status = Proc_GetHostIDs((int *) NULL, &migd_HostID);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "Proc_GetHostIDs");
	exit(Compat_MapCode(status));
    }

    hostPtr = Host_ByID(migd_HostID);
    if (hostPtr == (Host_Entry *) NULL) {
	Host_End();
	fprintf(stderr, "%s: unable to get host information for this host.\n",
		migd_ProgName);
	exit(1);
    }
    migd_HostName = hostNameBuffer;
    (void) strcpy(migd_HostName, hostPtr->name);

    migd_GlobalPdevName = Mig_GetPdevName(1);
    migd_LocalPdevName = Mig_GetPdevName(0);
    
    if (migd_LogToFiles) {
	(void) sprintf(buffer, LOCAL_ERROR_NAME, migd_HostName);
	if (freopen(buffer, "a", stderr) == NULL) {
	    syslog(LOG_ERR, "couldn't redirect stderr.\n");
	    exit(1);
	}
#ifdef SEEK_REOPEN	
	fseek(stderr, 0L, L_XTND);
#endif /* SEEK_REOPEN */
	if (fcntl(fileno(stderr), F_SETFL, FAPPEND) < 0) {
	    perror("fcntl");
	}
    }
    
    

    /*
     * Fork off someone to do the real work.
     */

    if (!migd_DontFork) {
	int pid = fork();

	if (pid < 0) {
	    perror("migd");
	    exit(1);
	}
	else if (pid > 0) {
	    exit(0);
	}
    }

    migd_Pid = getpid();
	
    if (debug) {
	extern char *Version();
	int t;

	setlinebuf(stderr);
	fprintf(stderr,
		"********************************************************\n");
	fprintf(stderr, "%s: pid %x version %d, %s.\n", migd_ProgName,
		migd_Pid, migd_Version, Version());
	t = time((time_t *)0);
	fprintf(stderr, "%s: run at %s", migd_ProgName, ctime(&t));
	fprintf(stderr,
		"- - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");

	migPdev_Debug = debug;
	migd_Debug = debug;
	global_Debug = debug;
    }

    /*
     * Get our migration version.
     */
    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_VERSION,
		       (Address) &migd_Parms.version);
    if (status != SUCCESS) {
	SYSLOG1(LOG_ERR, "Can't get migration version number: %s\n",
	       Stat_GetMsg(status));
	exit(Compat_MapCode(status));
    }

    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATE,
		       (Address) &migd_Parms.criteria);
    if (status != SUCCESS) {
	SYSLOG1(LOG_ERR, "Error in Sys_Stats getting migration state: %s.\n",
		Stat_GetMsg(status));
	exit(Compat_MapCode(status));
    }


    /*
     * MigPdev_Init must be called before Migd_Init.
     */
    MigPdev_Init();

    if(Migd_Init()) {
	exit(1);
    }

    
    Migd_HandleRequests();
    Migd_End();
    DATE();
    exit(0);
}




