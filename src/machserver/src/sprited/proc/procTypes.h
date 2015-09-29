/*
 * procTypes.h --
 *
 *	External declarations of data structures
 *	for managing processes.
 *
 * Copyright 1986, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * rcsid $Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procTypes.h,v 1.11 92/07/13 21:13:40 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCTYPES
#define _PROCTYPES

#include <sprite.h>
#include <spriteTime.h>
#include <servers/machid_types.h>

#ifdef SPRITED
#include <syncTypes.h>
#include <user/proc.h>
#include <user/sig.h>
#include <timerTick.h>
#include <vmTypes.h>
#else
#include <sprited/syncTypes.h>
#include <proc.h>
#include <sig.h>
#include <sprited/timerTick.h>
#include <sprited/vmTypes.h>
#endif

/*
 * Constants for Proc_Exec().  
 *
 * PROC_MAX_EXEC_ARG_LENGTH	The maximum length of all arguments that are
 *				passed to exec.  Also used to bound any
 *				individual argument.
 * PROC_MAX_EXEC_ARGS		The maximum number of arguments that can be
 *				passed to an exec'd process.  By making it
 *				the same, each argument could conceivably be
 *				a single byte.
 */

#define	PROC_MAX_EXEC_ARG_LENGTH	20480
#define	PROC_MAX_EXEC_ARGS		20480

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

typedef int ProcExitFlags;

#define PROC_DETACHED		0x01
#define PROC_WAITED_ON		0x02
#define	PROC_SUSPEND_STATUS	0x04
#define	PROC_RESUME_STATUS	0x08
#define	PROC_STATUSES		(PROC_SUSPEND_STATUS | PROC_RESUME_STATUS)


/* 
 * When creating a new "kernel" process, a Proc_ProcessRoot is passed 
 * in as the function that the process should execute.
 */
typedef void (*Proc_ProcessRoot) _ARGS_((void));



/* DATA STRUCTURES */

/*
 * Structure passed to the function called by Proc_CallFunc.
 */
typedef struct {
    Time		interval;	/* Set by func to cause it to be 
					 * rescheduled. */
    ClientData		clientData;	/* Data given to Proc_CallFunc*(). */
    ClientData		token;		/* Unique token to identify this call.*/
} Proc_CallInfo;

/*
 * Structure to describe an environment.
 */

typedef struct {
    int			refCount;	/* Number of processes using this 
					 * environment. */
    int			size;		/* Number of elements in environment. */
    struct ProcEnvironVar *varArray;	/* The environment itself. */
} Proc_EnvironInfo;

/*
 * >>> Process state flags have been moved to user/proc.h <<<
 */

/*
 *  Proc_PCBLinks is used to link the PCB entry into various doubly-linked
 *  lists. For example, processes belonging to the same family are 
 *  linked together.
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
 * Shared-memory user processes are handled by having multiple threads
 * in a single task, so the per-task information is in a separate
 * struct.  This struct is not used for server processes.
 */

typedef struct {
    int		refCount;
    mach_port_t	task;		/* Mach task */
    mtask_t	machId;		/* magic number from machid server 
				 * (for debugging) */
    Vm_TaskInfo	vmInfo;		/* VM information for the task */
} ProcTaskInfo;

/* 
 * Process control block.  
 * 
 * There are two types of processes: user processes and "kernel" 
 * processes.  A user process is a full-blown process, with an 
 * associated Mach task and thread.  A kernel process is just a C 
 * Thread in the server, with its PCB entry filled in to keep the rest 
 * of Sprite happy.
 * 
 * Mach lets us rename ports to arbitrary values, which means we can
 * equate a port name with the address of a corresponding data
 * structure.  For (user) processes we have two different ports that
 * we want to associate with the PCB: one is the system call request
 * port, and the other is the exception port.  The system call request
 * port we rename to the address of the process's PCB.  The exception
 * port we rename to the address of the back pointer.
 * 
 * Note: when adding fields to the PCB, there are many places you 
 * have to think about initialization: InitPCB, Proc_InitMainProc, 
 * ProcGetUnusedPCB, Proc_NewProc, RpcProcFork, and 
 * ProcMigAcceptMigration.  (XXX The latter two haven't been checked 
 * at all.)
 */

typedef struct Proc_ControlBlock {
    List_Links	links;		/* Used to link processes together. */
    int		magic;		/* magic number to verify is pcb */
    struct Proc_ControlBlock *backPtr; /* used as name for exception port */
    Proc_State	state;		/* current state (suspended, ready, etc.) */
    Proc_Flags	genFlags;	/* Flags to describe a process's
				 * overall state. */
    Sync_PCBFlags syncFlags;	/* Flags used by the sync module. */
    ProcExitFlags exitFlags;	/* Flags used by the exit-detach-
			 	 * wait monitor. */

    List_Links		childListHdr;	/* Header for list of children. */
    List_Links		*childList;	/* Pointer to header of list. */
    Proc_PCBLink	siblingElement;	/* Element in list of sibling 
					 * processes. */
    Proc_PCBLink	familyElement;	/* Element in a list of family
					   members. */

    Proc_PCBLink	deadElement; /* element in system-wide list of dead 
				      * processes */ 

    /*
     *-----------------------------------------------------------------
     *
     *   VM info and fields for dealing with the Mach kernel (user processes 
     *   only).
     *	
     *-----------------------------------------------------------------
     */
    
    mach_port_t syscallPort;	/* port used by process for Sprite requests */
    mach_port_t exceptionPort;	/* port used for exception requests */
    mach_port_t	thread;		/* Mach thread; null if server process */
    ProcTaskInfo *taskInfoPtr;	/* information for the (possibly shared) 
				 * task; null for server processes or 
				 * user processes that are about to die */

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
     *    XXX Maybe some of the condition variables can be merged?
     *
     *-----------------------------------------------------------------
     */

    Sync_Condition	*currCondPtr; /* ptr to the condition that the 
				       * process is currently waiting
				       * on.  Protected by a mutex in
				       * the sync module. */

    Sync_Condition	waitCondition; /* the process waits on this 
					* condition if it's waiting
					* for a child process to do
					* something */
    Sync_Condition	lockedCondition; /* if you can't lock a process, 
					  * wait on that process's
					  * lockedCondition */
    Sync_Condition	sleepCondition;	/* the process waits on this 
					 * condition if it's sleeping for a
					 * timed interval */
    Sync_Condition	remoteCondition; /* the process waits on this 
					  * condition if it's waiting for a
					  * remote event */
    Sync_Condition	resumeCondition; /* the process waits on this 
					  * condition if it's been
					  * suspended */

    /*
     * Fields for remote waiting.  A token is kept to guard against the
     * race between the wakeup message and the process's decision to sleep.
     */

    int			waitToken;

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
					/* Array of actions and signal 
					 * handlers for signals. */
    int		sigMasks[SIG_NUM_SIGNALS];
    					/* Array of signal hold masks for 
					   signal handlers. */
    int		sigCodes[SIG_NUM_SIGNALS];
    int		sigFlags;		/* Flags to indicate the signal 
					   state. */
    int		oldSigHoldMask;		/* Mask of held signals when a
					   Sig_Pause call starts. */
    int		sigAddr;		/* Address of the fault. */
    Address	sigTrampProc;		/* user's trampoline code */

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
    Address	remoteExecBuffer; 	/* Buffer to store info for remote
					 * exec prior to migration. */
    Address	migCmdBuffer;		/* Buffer to store multi-part
					 * migration command. */
    int		migCmdBufSize;		/* Size of migCmdBuffer, for
					 * sanity checks. */
    int		migFlags;		/* Flags used for migration. (Needs
					 * to be reorganized to include things
					 * currently in genFlags but that will
					 * also require a world recompile.) */
    Proc_Time   preEvictionUsage;	/* CPU usage (user + kernel)
					 * as of the start of
					 * eviction. */

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

    /* 
     * Instrumentation & accounting.
     */
    Time	copyInTime;	/* time in copyin for current system call; 
				 * should be accessed only by the owning
				 * process */
    Time	copyOutTime;	/* ditto, but for copyout */

    /* 
     * Debugging information for locks.
     */

#ifdef LOCKDEP
    /*
     * Stack of locks that process has grabbed.
     */
    Proc_LockStackElement	lockStack[PROC_LOCKSTACK_SIZE];
    int		lockStackSize;
#endif
    int		locksHeld;	/* number of locks currently held by 
				 * the process; not necessarily the
				 * same as lockStackSize because of
				 * the way the stack is popped */

#ifndef CLEAN_LOCK
    /*
     * Information on contention for PCB locks. PCB locks are implemented
     * as a bit in the genflag field and don't use the standard locking
     * stuff. The following field is used to keep lock information. 
     * Its type is Sync_Semaphore, but it is not used as such.
     */
     Sync_Semaphore lockInfo;
#endif

    /*
     * UNIX compatibility.
     */

    int         unixErrno;               /* Errno for unix system call. */
    /* 
     * As a random convention, we'll set unixProgress to -1 if the 
     * program is not in unix compatibility mode; otherwise 
     * unixProgres != -1.
     */
    int         unixProgress;            /* Progress indicator for restarting
                                            unix system calls. */

    /*
     *---------------------------------------------------------------------
     *
     *  Extra padding, so that we can add fields to this struct 
     *  without changing its size (useful for process migration).
     *
     *---------------------------------------------------------------------
     */

    int		extraField[5];		/* Extra fields for later use. */
    
} Proc_ControlBlock;

#define PROC_PCB_MAGIC_NUMBER	0x143a0891

/* 
 * Many routines expect to get a locked pcb handle.  This type is used so 
 * that we can get the compiler help verify that the pcb is locked.  The 
 * trick is that a (Proc_ControlBlock *) and a (Proc_LockedPCB *) are 
 * functionally equivalent, but the compiler will treat them as distinct 
 * types. 
 */

typedef struct Proc_LockedPCB {
    Proc_ControlBlock pcb;
} Proc_LockedPCB;



/*
 * Machine independent object file information.
 */
typedef struct Proc_ObjInfo {
    Address	codeLoadAddr;	/* Address in user memory to load code. */
    unsigned	codeFileOffset;	/* Offset in obj file to load code from.*/
    unsigned	codeSize;	/* Size of code segment. */
    Address	heapLoadAddr;	/* Address in user memory to load heap. */
    unsigned	heapFileOffset;	/* Offset in obj file to load initialized heap
				 * from . */
    unsigned	heapSize;	/* Size of heap segment. */
    Address	bssLoadAddr;	/* Address in user memory to load bss. */
    unsigned	bssSize;	/* Size of bss segment. */
    Address	entry;		/* Entry point to start execution. */
    Boolean	unixCompat;	/* True if running unix compat. mode */
} Proc_ObjInfo;

#endif /* _PROCTYPES */
