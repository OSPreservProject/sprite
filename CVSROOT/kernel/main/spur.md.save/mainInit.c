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
#include "mach.h"

extern void Fs_WakeupProc();
extern void Fs_HandleScavenge();

/*
 *  Pathname of the Init program.
 */
#define INIT	 	"/tmp/spurt"

/*
 * Flags defined in individual's mainHook.c to modify the startup behavior. 
 */

extern Boolean main_Debug;	/* If TRUE then enter the debugger */
extern Boolean main_DoProf;	/* If TRUE then start profiling */
extern Boolean main_DoDumpInit;	/* If TRUE then initialize dump routines */
extern Boolean main_UseAltInit;	/* IF TRUE then try to use /initSprite.new */
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
    Mach_Init();

    bootProgress = 3;
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
    Vm_BootInit();

    /*
     * Initialize all devices.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Dev_Init().\n");
    }
    bootProgress = 6;
    Dev_Init();

    /*
     * Initialize the timer, signal, process, scheduling and synchronization
     * modules' data structures.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Timer_Init().\n");
    }
    bootProgress = 7;
    Timer_Init();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sig_Init().\n");
    }
    bootProgress = 8;
    Sig_Init();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_InitTable().\n");
    }
    bootProgress = 9;
    Proc_InitTable();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sched_Init().\n");
    }
    bootProgress = 10;
    Sched_Init();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Sync_Init().\n");
    }
    bootProgress = 11;
    Sync_Init();

    /*
     * Sys_Printfs are not allowed before this point.
     */  

    Sys_Printf("Sprite kernel: %s\n", SpriteVersion());

    /*
     * Set up bins for the memory allocator.
     */
    Fs_Bin();

    /*
     * Initialize virtual memory.  After this point must use the normal
     * memory allocator to allocate memory.  If you use Vm_BootAlloc then
     * will get a panic into the debugger.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Vm_Init\n");
    }
    bootProgress = 12;
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
    Proc_InitMainProc();

    /*
     * Enable server process manager.
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Proc_ServerInit\n");
    }
    bootProgress = 14;
    Proc_ServerInit();

    /*
     * Initialize the ethernet drivers.
     * Dependencies: Vm_Init
     */
    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Net_Init\n");
    }
    bootProgress = 15;
    Net_Init();

    /*
     * Initialize the recovery module.  Do before Rpc and after Vm_Init.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Recov_Init\n");
    }
    bootProgress = 16;
    Recov_Init();

    /*
     * Initialize the data structures for the Rpc system.  This uses
     * Vm_RawAlloc to so it must be called after Vm_Init.
     * Dependencies: Timer_Init, Vm_Init, Net_Init, Recov_Init
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Calling Rpc_Init\n");
    }
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
    ENABLE_INTR();

    if (main_Debug) {
	DBG_CALL;
    }
    /*
     * Sleep for a few seconds to calibrate the idle time ticks.
     */
    bootProgress =  20;
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
    Rpc_Start();

    /*
     * Initialize the file system. 
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Call Fs_Init\n");
    }
    bootProgress = 22;
    Fs_Init();

    /*
     * Before starting up any more processes get a current directory
     * for the main process.  Subsequent new procs will inherit it.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Call Fs_ProcInit\n");
    }
    bootProgress = 23;
    Fs_ProcInit();

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Bunch of call funcs\n");
    }
    bootProgress = 24;
    /*
     * Start the clock daemon and the routine that opens up the swap directory.
     */
/*
    Proc_CallFunc(Vm_Clock, (ClientData) NIL, 0);
*/
    Proc_CallFunc(Vm_OpenSwapDirectory, (ClientData) NIL, 0);

    /*
     * Start the process that synchronizes the filesystem caches
     * with the data kept on disk.
     */
    Proc_CallFunc(Fs_SyncProc, (ClientData) NIL, 0);

    /*
     * Start the process that scavenges handles.
     */
    Proc_CallFunc(Fs_HandleScavenge, (ClientData)TRUE, 0);

    /*
     * Create a few RPC server processes and the Rpc_Daemon process which
     * will create more server processes if needed.
     */

    if (main_NumRpcServers > 0) {
        Sys_Printf("Creating %d RPC servers", main_NumRpcServers);
	for (i=0 ; i<main_NumRpcServers ; i++) {
	    Rpc_CreateServer(&pid);
	}
        Sys_Printf(" and Rpc_Daemon\n");
    } else {
	Sys_Printf("Creating Rpc_Daemon\n");
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
    Sys_Printf("%d bytes of memory allocated for kernel\n", 
		vmMemEnd - mach_KernStart);

    /*
     * Start up the first user process.
     */

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("Creating Init\n");
    }
    bootProgress = 25;
/*
    Init_ProcFsState();
*/
    Proc_NewProc((Address) Init, PROC_KERNEL, FALSE, &pid, "Init");

    bootProgress = 26;

    {
	Fs_Stream	*filePtr;
	ReturnStatus	status;
	static char	maryBuf[128];
	int		len;

	status = Fs_Open("/etc/spritehosts", FS_READ, FS_FILE, 0, &filePtr);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_FATAL, "Can't open file\n");
	}
	
	len = 127;
	status = Fs_Read(filePtr, maryBuf, 0, &len);
	if (status != SUCCESS || len != 127) {
	    Sys_Panic(SYS_FATAL, "Read returned %x %d\n", status, len);
	}
	status = Fs_Close(filePtr);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_FATAL, "Close returned %x\n", status);
	}
    }
    (void) Sync_WaitTime(time_OneYear);
    Sys_Printf("Main exiting\n");
    Proc_Exit(0);
}

static void	t1();
static void	t2();
Sync_Lock	tagLock = {0, 0};
Sync_Condition	t1Cond;
Sync_Condition	t2Cond;

#define	LOCKPTR &tagLock


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
    led_display(0xc1, 0, 0);

    for (i = 0; i <= 5; i += 3) {
	Sync_WaitTime(time_OneSecond);
	led_display(0xe0 | i, 0, 0);
    }
#ifdef notdef
    /*
     * Fork a new process and then play tag with it.
     */
    Proc_NewProc((Address) t2, PROC_KERNEL, FALSE, &pid, "t2");
    t1();
#endif

    if (main_PrintInitRoutines) {
	Mach_MonPrintf("In Init\n");
    }
    if (main_AltInit != 0) {
	altInitArgs[0] = main_AltInit;
	Sys_Printf("Execing \"%s\"\n", altInitArgs[0]);
	status = Proc_KernExec(altInitArgs[0], initArgs);
	Sys_Panic(SYS_WARNING, "Init: Could not exec %s.\n", altInitArgs[0]);
    }
    status = Proc_KernExec(initArgs[0], initArgs);
    Sys_Panic(SYS_WARNING, "Init: Could not exec %s.\n", initArgs[0]);

    Proc_Exit(1);
}

void t1()
{
    int	i;
    LOCK_MONITOR;

    i = 0;
    while (TRUE) {
	Sync_WaitTime(time_OneSecond);
	led_display(0xa0 | (i & 0xf), 0, 0);
	i++;
	Sync_Broadcast(&t2Cond);
	(void)Sync_Wait(&t1Cond, TRUE);
    }

    UNLOCK_MONITOR;
}


static unsigned int lockVal;

void t2()
{
    int	i;

    lockVal = tagLock.inUse;

    LOCK_MONITOR;

    i = 0;
    while (TRUE) {
	Sync_WaitTime(time_OneSecond);
	led_display(0xb0 | (i & 0xf), 0, 0);
	i++;
	Sync_Broadcast(&t1Cond);
	(void)Sync_Wait(&t2Cond, TRUE);
    }

    UNLOCK_MONITOR;
}





Init_ProcFsState()
{
    Proc_ControlBlock   *procPtr;       /* Main process's proc table entry */
    ReturnStatus        status;         /* General status code return */
    register Fs_ProcessState    *fsPtr; /* FS state ref'ed from proc table */
    static Fs_ProcessState fsState;

    procPtr = Proc_GetCurrentProc();
    procPtr->fsPtr = fsPtr =  &fsState;

    fsPtr->numGroupIDs  = 0;
    fsPtr->groupIDs     =  (int *) NIL;

    fsPtr->cwdPtr = (Fs_Stream *) NIL;

    fsPtr->numStreams = 0;
    fsPtr->streamList = (Fs_Stream **)NIL;
 
    return;
}
	
