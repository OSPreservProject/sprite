/* 
 * mainInit.c --
 *
 *	Startup code for Sprite server.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/main/RCS/mainInit.c,v 1.21 92/04/23 23:50:20 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <cthreads.h>
#include <status.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dev.h>
#include <fsutil.h>
#include <main.h>
#include <net.h>
#include <proc.h>
#include <user/proc.h>
#include <recov.h>
#include <rpc.h>
#include <sig.h>
#include <sync.h>
#include <sys.h>
#include <user/sys.h>
#include <timer.h>
#include <vm.h>
#include <vmSwapDir.h>

extern char *Version();

/* 
 * This is defined in the Mach libc.h, but there are too many type clashes 
 * between libc.h and Sprite header files.
 */
extern int getopt _ARGS_((int, const char **, const char *));
#ifndef EOF
#define EOF	(-1)
#endif


#define DEFAULT_INIT	"cmds.sprited/initsprite" /* default initial user 
						   * program */ 
char *main_InitPath;		/* path to the first user program */

/* 
 * This holds the array of arguments that are passed to the init program.
 */
char *main_InitArgArray[MAIN_MAX_INIT_ARGS + 1];

Boolean main_DebugFlag = FALSE; /* enables various debug checks */

/* 
 * This flag lets some low-level routines avoid locks, getting the current 
 * process handle, etc. during initialization.  If the flag is false, code 
 * can assume there is only one thread running.  Otherwise, code should 
 * assume that there are multiple threads running.
 */
Boolean main_MultiThreaded = FALSE;


/* 
 * Forward references
 */

static void CheckArguments _ARGS_((int argc, char *argv[]));
static void StartInit _ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Startup and server loop.
 *	XXX - should eventually take Sprite main (e.g., call Main_InitVars).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    Proc_PID	pid;
    int		i;
    ReturnStatus status;

    CheckArguments(argc, argv);

    Sync_Init();
    Proc_Init();

    Dev_Init();

    /* 
     * This might be able to go before Dev_Init, but it should probably 
     * come after Sync_Init and Proc_Init.
     */
    printf("Sprite server: %s\n", Version());

    Vm_Init();

    Sys_Init();

    Sig_Init();

    /*
     * Initialize the main process. Must be called before any new 
     * processes are created.
     * Dependencies: Proc_Init, Sync_Init (for malloc, from cthread_init), 
     * Sig_Init (to get the default signal actions).
     */
    Proc_InitMainProc();
    main_MultiThreaded = TRUE;

    Timer_Init();

    /*
     * Initialize the network and the routes.  Creates new processes, so 
     * must come after Proc_InitMainProc.
     */
    Net_Init();
    Net_RouteInit();

    /*
     * Enable server process manager.
     */
    Proc_ServerInit();

    /*
     * Initialize the recovery module.  Do before Rpc and after Vm_Init.
     */
#ifndef NO_RECOVERY
    Recov_Init();
#endif

    /*
     * Initialize the data structures for the Rpc system.
     * Dependencies: Timer_Init, Net_Init, Recov_Init, Sys_Init (to get 
     * current machine type).
     */
    Rpc_Init();

    /*
     * Do an initial RPC to get a boot timestamp.  This allows
     * servers to detect when we crash and reboot.  This will set the
     * system clock too, although rdate is usually done from user level later.
     */
    Rpc_Start();

    /*
     * Initialize the file system. 
     */
    Fs_Init();

    /*
     * Before starting up any more processes get a current directory
     * for the main process.  Subsequent new procs will inherit it.
     */
    Fs_ProcInit();
    /*
     * Start the routine that opens up the swap directory.
     */
    Proc_CallFunc(Vm_OpenSwapDirectory, (ClientData) NIL, time_ZeroSeconds);

    /*
     * Start the process that synchronizes the filesystem caches
     * with the data kept on disk.
     */
    Proc_CallFunc(Fsutil_SyncProc, (ClientData) NIL, time_ZeroSeconds);

    /*
     * Create a few RPC server processes and the Rpc_Daemon process which
     * will create more server processes if needed.
     */
    if (main_NumRpcServers > 0) {
	for (i=0 ; i<main_NumRpcServers ; i++) {
	    (void) Rpc_CreateServer((int *) &pid);
	}
    }
    (void) Proc_NewProc((Address) Rpc_Daemon, (Address)0, PROC_KERNEL,
			FALSE, &pid, "Rpc_Daemon");

    /*
     * Create processes  to execute functions.
     */
    for (i = 0; i < proc_NumServers; i++) {
	(void) Proc_NewProc((Address)Proc_ServerProc, (Address)0,
			    PROC_KERNEL, FALSE, &pid, "Proc_ServerProc");
    }

    /*
     * Create a recovery process to monitor other hosts.  Can't use
     * Proc_CallFunc's to do this because they can be used up servicing 
     * memory requests against down servers.
     */
#ifndef NO_RECOVERY
    (void) Proc_NewProc((Address) Recov_Proc, (Address)0, PROC_KERNEL,
			FALSE, &pid, "Recov_Proc");
#endif

    /* 
     * Start the first user process.  Do it in a separate process so that 
     * we can service any VM requests that might get generated.  Fork off a 
     * new process, rather than using a Proc_ServerProc, because 
     * Proc_ServerProcs don't have FS state.
     */
    status = Proc_NewProc((Address) UTILSMACH_MAGIC_CAST StartInit, 
			  (Address)0, PROC_KERNEL, FALSE, &pid,
			  "Start Init"); 
    if (status != SUCCESS) {
	panic("Can't start up thread to start init: %s\n",
	      Stat_GetMsg(status));
    }

    Sys_ServerLoop();
    return 0;			/* lint */
}


/*
 *----------------------------------------------------------------------
 *
 * CheckArguments --
 *
 *	Set flags according to the arguments the user gave.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May set main_DebugFlag.  Sets up the arguments array to pass to the 
 *	init program.
 *
 *----------------------------------------------------------------------
 */

static void
CheckArguments(argc, argv)
    int argc;
    char **argv;
{
    int argChar;		/* argument character */
    Boolean error = FALSE;
    extern int optind;		/* part of getopt package */
    extern char *optarg;	/* part of getopt package */
    int initArg;		/* index for setting up init argument array */

    main_InitPath = DEFAULT_INIT;

    if (argc == 2 && strcmp(argv[1], "-help") == 0) {
	printf("options:\n");
	printf("-d\tEnable debugging.\n");
	printf("-i init\tStart ``init'' instead of %s.\n",
	       DEFAULT_INIT);
	printf("-p\tTurn on system call profiling.\n");
	printf("-v maxPending\tSet the limit on pending requests for a\n");
	printf("\t\tsingle segment.\n");
	exit(0);
    }

    while ((argChar = getopt(argc, argv, "di:pv:")) != EOF) {
	switch (argChar) {
	case 'd':
	    main_DebugFlag = TRUE;
	    break;
	case 'i':
	    main_InitPath = optarg;
	    break;
	case 'p':
	    sys_CallProfiling = TRUE;
	    break;
	case 'v':
	    vm_MaxPendingRequests = atoi(optarg);
	    break;
	default:
	    error = TRUE;
	    break;
	}
    }

    main_InitArgArray[0] = main_InitPath;
    for (initArg = 1; optind < argc; optind++, initArg++) {
	if (initArg >= MAIN_MAX_INIT_ARGS) {
	    break;
	}
	main_InitArgArray[initArg] = argv[optind];
    }	
    main_InitArgArray[initArg] = (char *)NIL;

    if (error) {
	printf("Use -help to get a list of command-line arguments.\n");
	exit(1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * StartInit --
 *
 *	Start up the first user program.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Start up a user process running the usual initial program.  Causes 
 *	the server to exit if there was a problem.
 *
 *----------------------------------------------------------------------
 */

static void
StartInit()
{
    ReturnStatus status;

    status = Proc_NewProc((Address)0, (Address)0, PROC_USER, FALSE, 
			  (Proc_PID *)0, "Init");
    if (status != SUCCESS) {
	printf("Couldn't start %s: %s\n", main_InitPath,
	       Stat_GetMsg(status));
	Sys_Shutdown(SYS_KILL_PROCESSES | SYS_WRITE_BACK | SYS_HALT);
    }
}

