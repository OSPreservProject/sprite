/*
 * proc.h --
 *
 *	External declarations of data structures and routines 
 *	for managing processes.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * rcsid $Header$ SPRITE (Berkeley)
 */

#ifndef _PROC
#define _PROC

#ifdef KERNEL
#include "user/proc.h"
#include "user/sync.h"
#include "syncLock.h"
#include "list.h"
#include "timer.h"
#include "sig.h"
#include "mach.h"
#else
#include <proc.h>
#include <sync.h>
#include <kernel/syncLock.h>
#include <list.h>
#include <kernel/timer.h>
#include <kernel/sig.h>
#include <kernel/mach.h>
#endif /* */

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

/*
 * PROC_INDEX_MASK is defined in user/proc.h.
 * #define	PROC_INDEX_MASK		0x000000FF
 */
#define	PROC_ID_NUM_MASK	0x0000FF00
#define PROC_ID_NUM_SHIFT	8
#define	PROC_GEN_NUM_MASK	0x000F0000
#define PROC_GEN_NUM_SHIFT	16


/*
 * Number of locks that can be pushed on the lock stack for a process. The
 * stack is used in the sync module to determine the locking structure of the
 * system.
 */
#define PROC_LOCKSTACK_SIZE 10


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
 * >>> Process state flags have been moved to user/proc.h <<<
 */

/*
 *  Proc_PCBLinks is used to link the PCB entry into various doubly-linked
 *  lists. For example, processes waiting on an event are linked togther.
 */

typedef struct {
    List_Links links;			/* Linked list for the hash chain. */
    struct Proc_ControlBlock *procPtr;	/* Back pointer to this structure. */
} Proc_PCBLink;


/*
 * Proc_Time is used to represent time such that it can be understood
 * by users through Proc_GetPCBInfo in a machine independent format and
 * by the kernel in a machine dependent ticks format.  Thus all users of
 * Proc_Time in the kernel should always use the ticks field and user
 * programs that call Proc_GetPCBInfo should use the time field.
 */
typedef union {
    Timer_Ticks	ticks;	/* The kernel's notion of time. */
    Time	time;	/* The user's notion of time. */
} Proc_Time;

typedef struct {
    int		type;		/* type of lock */
    Address	lockPtr;	/* Ptr to lock */
} Proc_LockStackElement;

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
     *	Note that the user and effectiveUser ID are kept here because
     *	they are used for permission checking in various places.  There
     *	is also a list of group IDs which is kept in the filesystem state.
     *
     *-----------------------------------------------------------------
     */

    Proc_PID	processID;		/* Actual process ID of this
					 * process (for migrated processes
					 * this is different than the PID
					 * that the user sees). */
    Proc_PID	parentID;		/* The process ID of the parent 
					 * of this process. */
    int		familyID;		/* The id of the process family that 
					 * this process belongs to. */
    int		userID;			/* The user id is used to check access
					 * rights to files and check ability
					 * to signal other processes. */
    int		effectiveUserID;	/* The effective user id is used
					 * for setuid access. */

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

    Proc_Time kernelCpuUsage;	/* How much time has been spent in kernel mode*/
    Proc_Time userCpuUsage;	/* How much time has been spent in user mode. */

    Proc_Time childKernelCpuUsage;	/* Sum of time spent in kernel mode for 
				 	 * all terminated children. */
    Proc_Time childUserCpuUsage;	/* Sum of time spent in user mode for
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
    struct	Mach_State	*machStatePtr;


    /*
     *-----------------------------------------------------------------
     *
     *   Virtual Memory fields.
     *
     *-----------------------------------------------------------------
     */
    struct	Vm_ProcInfo	*vmPtr;

    /*
     *-----------------------------------------------------------------
     *
     *   I/O and File System fields.
     *
     *-----------------------------------------------------------------
     */

    struct Fs_ProcessState	*fsPtr;

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
     * Arguments for the process, taken from Proc_Exec.
     */
    char	*argString;

#ifdef LOCKDEP
    /*
     * Stack of locks that process has grabbed.
     */
     Proc_LockStackElement	lockStack[PROC_LOCKSTACK_SIZE];
     int			lockStackSize;
#endif

#ifndef CLEAN_LOCK
    /*
     * Information on contention for PCB locks. PCB locks are implemented
     * as a bit in the genflag field and don't use the standard locking
     * stuff. The following field is used to keep lock information. 
     * Its type is Sync_Semaphore, but it is not used as such.
     */
     Sync_Semaphore		lockInfo;
#endif

    /*
     * Used to speed up basic kernel-call processing.  These two fields
     * must be next to each other in the table, and in the order below.
     * If you change this, you'll have to change the assembler code that
     * takes kernel-call traps.
     */

    ReturnStatus (**kcallTable)();	/* Pointer to array of addresses,
					 * which are procedures to handle
					 * the various kernel calls.  Points
					 * to a different place for migrated
					 * processes than for processes running
					 * at home. */
    int specialHandling;		/* If non-zero, means the process
					 * requires special (slower) handling
					 * (deliver signal, switch contexts,
					 * ect.) on return from the next kernel
					 * call. */

    /*
     *---------------------------------------------------------------------
     *
     *  User level profiling information
     *
     *---------------------------------------------------------------------
     */

     short *Prof_Buffer;    /* Pointer to an array of profiling information
                             * in the process's address space. */
     int Prof_BufferSize;   /* The size of Prof_Buffer. */
     int Prof_Offset;       /* Value subtracted from the program counter */
     int Prof_Scale;        /* 16 bit fixed point fraction.  Scales the PC
                             * to fit in the Prof_Buffer */
     int Prof_PC;           /* Program counter recorded during the last
                             * timer tick. */

    /*
     * This needs to go with the other migration stuff but can't without
     * a world recompile.
     */
    Address	remoteExecBuffer;	 /* Buffer to store info for remote
					  * exec prior to migration */

} Proc_ControlBlock;

/*
 * Exit-detach-wait monitor flags:
 *
 *  PROC_DETACHED		- This process is detached from its parent.
 *				  When this process exits, it won't go on the
 *				  exiting processes list.
 *  PROC_WAITED_ON		- This process is detached and the parent has 
 *				  already done a Proc_Wait on it.
 *  PROC_SUSPEND_STATUS		- The process went into the suspended state
 *				  and it hasn't been waited on yet.
 *  PROC_RESUME_STATUS		- The process was resumed and it hasn't been 
 *				  waited on yet.
 *  PROC_STATUSES		- The union of the two above statuses.
 */

#define PROC_DETACHED		0x01
#define PROC_WAITED_ON		0x02
#define	PROC_SUSPEND_STATUS	0x04
#define	PROC_RESUME_STATUS	0x08
#define	PROC_STATUSES		(PROC_SUSPEND_STATUS | PROC_RESUME_STATUS)





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
 * set to TRUE to disallow all migrations to this machine.
 */
extern Boolean proc_RefuseMigrations;

/*
 *  Macros to manipulate process IDs.
 */

#define Proc_ComparePIDs(p1, p2) (p1 == p2)

#define Proc_GetPCB(pid) (proc_PCBTable[pid & PROC_INDEX_MASK])

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

#define	Proc_GetActualProc() \
	proc_RunningProcesses[Mach_GetProcessorNumber()]
#define Proc_SetActualProc(processPtr) \
	proc_RunningProcesses[Mach_GetProcessorNumber()] = processPtr

#define Proc_GetCurrentProc() Proc_GetActualProc()
#define Proc_SetCurrentProc(processPtr)	Proc_SetActualProc(processPtr)

/*
 * Various routines use Proc_IsMigratedProcess to decide whether the
 * effective process is different from the actual process (i.e.,
 * migrated).  This macro bypasses the procedure call, since
 * proc_RunningProcesses[processor] must be non-NIL.
 */

#define	Proc_IsMigratedProcess() \
    (proc_RunningProcesses[Mach_GetProcessorNumber()]->rpcClientProcess != \
		((Proc_ControlBlock *) NIL))

/*
 * Used to get the lock at the top of the lock stack without popping it off.
 */
#define Proc_GetCurrentLock(pcbPtr, typePtr, lockPtrPtr) \
    { \
	if ((pcbPtr)->lockStackSize <= 0) { \
	    *(typePtr) = -1; \
	    *(lockPtrPtr) = (Address) NIL; \
	} else { \
	    *(typePtr) = (pcbPtr)->lockStack[(pcbPtr)->lockStackSize-1].type; \
	    *(lockPtrPtr) = \
		(pcbPtr)->lockStack[(pcbPtr)->lockStackSize-1].lockPtr; \
	} \
    }
/* 
 * External procedures.
 */

extern void		  	Proc_Init();
extern void		  	Proc_InitMainProc();
extern ReturnStatus		Proc_NewProc();
extern void			ProcStartUserProc();
extern void			Proc_ExitInt();
extern void			Proc_Exit();
extern void			Proc_DetachInt();
extern ReturnStatus		Proc_Detach();
extern void			Proc_InformParent();	
extern void			Proc_Reaper();
extern void			Proc_NotifyMigratedWaiters();
extern void			Proc_PutOnDebugList();
extern void			Proc_SuspendProcess();
extern void			Proc_ResumeProcess();
extern int			Proc_ExecEnv();
extern int			Proc_RemoteExec();
extern ReturnStatus 		Proc_GetHostIDs();


extern ReturnStatus		Proc_EvictForeignProcs();
extern ReturnStatus		Proc_EvictProc();
extern Boolean			Proc_IsMigratedProc();
extern void			Proc_FlagMigration();
extern void			Proc_MigrateTrap();
extern void			Proc_OkayToMigrate();
extern ReturnStatus		Proc_MigSendUserInfo();
extern ReturnStatus		Proc_DoRemoteCall();
extern void			Proc_SetEffectiveProc();
extern Proc_ControlBlock *	Proc_GetEffectiveProc();
extern ReturnStatus		Proc_ByteCopy();
extern ReturnStatus		Proc_MakeStringAccessible();
extern void			Proc_MakeUnaccessible();
extern void			Proc_MigrateStartTracing();
extern void			Proc_DestroyMigratedProc();
extern void			Proc_NeverMigrate();

extern void			ProcInitMainEnviron();
extern void			ProcSetupEnviron();
extern void			ProcDecEnvironRefCount();

extern void			Proc_SetServerPriority();

extern	int			Proc_KillAllProcesses();
extern	void			Proc_WakeupAllProcesses();

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

extern  ReturnStatus		Proc_Dump();
extern  ReturnStatus		Proc_DumpPCB();

extern  void			Proc_RemoveFromLockStack();
extern  void			Proc_PushLockStack();

/*
 * The following are kernel stubs corresponding to system calls.  They
 * used to be known by the same name as the system call, but the C library
 * has replaced them at user level in order to use the stack environments.
 * The "Stub" suffix therefore avoids naming conflicts with the library.
 */

extern ReturnStatus		Proc_SetEnvironStub();
extern ReturnStatus		Proc_UnsetEnvironStub();
extern ReturnStatus		Proc_GetEnvironVarStub();
extern ReturnStatus		Proc_GetEnvironRangeStub();
extern ReturnStatus		Proc_InstallEnvironStub();
extern ReturnStatus		Proc_CopyEnvironStub();
extern int                      Proc_KernExec();

#endif /* _PROC */
