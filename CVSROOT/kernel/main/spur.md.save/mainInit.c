/* 
 *  main.c --
 *
 *	The main program for Sprite: initializes modules and creates
 *	system processes. Also creates a process to run the Init program.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "dbg.h"
#include "dev.h"
#include "mem.h"
#include "net.h"
#include "mach.h"
#include "proc.h"
#include "prof.h"
#include "rpc.h"
#include "sched.h"
#include "sig.h"
#include "sync.h"
#include "sys.h"
#include "timer.h"
#include "vm.h"
#include "machMon.h"
#include "devAddrs.h"
#include "user/sysStats.h"
#include "mach.h"

extern void Fs_WakeupProc();
extern void Fs_HandleScavenge();

static void StartSlaveProcessors();

/*
 *  Pathname of the Init program.
 */
#define INIT	 	"/initsprite.spur"

/*
 * Flags defined in individual's mainHook.c to modify the startup behavior. 
 */

extern Boolean main_Debug;	/* If TRUE then enter the debugger */
extern Boolean main_DoProf;	/* If TRUE then start profiling */
extern Boolean main_DoDumpInit;	/* If TRUE then initialize dump routines */
extern Boolean main_AllowNMI;	/* If TRUE then allow non-maskable interrupts.*/

extern int main_NumRpcServers;	/* # of rpc servers to spawn off */
extern void Main_HookRoutine();	/* routine to allow custom initialization */
extern void Main_InitVars();

int main_PrintInitRoutines = FALSE;
				/* print out each routine as it's called? */

extern	Address	vmMemEnd;	/* The end of allocated kernel memory. */

static void	Init();
static void	Init2();

int	bootProgress = 0;


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

main()
{
    int		pid;
    int		i;
    char	*SpriteVersion();

    bootProgress = 1; 
    led_display(bootProgress,0,0);
    /*
     * Initialize variables specific to a given kernel.  
     * IMPORTANT: Only variable assignments and nothing else can be
     *		  done in this routine.
     */
    Main_InitVars();

    /*
     * Initialize machine dependent info.  MUST BE CALLED HERE!!!.
     */
    bootProgress = 2;
    led_display(bootProgress,0,0);
    Mach_Init();

    bootProgress = 3;
    led_display(bootProgress,0,0);
    /*
     * Initialize the debugger.
     */
    Dbg_Init();

    /*
     * Initialize the system module, particularly the fact that there is an
     * implicit DISABLE_INTR on every processor.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sys_Init().\n");
    }
    bootProgress = 4;
    led_display(bootProgress,0,0);
    Sys_Init();

    /*
     * Now allow memory to be allocated by the "Vm_BootAlloc" call.  Memory
     * can be allocated by this method until "Vm_Init" is called.  After this
     * then the normal memory allocator must be used.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Vm_BootInit().\n");
    }
    bootProgress = 5;
    led_display(bootProgress,0,0);
    Vm_BootInit();

    /*
     * Initialize all devices.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Dev_Init().\n");
    }
    bootProgress = 6;
    led_display(bootProgress,0,0);
    Dev_Init();

    /*
     * Initialize the timer, signal, process, scheduling and synchronization
     * modules' data structures.
     */
    Proc_Init();
    Sync_LockStatInit();
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Timer_Init().\n");
    }
    bootProgress = 7;
    led_display(bootProgress,0,0);
    Timer_Init();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sig_Init().\n");
    }
    bootProgress = 8;
    led_display(bootProgress,0,0);
    Sig_Init();


    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sched_Init().\n");
    }
    bootProgress = 10;
    led_display(bootProgress,0,0);
    Sched_Init();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sync_Init().\n");
    }
    bootProgress = 11;
    led_display(bootProgress,0,0);
    Sync_Init();

    /*
     * printfs are not allowed before this point.
     */  

    printf("Sprite kernel: %s\n", SpriteVersion());

    /*
     * Set up bins for the memory allocator.
     */
    Fs_Bin();
    Net_Bin();

    /*
     * Initialize virtual memory.  After this point must use the normal
     * memory allocator to allocate memory.  If you use Vm_BootAlloc then
     * will get a panic into the debugger.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Vm_Init\n");
    }
    bootProgress = 12;
    led_display(bootProgress,0,0);
    Vm_Init();

    /*
     * Mem_Alloc can be called from this point on.
     */

    /*
     * Initialize the main process. Must be called before any new 
     * processes are created.
     * Dependencies: Proc_InitTable, Sched_Init, Vm_Init, Mem_Init
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_InitMainProc\n");
    }
    bootProgress = 13;
    led_display(bootProgress,0,0);
    Proc_InitMainProc();

    /*
     * Enable server process manager.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_ServerInit\n");
    }
    bootProgress = 14;
    led_display(bootProgress,0,0);
    Proc_ServerInit();

    /*
     * Initialize the ethernet drivers.
     * Dependencies: Vm_Init
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Net_Init\n");
    }
    bootProgress = 15;
    led_display(bootProgress,0,0);
    Net_Init();

    /*
     * Initialize the recovery module.  Do before Rpc and after Vm_Init.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Recov_Init\n");
    }
    bootProgress = 16;
    led_display(bootProgress,0,0);
    Recov_Init();

    /*
     * Initialize the data structures for the Rpc system.  This uses
     * Vm_RawAlloc to so it must be called after Vm_Init.
     * Dependencies: Timer_Init, Vm_Init, Net_Init, Recov_Init
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Rpc_Init\n");
    }
    led_display(bootProgress,0,0);
    bootProgress = 17;
    Rpc_Init();

    /*
     * Configure devices that may or may not exist.  This needs to be
     * done after Proc_InitMainProc because the initialization routines
     * use SetJump which uses the proc table entry for the main process.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Dev_Config\n");
    }
    bootProgress = 18;
    led_display(bootProgress,0,0);
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
    bootProgress =  19;
    led_display(bootProgress,0,0);
    ENABLE_INTR();
    if (main_Debug) {
	DBG_CALL;
    }
    /*
     * Sleep for a few seconds to calibrate the idle time ticks.
     */
    bootProgress =  20;
    led_display(bootProgress,0,0);
    Sched_TimeTicks();

    /*
     * Start profiling, if desired.
     */

    if (main_DoProf) {
        Prof_Start();
    }

    /*
     * Do an initial RPC to get a boot timestamp.  This allows
     * servers to detect when we crash and reboot.  This will set the
     * system clock too, although rdate is usually done from user level later.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Call Rpc_Start\n");
    }
    bootProgress = 21;
    led_display(bootProgress,0,0);
    Rpc_Start();

    /*
     * Initialize the file system. 
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Call Fs_Init\n");
    }
    bootProgress = 22;
    led_display(bootProgress,0,0);
    Fs_Init();

    /*
     * Before starting up any more processes get a current directory
     * for the main process.  Subsequent new procs will inherit it.
     */ 

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Call Fs_ProcInit\n");
    }
    bootProgress = 23;
    led_display(bootProgress,0,0);
    Fs_ProcInit();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Bunch of call funcs\n");
    }
    bootProgress = 24;
    led_display(bootProgress,0,0);
    /*
     * Start the clock daemon and the routine that opens up the swap directory.
     */

    Proc_CallFunc(Vm_Clock, (ClientData) NIL, 0);

    Proc_CallFunc(Vm_OpenSwapDirectory, (ClientData) NIL, 0);

    /*
     * Start the process that synchronizes the filesystem caches
     * with the data kept on disk.
     */
    Proc_CallFunc(Fs_SyncProc, (ClientData) NIL, 0);
    /*
     * Create a few RPC server processes and the Rpc_Daemon process which
     * will create more server processes if needed.
     */

    if (main_NumRpcServers > 0) {
	for (i=0 ; i<main_NumRpcServers ; i++) {
	    Rpc_CreateServer(&pid);
	}
    }
    Proc_NewProc((Address) Rpc_Daemon, PROC_KERNEL, FALSE, &pid, "Rpc_Daemon");

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Creating Proc server procs\n");
    }
    /*
     * Create processes  to execute functions.
     */
    for (i = 0; i < proc_NumServers; i++) {
	Proc_NewProc((Address) Proc_ServerProc, PROC_KERNEL, FALSE, 
			&pid, "Proc_ServerProc");
    }

    /*
     * Call the routine to start test kernel processes.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Main_HookRoutine\n");
    }
    Main_HookRoutine();

    /*
     * Print out the amount of memory used.
     */
    printf("MEMORY %d bytes allocated for kernel\n", 
		vmMemEnd - mach_KernStart);

    bootProgress = 26;
    led_display(bootProgress,0,0);
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Creating Init\n");
    }
    /*
     * Start up the first user process.
     */
    Proc_NewProc((Address) Init, PROC_KERNEL, FALSE, &pid, "Init");

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
    int		status;
    static char	*initArgs[] = { INIT, (char *) NIL };
    static char	*altInitArgs[] = { 0, (char *) NIL };
    static int	initLoop = 0;
    int		pid;
    int		i;

    bootProgress = 20;
    /*
     * Indicate that we are alive.
     */
    led_display(0x00, 0, 0);

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("In Init\n");
    }
    Rpc_GetStats(SYS_RPC_ENABLE_SERVICE,1,0);
    led_display(0x50,0,0);
    status = Proc_KernExec(initArgs[0], initArgs);
    printf("Warning: Init: Could not exec %s.\n", initArgs[0]);

    Proc_Exit(1);
}


/*
 *----------------------------------------------------------------------
 *
 *  mainSlaveStart --
 *
 *	This routine is called when a slave processor first starts.
 *
 * Results:
 *	None.

 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
mainSlaveStart()
{

    Proc_ControlBlock *procPtr;
    /*
     * Initialize the machine state of the processor. 
     */
    Mach_InitSlaveProcessor();

    printf("Slave processor %d started\n",Mach_GetProcessorNumber());
    led_display(0x50 + Mach_GetProcessorNumber(),1,0);
    procPtr = Proc_GetCurrentProc();
    procPtr->schedFlags |= SCHED_STACK_IN_USE; 
    procPtr->processor = Mach_GetProcessorNumber(); 
    /* 
     * Enable interrupts.
     */
    ENABLE_INTR();
    Sched_TimeTicks();
    /*
     * Enter the scheduler by calling Proc_Exit.
     */
    Proc_Exit(0);	
}
