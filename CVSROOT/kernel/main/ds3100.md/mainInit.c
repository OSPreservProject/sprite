/* 
 *  main.c --
 *
 *	The main program for Sprite: initializes modules and creates
 *	system processes. Also creates a process to run the Init program.
 *
 * Copyright 1984 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif /* !lint */

#include "sprite.h"
#include "dbg.h"
#include "dev.h"
#include "net.h"
#include "proc.h"
#include "prof.h"
#include "fsutil.h"
#include "recov.h"
#include "rpc.h"
#include "sched.h"
#include "sig.h"
#include "sync.h"
#include "sys.h"
#include "timer.h"
#include "vm.h"
#include "machMon.h"
#include "mach.h"
#include "fs.h"
#include "main.h"
#include "stdio.h"

static void Init _ARGS_((void));

/*
 *  Pathname of the Init program.
 */
#define INIT	 	"cmds/initsprite"

int main_PrintInitRoutines = FALSE;/* print out each routine as it's called? */

int main_PanicOK = 0;	/* Set to 1 if it's OK to panic. */

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	All kernel modules are initialized by calling their *_Init()
 *	routines. In addition, kernel processes are created to
 *	handle virtual memory and rpc-specific stuff. The last process
 *	created runs the `init' program.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The whole system is initialized.
 *
 *----------------------------------------------------------------------
 */

void
main(argc,argv)
int argc;
MachStringTable *argv;
{
    Proc_PID	pid;
    int		i;

    /*
     * Initialize variables specific to a given kernel.  
     * IMPORTANT: Only variable assignments and nothing else can be
     *		  done in this routine.
     */
    Main_InitVars();

    /*
     * Initialize machine dependent info.  MUST BE CALLED HERE!!!.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Mach_Init().\n");
    }
    Mach_Init(argc, argv);
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sync_Init().\n");
    }
    Sync_Init();

    /*
     * Initialize the debugger.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Dbg_Init().\n");
    }
    Dbg_Init();

    /*
     * Initialize the system module, particularly the fact that there is an
     * implicit DISABLE_INTR on every processor.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sys_Init().\n");
    }
    Sys_Init();

    /*
     * Now allow memory to be allocated by the "Vm_BootAlloc" call.  Memory
     * can be allocated by this method until "Vm_Init" is called.  After this
     * then the normal memory allocator must be used.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Vm_BootInit().\n");
    }
    Vm_BootInit();

    /*
     * Initialize the ethernet drivers.  Do it early because we need
     * the ethernet driver so that we can use the debugger.
     *
     * Dependencies: VmBoot_Init
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Net_Init\n");
    }
    Net_Init();

/*
    mach_DebugState.regs[SP] = 0x80030000; 

    Dbg_Main();
 */

    /*
     * Initialize all devices.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Dev_Init().\n");
    }
    Dev_Init();

    /*
     *  Initialize the mappings of keys to call dump routines.
     *  Must be after Dev_Init. 
     */
    if (main_DoDumpInit) {
	if (main_PrintInitRoutines) {
	    Mach_MonPrintf("Calling Dump_Init().\n");
	}
	Dump_Init();
    }

    /*
     * Initialize the timer, signal, process, scheduling and synchronization
     * modules' data structures.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_Init().\n");
    }
    Proc_Init();
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sync_LockStatInit().\n");
    }
    Sync_LockStatInit();
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Timer_Init().\n");
    }
    Timer_Init();
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sig_Init().\n");
    }
    Sig_Init();
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sched_Init().\n");
    }
    Sched_Init();

    /*
     * Sys_Printfs are not allowed before this point.
     */  
    main_PanicOK++;
    printf("Sprite kernel: %s\n", SpriteVersion());

    /*
     * Set up bins for the memory allocator.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Fs_Bin\n");
    }
    Fs_Bin();
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Net_Bin\n");
    }
    Net_Bin();

    /*
     * Initialize virtual memory.  After this point must use the normal
     * memory allocator to allocate memory.  If you use Vm_BootAlloc then
     * will get a panic into the debugger.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Vm_Init\n");
    }
    Vm_Init();

    /*
     * malloc can be called from this point on.
     */

    /*
     * Initialize the main process. Must be called before any new 
     * processes are created.
     * Dependencies: Proc_InitTable, Sched_Init, Vm_Init, Mem_Init
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_InitMainProc\n");
    }
    Proc_InitMainProc();

    /*
     * Initialize the routes.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Net_RouteInit\n");
    }
    Net_RouteInit();

    /*
     * Enable server process manager.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_ServerInit\n");
    }
    Proc_ServerInit();

    /*
     * Initialize the recovery module.  Do before Rpc and after Vm_Init.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Recov_Init\n");
    }
    Recov_Init();

    /*
     * Initialize the data structures for the Rpc system.  This uses
     * Vm_RawAlloc to so it must be called after Vm_Init.
     * Dependencies: Timer_Init, Vm_Init, Net_Init, Recov_Init
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Rpc_Init\n");
    }
    Rpc_Init();

    /*
     * Configure devices that may or may not exist.  This needs to be
     * done after Proc_InitMainProc because the initialization routines
     * use SetJump which uses the proc table entry for the main process.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Dev_Config\n");
    }
    Dev_Config();

    /*
     * Initialize profiling after the timer and vm stuff is set up.
     * Dependencies: Timer_Init, Vm_Init
     */
    if (main_DoProf) {
	Prof_Init();
    }
    /*
     *  Allow interrupts from now on.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Enabling interrupts\n");
    }
    ENABLE_INTR();

    if (main_Debug) {
	DBG_CALL;
    }

    /*
     * Sleep for a few seconds to calibrate the idle time ticks.
     */
    Sched_TimeTicks();

    /*
     * Start profiling, if desired.
     */
    if (main_DoProf) {
        (void) Prof_Start();
    }

    /*
     * Do an initial RPC to get a boot timestamp.  This allows
     * servers to detect when we crash and reboot.  This will set the
     * system clock too, although rdate is usually done from user level later.
     */
    if (main_PrintInitRoutines) {
	printf("Call Rpc_Start\n");
    }
    Rpc_Start();

    /*
     * Initialize the file system. 
     */
    if (main_PrintInitRoutines) {
	printf("Call Fs_Init\n");
    }
    Fs_Init();

    /*
     * Before starting up any more processes get a current directory
     * for the main process.  Subsequent new procs will inherit it.
     */
    if (main_PrintInitRoutines) {
	printf("Call Fs_ProcInit\n");
    }
    Fs_ProcInit();

#ifdef TESTING
    device.unit = 0;
    Dev_ConsoleOpen(&device, FS_READ, NIL);
    while (1) {
	char		buf[11];
	int		len;
	Time		time;

	time.seconds = 1;
	time.microseconds = 0;

	if (Dev_ConsoleRead(&device, 0, 10, buf, &len) == SUCCESS) {
	    printf("%s", buf);
	}
	Sync_WaitTime(time);
    }
#endif

    if (main_PrintInitRoutines) {
	printf("Bunch of call funcs\n");
    }
    /*
     * Start the clock daemon and the routine that opens up the swap directory.
     */
    Proc_CallFunc(Vm_Clock, (ClientData) NIL, 0);
    Proc_CallFunc(Vm_OpenSwapDirectory, (ClientData) NIL, 0);

    /*
     * Start the process that synchronizes the filesystem caches
     * with the data kept on disk.
     */
    Proc_CallFunc(Fsutil_SyncProc, (ClientData) NIL, 0);

    /*
     * Create a few RPC server processes and the Rpc_Daemon process which
     * will create more server processes if needed.
     */
    if (main_NumRpcServers > 0) {
	for (i=0 ; i<main_NumRpcServers ; i++) {
	    (void) Rpc_CreateServer((int *) &pid);
	}
    }
    (void) Proc_NewProc((Address)(unsigned)(int (*)())Rpc_Daemon, 
			PROC_KERNEL, FALSE, &pid, "Rpc_Daemon");
    if (main_PrintInitRoutines) {
	printf("Creating Proc server procs\n");
    }

    /*
     * Create processes  to execute functions.
     */
    for (i = 0; i < proc_NumServers; i++) {
	(void) Proc_NewProc((Address)(unsigned)(int (*)()) Proc_ServerProc,
			    PROC_KERNEL, FALSE,	&pid, "Proc_ServerProc");
    }

    /*
     * Create a recovery process to monitor other hosts.  Can't use
     * Proc_CallFunc's to do this because they can be used up waiting
     * for page faults against down servers.  (Alternatively the VM
     * code could be fixed up to retry page faults later instead of
     * letting the Proc_ServerProc wait for recovery.)
     */
    (void) Proc_NewProc((Address) Recov_Proc, PROC_KERNEL, FALSE, &pid,
			"Recov_Proc");

    /*
     * Set up process migration recovery management.
     */
    if (main_PrintInitRoutines) {
	printf("Calling Proc_MigInit\n");
    }
    Proc_MigInit();

    /*
     * Call the routine to start test kernel processes.
     */

    if (main_PrintInitRoutines) {
	printf("Calling Main_HookRoutine\n");
    }
    Main_HookRoutine();

    /*
     * Print out the amount of memory used.
     */
    printf("MEMORY %d bytes allocated for kernel\n", 
		vmMemEnd - mach_KernStart);

    /*
     * Start up the first user process.
     */
    if (main_PrintInitRoutines) {
	printf("Creating Init\n");
    }
    (void) Proc_NewProc((Address)(unsigned)(int (*)())Init, PROC_KERNEL,
			FALSE, &pid, "Init");

    (void) Sync_WaitTime(time_OneYear);
    printf("Main exiting\n");
    Proc_Exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * Init --
 *
 *	This routine execs the init program.
 *
 * Results:
 *	This routine only returns an error if the exec failed.
 *
 * Side effects:
 *	The current process image is overlayed by the init process.
 *
 *----------------------------------------------------------------------
 */
static void
Init()
{
    char		*initArgs[10];
    ReturnStatus	status;
    char		argBuffer[256];
    int			argc;
    Fs_Stream		*dummy;
    char		bootCommand[103];
    char		*ptr;
    int			i;
    int			argLength;

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("In Init\n");
    }
    bzero(bootCommand, 103);
    argc = Mach_GetBootArgs(8, 256, &(initArgs[2]), argBuffer);
    if (argc>0 && !strcmp(initArgs[argc+1],"-a")) {
	argc--;
    }
    if (argc>0) {
	argLength = (((int) initArgs[argc+1]) + strlen(initArgs[argc+1]) +
			1 - ((int) argBuffer));
    } else {
	argLength = 0;
    }
    if (argLength > 0) {
	initArgs[1] = "-b";
	ptr = bootCommand;
	for (i = 0; i < argLength; i++) {
	    if (argBuffer[i] == '\0') {
		*ptr++ = ' ';
	    } else {
		*ptr++ = argBuffer[i];
	    }
	}
	bootCommand[argLength] = '\0';
	initArgs[2] = bootCommand;
	initArgs[argc + 2] = (char *) NIL;
    } else {
	initArgs[1] = (char *) NIL;
    }
    if (main_AltInit != 0) {
	initArgs[0] = main_AltInit;
	printf("Execing \"%s\"\n", initArgs[0]);
	status = Proc_KernExec(initArgs[0], initArgs);
	printf( "Init: Could not exec %s status %x.\n",
			initArgs[0], status);
    }
    status = Fs_Open(INIT,FS_EXECUTE | FS_FOLLOW, FS_FILE, 0, &dummy);
    if (status != SUCCESS) {
	printf("Can't open %s <0x%x>\n", INIT,status);
    }
    initArgs[0] = INIT;
    status = Proc_KernExec(initArgs[0], initArgs);
    printf( "Init: Could not exec %s status %x.\n",
			initArgs[0], status);
    Proc_Exit(1);
}

TestEther()
{
    extern Boolean	dbg_UsingNetwork;

    dbg_UsingNetwork = TRUE;

    while (1) {
	Net_RecvPoll();
    }
}
