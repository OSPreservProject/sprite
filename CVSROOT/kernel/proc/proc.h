/*
 * proc.h --
 *
 *	External declarations of data structures and routines 
 *	for managing processes.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * rcsid $Header$ SPRITE (Berkeley)
 */

#ifndef _PROC
#define _PROC

#include "user/proc.h"
#include "syncMonitor.h"
#include "vm.h"
#include "list.h"
#include "timer.h"
#include "machine.h"
#include "sig.h"
#include "fs.h"
#include "exc.h"

/* DATA STRUCTURES */

/*
 * Structure passed to the function called by Proc_CallFunc.
 */
typedef struct {
    unsigned int	interval;	/* Set by func to cause it to be 
					 * rescheduled. */
    ClientData		clientData;	/* Data given to Proc_CallFunc*(). */
    ClientData		token;		/* Unique token to identify this call.*/
} Proc_CallInfo;

/*
 * Structure to describe an environment.
 */

typedef struct {
    int			refCount;	/* Number of processes using this 
					   environment. */
    int			size;		/* Number of elements in environment. */
    struct ProcEnvironVar *varArray;	/* The environment itself. */
} Proc_EnvironInfo;

/*
 *  Process state flags:
 */
typedef enum {
    PROC_UNUSED,	/* The process doesn't exist yet. */
    PROC_RUNNING,	/* The process is executing on a processor. */
    PROC_READY,		/* The process is ready to execute. */
    PROC_WAITING,	/* The process is waiting for an event to occur such
			 * as I/O completion or a mutex lock released. */
    PROC_EXITING,	/* The process has terminated and is on the 
			 * exiting list. */
    PROC_DEAD,		/* The process has been terminated is on the dead list*/
    PROC_DEBUGABLE,	/* The process is eligible for debugging. */
    PROC_MIGRATED,	/* The process is running on a remote machine. */
    PROC_NEW,		/* The process was just created. */
    PROC_SUSPENDED	/* The process is suspended. */
} Proc_State;

/*
 *  Proc_PCBLinks is used to link the PCB entry into various doubly-linked
 *  lists. For example, processes waiting on an event are linked togther.
 */

typedef struct {
    List_Links links;			/* Linked list for the hash chain. */
    struct Proc_ControlBlock *procPtr;	/* Back pointer to this structure. */
} Proc_PCBLink;


/*
 *  The Proc_ControlBlock structure:
 *   It contains information to manage the process such as virtual 
 *   memory usage, cpu usage, scheduling info, process state, 
 *   which processor the process is executing on, etc.
 */

typedef struct Proc_ControlBlock {
    List_Links	links;		/* Used to link processes together. */

    int		processor;	/* Processor number the process is running on
				 * or wants to run on if the processor is
				 * available.  */

    Proc_State	state;		/* Describes a process's current running state.
				 * >>> See Proc_State definitions above. */ 

    int		genFlags;	/* Flags to describe a processes overall state.
				 * >>> See definitions below */ 
    int		syncFlags;	/* Flags used by the sync module. */
    int		schedFlags;	/* Flags used by the sched module. */
    int		exitFlags;	/* Flags used by the exit-detach-
			 	 * wait monitor. */

    List_Links		childListHdr;	/* Header for list of children. */
    List_Links		*childList;	/* Pointer to header of list. */
    Proc_PCBLink	siblingElement;	/* Element in list of sibling 
					 * processes. */
    Proc_PCBLink	familyElement;	/* Element in a list of family
					   members. */

    /*
     *-----------------------------------------------------------------
     *
     *   Various Process Identifiers.
     *
     *-----------------------------------------------------------------
     */

    Proc_PID	processID;		/* Index into process table for 
					 * this process. */
    
    Proc_PID	parentID;		/* The process ID of the parent 
					 * of this process. */
    int		familyID;		/* The id of the process family that 
					 * this process belongs to. */
    int		userID;			/* The user id is used to check access
					 * rights to files and check ability
					 * to signal other processes. */
    int		effectiveUserID;	/* The effective user id is used
					 * for setuid access. */
    int		numGroupIDs;		/* The length of the groupIDs array */
    int		*groupIDs;		/* An array of group IDs.  Group IDs
					 * are used similarly to the User ID. */

    /*
     *-----------------------------------------------------------------
     *
     *    Synchronization fields.
     *
     * Synchronization state includes an event the process is waiting on.
     * PCB's are linked into a hash chain keyed on this event.
     *
     *-----------------------------------------------------------------
     */

    int		 event;		 /* Event # the process is waiting for. */
    Proc_PCBLink eventHashChain; /* Hash chain this PCB is linked to */

    /*
     * Monitor conditions for locking this PCB entry.
     */

    Sync_Condition	waitCondition;
    Sync_Condition	lockedCondition;

    /*
     * Fields for remote waiting.  A token is kept to guard against the
     * race between the wakeup message and the process's decision to sleep.
     */

    int			waitToken;

    /*
     *-----------------------------------------------------------------
     *
     *    Scheduling fields.
     *
     *-----------------------------------------------------------------
     */


    int 	 billingRate;	/* Modifies the scheduler's calculation of
				 * the processes priority.  */
    unsigned int recentUsage;	/* Amount of CPU time used recently */
    unsigned int weightedUsage;	/* Smoothed avg. of CPU usage, weighted by
				 * billing rate. */
    unsigned int unweightedUsage; /* Smoothed avg. of CPU usage, not weighted by
				   * billing rate. */

    /*
     *-----------------------------------------------------------------
     *
     *    Accounting and Resource Usage fields.
     *
     *-----------------------------------------------------------------
     */

    Timer_Ticks kernelCpuUsage;	/* How much time has been spent in kernel mode*/
    Timer_Ticks userCpuUsage;	/* How much time has been spent in user mode. */

    Timer_Ticks childKernelCpuUsage;	/* Sum of time spent in kernel mode for 
				 	 * all terminated children. */
    Timer_Ticks childUserCpuUsage;	/* Sum of time spent in user mode for
				 	 * all terminated children. */
    int 	numQuantumEnds;		/* number of times the process was 
				 	 * context switched due to a quantum 
					 * end. */
    int		numWaitEvents;		/* number of times the process was
				 	 * context switched due to its waiting 
					 * for an event. */
    unsigned int schedQuantumTicks;	/* Number of clock ticks until this 
					 * process is due to be switched out. */


    /*
     *-----------------------------------------------------------------
     *
     *   Machine-Dependent fields.
     *
     *	General processor registers, stack information.
     *
     *-----------------------------------------------------------------
     */

    int	saveRegs[MACH_NUM_GENERAL_REGS];/* The general registers used by
					   the process when it was context
					   switched. */
    int	stackStart;			/* Start of the kernel stack for this
					   process. */
    /*
     * The state of a user process when it traps into the kernel.  
     *
     * NOTE: The registers and program counter are not automatically saved
     * 	     when a user process traps into the kernel.  These fields provide
     *	     a place to store the information but it is up to individual trap
     *	     handling routines to save the registers and pc.
     */

    int		genRegs[MACH_NUM_GENERAL_REGS];	/* All of the general purpose 
					 	 * registers.  */
    int		progCounter;			/* The program counter when the 
					   	 * system call occured. */
    short	statusReg;			/* Status register */

    /*
     *-----------------------------------------------------------------
     *
     *   Virtual Memory fields.
     *
     *-----------------------------------------------------------------
     */

    Vm_Segment	*segPtrArray[4];	/* The four segments for this process.*/
    int		context;		/* Which context this process is
					   executing in. */
    int		contextReg;		/* The value of the context register
					   when this process was context 
					   switched. */
    int		vmFlags;		/* Flags used by VM module (see vm.h) */

    /*
     *-----------------------------------------------------------------
     *
     *   I/O and File System fields.
     *
     *-----------------------------------------------------------------
     */

    Fs_Stream	  *cwdPtr;		/* The current working directory. */
    unsigned int   filePermissions;	/* The bits in this mask correspond
					 * to the permissions mask of a file.
					 * If one of these bits is set it
					 * TURNS OFF the corresponding
					 * permission when a file is created. */
    int		   numStreams;		/* Size of streamList array. */
    Fs_Stream	   **streamList;	/* Array of pointers to open files.
					 * This list is indexed by an integer
					 * known as a streamID. */

    /*
     *-----------------------------------------------------------------
     *
     *   Termination Reason, Status and Status Subcode Information.
     *
     *-----------------------------------------------------------------
     */

    int	termReason;		/* Reason why process has died or
				 * it has been detached. */
				/* >>> See definitions in procUser.h */
    int	termStatus;		/* Exit/detach status or signal number
				 * that caused the process to die. */
				/* >>> See definitions in procUser.h */
    int	termCode;		/* The code for the signal. */
				/* >>> See definitions in procUser.h */


    /*
     *-----------------------------------------------------------------
     *
     *	Signals
     *
     *-----------------------------------------------------------------
     */

    int		sigHoldMask;		/* Mask of signals to be held. */
    int		sigPendingMask;		/* Mask of pending signals. */
    					/* Array of the different types
					   of actions for signals. */
    int		sigActions[SIG_NUM_SIGNALS];
    					/* Array of signal hold masks for 
					   signal handlers. */
    int		sigMasks[SIG_NUM_SIGNALS];
					/* Array of signal handlers for 
					   signals. */
    int		sigCodes[SIG_NUM_SIGNALS];
    int		sigFlags;		/* Flags to indicate the signal 
					   state. */
    int		oldSigHoldMask;		/* Mask of held signals when a
					   Sig_Pause call starts. */

    /*
     * Info for interval timers. The timer info is not put directly in
     * this struct so additional timers can be added without extending
     * the struct. This information is used to deliver the SIG_TIMER signal
     * to the process.
     */

    struct ProcIntTimerInfo	*timerArray;

    /*
     *---------------------------------------------------------------------
     *
     * Data for process migration.
     *
     *---------------------------------------------------------------------
     */

    int			peerHostID;	/* If on home node, ID of remote node.
					 * If on remote node, ID of home node.
					 * If not migrated, undefined. */
    Proc_PID		peerProcessID; 	/* If on remote note, process ID on
					 * home node, and vice-versa. */
    Exc_TrapStack	*trapStackPtr;	/* Contents of trap stack on home node,
					 * saved in PCB on remote node. */
    struct Proc_ControlBlock
	             *rpcClientProcess;	/* procPtr for migrated process
					 * performing system call, if
					 * applicable. */

    /*
     *---------------------------------------------------------------------
     *
     *  Miscellaneous items:
     *
     *---------------------------------------------------------------------
     */

    /*
     * Info that describes the process's environment variable table.
     */

    Proc_EnvironInfo	*environPtr;

    /*
     * Set-jump structure (NIL if no state saved).
     */

    Sys_SetJumpState	*setJumpStatePtr;


    /*
     * Name of the exec'd file. At end because is so big.
     */

    char		codeFileName[FS_MAX_NAME_LENGTH];

} Proc_ControlBlock;



/*
 * Process attributes flags:
 *
 *  PROC_KERNEL   	        - The process is a kernel process.
 *  PROC_USER     	        - The process is a user process.
 *  PROC_DEBUGGED		- The process is being debugged by the system
 *				  debugger.
 *  PROC_DEBUG_ON_EXEC		- The process will start in debugged mode.
 *  PROC_DEBUG_WAIT		- A debugger is waiting for this process to go
 *				  onto the debug list.
 *  PROC_SINGLE_STEP		- The process will have the trace bit set
 *				  before it runs.
 *  PROC_MIG_PENDING		- The process will be migrated when it
 *				  completes its next trap.
 *  PROC_DONT_MIGRATE		- The process should not be migrated yet, even
 *				  when it traps.
 *  PROC_FOREIGN		- The process has been migrated from another
 *				  workstation to this one.
 *  PROC_DIEING			- The process is comitting hari-kari.
 *  PROC_LOCKED			- This process is locked.
 *  PROC_NO_VM			- The virtual memory resources have been
 *				  freed up for this user process.
 *  PROC_MIGRATING		- The process is in the middle of migrating
 *				  to another workstation.  This happens after
 *				  PROC_MIG_PENDING is set but before the
 *				  process's state becomes PROC_MIGRATED and
 *				  its PROC_MIGRATION_DONE flag is set.
 *  PROC_MIGRATION_DONE		- indicates successful completion of a
 *				  migration trap.
 */

#define PROC_KERNEL			0x00001
#define PROC_USER			0x00002
#define PROC_DEBUGGED			0x00004
#define PROC_DEBUG_ON_EXEC		0x00008
#define PROC_SINGLE_STEP		0x00010
#define PROC_DEBUG_WAIT			0x00020
#define PROC_MIG_PENDING		0x00040
#define PROC_DONT_MIGRATE		0x00080
#define PROC_FOREIGN			0x00100
#define PROC_DIEING			0x00200
#define PROC_LOCKED			0x00400
#define PROC_NO_VM			0x00800
#define PROC_MIGRATING			0x01000
#define PROC_MIGRATION_DONE		0x02000


/*
 * Exit-detach-wait monitor flags:
 *
 *  PROC_DETACHED		- This process is detached from its parent.
 *				  When this process exits, it won't go on the
 *				  exiting processes list.
 *  PROC_WAITED_ON		- This process is detached and the parent has 
 *				  already done a Proc_Wait on it.
 *  PROC_SUSPENDED_AND_WAITED_ON - This process is suspended and it has been
 *				   waited on.  This only gets used if 
 *				   PROC_WAITED_ON isn't set.
 */

#define PROC_DETACHED			0x01
#define PROC_WAITED_ON			0x02
#define	PROC_SUSPENDED_AND_WAITED_ON	0x04



/*
 *  proc_RunningProcesses points to an array of pointers to
 *  Proc_ControlBlock structures of processes currently running on each
 *  CPU.  It is initialized by Proc_Init at boot time to the appropriate
 *  size, which depends on the workstation configuration.
 */

extern Proc_ControlBlock  **proc_RunningProcesses;


/*
 *  proc_PCBTable is the array of all valid PCB's in the system.
 *  It is initialized by Proc_Init at boot time to the appropriate size,
 *  which depends on the workstation configuration.
 */

extern Proc_ControlBlock **proc_PCBTable;


/*
 *   Keep track of the maximum number of processes at any given time.
 */

extern int proc_MaxNumProcesses;

/*
 * Constants for Proc_Exec().
 *
 * PROC_MAX_EXEC_ARG_LENGTH	The maximum length of any argument that is
 *				passed to exec.
 * PROC_MAX_EXEC_ARGS		The maximum number of arguments that can be
 *				passed to an exec'd process.
 */

#define	PROC_MAX_EXEC_ARG_LENGTH	1024
#define	PROC_MAX_EXEC_ARGS		512

/*
 * Masks to extract the proc table index and the generation number from
 * a process id.  Process IDs need to be unique across reuses of the same
 * procTable slot, and they need to be unique from host to host.
 */

#define	PROC_INDEX_MASK		0x000000FF
#define	PROC_ID_NUM_MASK	0x0000FF00
#define PROC_ID_NUM_SHIFT	8
#define	PROC_GEN_NUM_MASK	0x000F0000
#define PROC_GEN_NUM_SHIFT	16

/*
 *  Macros to manipulate process IDs.
 */

#define Proc_ComparePIDs(p1, p2) (p1 == p2)

#define Proc_GetPCB(pid) (proc_PCBTable[pid & PROC_INDEX_MASK])

#define	Proc_PIDToIndex(pid) (pid & PROC_INDEX_MASK)

#define Proc_ValidatePID(pid) \
    (((pid & PROC_INDEX_MASK) < proc_MaxNumProcesses) && \
     ((pid == proc_PCBTable[pid & PROC_INDEX_MASK]->processID)))

#define PROC_GET_VALID_PCB(pid, procPtr) \
    if ((pid & PROC_INDEX_MASK) >= proc_MaxNumProcesses) { \
	procPtr = (Proc_ControlBlock *) NIL; \
    } else { \
	procPtr = proc_PCBTable[pid & PROC_INDEX_MASK]; \
	if (pid != procPtr->processID) { \
	    procPtr = (Proc_ControlBlock *) NIL; \
	} \
    }

#define	Proc_GetHostID(pid) ((pid & PROC_ID_NUM_MASK) >> PROC_ID_NUM_SHIFT)

/*
 * Macros to determine and set the "actual" currently running process.
 */

#define	Proc_GetActualProc(processor) proc_RunningProcesses[processor]
#define Proc_SetActualProc(processor, processPtr) \
		proc_RunningProcesses[processor] = processPtr

#define Proc_GetCurrentProc(processor) Proc_GetActualProc(processor)
#define Proc_SetCurrentProc(process, processPtr) \
		Proc_SetActualProc (process, processPtr)

/*
 * Various routines use Proc_IsMigratedProcess to decide whether the
 * effective process is different from the actual process (i.e.,
 * migrated).  This macro bypasses the procedure call, since
 * proc_RunningProcesses[processor] must be non-NIL.
 */

#define	Proc_IsMigratedProcess(processor) \
		(proc_RunningProcesses[processor]->rpcClientProcess != \
		((Proc_ControlBlock *) NIL))

/* 
 * External procedures.
 */

extern void		  	Proc_InitTable();
extern void		  	Proc_InitMainProc();
extern ReturnStatus		Proc_NewProc();
extern void			ProcStartUserProc();
extern void			Proc_ExitInt();
extern void			Proc_Exit();
extern void			Proc_DetachInt();
extern ReturnStatus		Proc_Detach();
extern void			ProcInformParent();	
extern void			Proc_Reaper();
extern void			Proc_NotifyMigratedWaiters();
extern void			Proc_PutOnDebugList();
extern void			Proc_Suspend();
extern void			Proc_Resume();


extern ReturnStatus		Proc_EvictForeignProcs();
extern ReturnStatus		Proc_EvictProc();
extern Boolean			Proc_IsMigratedProc();
extern void			Proc_FlagMigration();
extern void			Proc_MigrateTrap();
extern void			Proc_OkayToMigrate();
extern ReturnStatus		Proc_MigSendUserInfo();
extern void			ProcResumeMigProc();
extern ReturnStatus		Proc_DoRemoteCall();
extern void			Proc_SetEffectiveProc();
extern Proc_ControlBlock *	Proc_GetEffectiveProc();
extern ReturnStatus		Proc_ByteCopy();
extern ReturnStatus		Proc_MakeStringAccessible();
extern void			Proc_MakeUnaccessible();
extern void			Proc_MigrationStartTracing();
extern void			Proc_DestroyMigratedProc();

extern void			ProcInitMainEnviron();
extern void			ProcSetupEnviron();
extern void			ProcDecEnvironRefCount();

extern void			Proc_WakeupIfSignal();
extern void			Proc_SetServerPriority();

extern	int			Proc_KillAllProcesses();

extern	void			Proc_Unlock();
extern	void			Proc_Lock();
extern	Proc_ControlBlock	*Proc_LockPID();
extern	ReturnStatus		Proc_LockFamily();
extern	void			Proc_UnlockFamily();
extern	void			Proc_TakeOffDebugList();
extern	Boolean			Proc_HasPermission();

extern	void			Proc_ServerInit();
extern	void			Proc_CallFunc();
extern	ClientData		Proc_CallFuncAbsTime();
extern	void			Proc_ServerProc();
extern	int			proc_NumServers;

#endif _PROC
