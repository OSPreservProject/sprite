/*
 * proc.h --
 *
 *	User-visible declarations for processes.
 *
 * Copyright 1986, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/proc.h,v 1.8 92/06/10 15:17:31 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCUSER
#define _PROCUSER

#include <mach/thread_status.h>

/*
 * Process Termination Reason flags:
 *
 *   PROC_TERM_EXITED		- The process has called Proc_Exit.
 *   PROC_TERM_DETACHED		- The process has called Proc_Detach.
 *   PROC_TERM_SIGNALED		- The process has died because of a signal.
 *   PROC_TERM_DESTROYED	- The process has died because the internal
 *				  state of the process was found to be
 *				  invalid.
 *   PROC_TERM_SUSPENDED	- The process has been suspended.
 *   PROC_TERM_RESUMED		- The process has resumed execution as the
 *				  result of a resume signal.
 */

#define PROC_TERM_EXITED		1
#define PROC_TERM_DETACHED		2
#define PROC_TERM_SIGNALED		3
#define PROC_TERM_DESTROYED		4
#define	PROC_TERM_SUSPENDED		5
#define	PROC_TERM_RESUMED		6

/*
 * Reasons why a process was destroyed (PROC_TERM_DESTROYED):
 * 
 * PROC_BAD_STACK		- A process's user stack is invalid upon
 *				  return from a signal handler.
 * PROC_BAD_PSW 		- The processor status word that is to be
 *				  restored upon return from a signal handler
 *				  has the supervisor bit set.
 * PROC_VM_READ_ERROR		- The virtual memory system couldn't read from
 *				  the page server.
 * PROC_VM_WRITE_ERROR		- The virtual memory system couldn't write to
 *				  the page server.
 * PROC_EXEC_FAILED		- An exec call failed after the process's 
 * 				  state had been irretrievably munged.
 * PROC_MACH_FAILURE		- An operation on the process's task or 
 * 				  thread failed unexpectedly.
 */

#define	PROC_BAD_STACK			1
#define	PROC_BAD_PSW			2
#define	PROC_VM_READ_ERROR		3
#define	PROC_VM_WRITE_ERROR		4
#define PROC_EXEC_FAILED		5
#define PROC_MACH_FAILURE		6

#ifndef _ASM
/*
 *  Definition of a process ID.
 */

typedef unsigned int 	Proc_PID;



/*
 * Special values to indicate the pid of the current process, or the host on
 * which it is running, respectively.
 */

#define PROC_MY_PID	((Proc_PID) 0xffffffff)
#define PROC_MY_HOSTID	((unsigned int) 0xffffffff)

/*
 * Mask to extract process table index from pid.
 */
#define	PROC_INDEX_MASK		0x000000FF

/*
 * Convert a process id into a process table index.
 */
#define	Proc_PIDToIndex(pid) ((pid) & PROC_INDEX_MASK)

/*
 * Special parameter to Proc_Migrate to evict all processes from a
 * workstation.
 */

#define PROC_ALL_PROCESSES	((Proc_PID) 0)

/*
 * Special family value to indicate the process isn't in a family and 
 * a macro to see if the process is in a family.
 */

#define PROC_NO_FAMILY	(Proc_PID) -1
#define Proc_In_A_Family(familyID) ((familyID) != PROC_NO_FAMILY)


/*
 * PROC_SUPER_USER_ID is the user ID of the omnipotent super-user and 
 * PROC_NO_ID is used when specifying no id to the Proc_SetIDs call.
 */

#define PROC_SUPER_USER_ID  	0
#define PROC_NO_ID  		-1

/*
 * PROC_NO_INTR_PRIORITY is used to provide system processes 
 *  infinitely-high priority.
 */

#define PROC_MIN_PRIORITY	-2
#define PROC_MAX_PRIORITY	 2

#define PROC_NO_INTR_PRIORITY	 2
#define PROC_HIGH_PRIORITY	 1
#define PROC_NORMAL_PRIORITY	 0
#define PROC_LOW_PRIORITY	-1
#define PROC_VERY_LOW_PRIORITY	-2

/*
 *  Possible states for a process.
 */

typedef enum {
    PROC_UNUSED,	/* The process doesn't exist yet. */
    PROC_RUNNING,		/* unused; kept for compatibility */
    PROC_READY,		/* The process is ready to execute. */
    PROC_WAITING,	/* The process is waiting for an event to occur such
			 * as I/O completion or a mutex lock released. */
    PROC_EXITING,	/* The process has terminated and is on the 
			 * exiting list. */
    PROC_DEAD,		/* The process has been terminated is on the dead list*/
    PROC_MIGRATED,	/* The process is running on a remote machine. */
    PROC_NEW,		/* The process was just created. */
    PROC_SUSPENDED	/* The process is suspended. */
} Proc_State;

#define PROC_NUM_STATES	(PROC_SUSPENDED + 1) /* number of different 
					      * process states */

#endif /* _ASM */


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
 *  PROC_SINGLE_STEP_FLAG	- The process will have the trace bit set
 *				  before it runs.
 *  PROC_MIG_PENDING		- The process will be migrated when it
 *				  completes its next trap.
 *  PROC_DONT_MIGRATE		- The process should not be migrated yet, even
 *				  when it traps.
 *  PROC_FOREIGN		- The process has been migrated from another
 *				  workstation to this one.
 *  PROC_DYING			- The process is comitting hari-kari.  
 *  				  Implies that PROC_NO_MORE_REQUESTS is 
 *  				  also set.
 *  PROC_LOCKED			- This process is locked.
 *  PROC_NEEDS_WAKEUP		- The process's thread needs to be resumed 
 *  				  as part of the cleanup from a "system call".
 *  PROC_MIGRATING		- The process is in the middle of migrating
 *				  to another workstation.  This happens after
 *				  PROC_MIG_PENDING is set but before the
 *				  process's state becomes PROC_MIGRATED and
 *				  its PROC_MIGRATION_DONE flag is set.
 *  PROC_MIGRATION_DONE		- indicates successful completion of a
 *				  migration trap.
 *  PROC_ON_DEBUG_LIST		- The process is on the debug list.
 *  PROC_REMOTE_EXEC_PENDING	- The process should perform an exec as part
 *				  of migration.
 *  PROC_MIG_ERROR		- Indicates asynchronous error before
 *				  migrating process context switches.
 *  PROC_EVICTING		- Indicates process is being evicted
 *				  (for statistics gathering).
 *  PROC_KILLING		- Indicates we're trying to kill the process
 *				  but it's in the debugger.  This is a
 *				  big hack to get dbx to work.
 *  PROC_BEING_SERVED		- The process has made a Sprite 
 *  				  request (or generated an exception), 
 *  				  which is being serviced.
 *  PROC_NO_MORE_REQUESTS	- Future requests for the process 
 *  				  should be dropped on the floor.  If there
 *  				  is a pending request, it should try to
 *  				  bail out.  Note: the code that sets this 
 *  				  flag should also set the termReason, 
 *  				  termStatus, and termCode for the PCB.
 */

typedef int Proc_Flags;

#define PROC_KERNEL			0x000001
#define PROC_USER			0x000002
#define PROC_DEBUGGED			0x000004
#define PROC_DEBUG_ON_EXEC		0x000008
#define PROC_SINGLE_STEP_FLAG		0x000010
#define PROC_DEBUG_WAIT			0x000020
#define PROC_MIG_PENDING		0x000040
#define PROC_DONT_MIGRATE		0x000080
#define PROC_FOREIGN			0x000100
#define PROC_DYING			0x000200
#define PROC_LOCKED			0x000400
#define PROC_NEEDS_WAKEUP		0x000800 /* was PROC_NO_VM */
#define PROC_MIGRATING			0x001000
#define PROC_MIGRATION_DONE		0x002000
#define PROC_ON_DEBUG_LIST		0x004000
#define PROC_REMOTE_EXEC_PENDING	0x008000
#define PROC_MIG_ERROR			0x010000
#define PROC_EVICTING			0x020000
#define PROC_KILLING			0x040000
#define PROC_BEING_SERVED		0x080000
#define PROC_NO_MORE_REQUESTS		0x100000

/* 
 * The include's must come after the definition of Proc_State (and 
 * possibly some other stuff as well).  Blech.
 */

#ifndef _ASM
#include <sprite.h>
#include <spriteTime.h>
#if defined(SPRITED) || defined(KERNEL)
#include <user/sig.h>
#include <user/vmTypes.h>
#else
#include <sig.h>
#include <vmTypes.h>
#endif

#endif /* _ASM */

#ifndef _ASM


/*
 *  Resource usage summary for a process. 
 *  Used by Proc_Wait and Proc_GetResUsage.
 *
 *   Preliminary version: more fields will be added when needed.
 *
 *  Note: the cpu usage fields use the Time format. In the process
 *  control block, they are stored in the Timer_Ticks format.
 *  They are converted to Time format by the system calls that return
 *  resource usage info.
 */

typedef struct {
    Time kernelCpuUsage;	/* How much time has been spent in kernel mode*/
    Time userCpuUsage;		/* How much time has been spent in user mode. */

    Time childKernelCpuUsage;	/* Sum of time spent in kernel mode for 
				 * all terminated children. */
    Time childUserCpuUsage;	/* Sum of time been spent in user mode for
				 * all terminated children. */
    int	numQuantumEnds;		/* number of times the process was
				 * context switched due to a quantum end. */
    int numWaitEvents;		/* number of times the process was
     				 * context switched due to its waiting for
				 *  an event. */
} Proc_ResUsage;

/*
 *  Request values for use with Proc_Debug system call.
 */

typedef enum {
    PROC_GET_THIS_DEBUG,
    PROC_GET_NEXT_DEBUG,
    PROC_CONTINUE,
    PROC_SINGLE_STEP,
    PROC_GET_DBG_STATE,
    PROC_SET_DBG_STATE,
    PROC_READ,
    PROC_WRITE,
    PROC_DETACH_DEBUGGER
} Proc_DebugReq;

/*
 * Flags to Proc_Wait
 *
 *     	PROC_WAIT_BLOCK	-	Block if there if are no stopped or
 *				terminated processes.
 *	PROC_WAIT_FOR_SUSPEND - Return status of children that are suspended.
 *				If this option isn't specified and children
 *				are stopped then it is as though they are
 *				still running.
 */
#define	PROC_WAIT_BLOCK		0x1
#define	PROC_WAIT_FOR_SUSPEND	0x2

#define PROC_NUM_GENERAL_REGS 16

typedef struct {
    Proc_PID	processID;		/* Process ID of debuggee */
    int	termReason;			/* Reason why process has died or
					 * it has been detached. */
    int	termStatus;			/* Exit/detach status or signal number
					 * that caused the process to die. */
    int	termCode;			/* The code for the signal. */
    thread_state_data_t regState;	/* The register state of the process. */
    int	sigHoldMask;			/* Mask of signals to be held. */
    int	sigPendingMask;			/* Mask of pending signals. */
    int	sigActions[SIG_NUM_SIGNALS]; 	/* Array of the different types
					   of actions for signals. */
    int	sigMasks[SIG_NUM_SIGNALS]; 	/* Array of signal hold masks for 
					   signal handlers. */
    int	sigCodes[SIG_NUM_SIGNALS]; 	/* Array of signal handlers for 
					   signals. */

} Proc_DebugState;

/*
 * Structure that represents one environment variable.
 */

typedef struct {
    char *name;		/* Variable name. */
    char *value;	/* Value for variable. */
} Proc_EnvironVar;

/*
 * Process information. Add new fields to the end of this structure!
 */
typedef struct  {
    int		processor;	/* Processor number the process is running on
				 * or wants to run on if the processor is
				 * available.  */

    Proc_State	state;		/* Describes a process's current running state.
				 * >>> See Proc_State definitions above. */ 

    int		genFlags;	/* Flags to describe a processes overall state.
				 * >>> See definitions below */ 

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
     *
     *-----------------------------------------------------------------
     */
    
    int		 event;		 /* Address of the condition variable the 
				  * process is waiting for. */ 

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

    Time kernelCpuUsage;	/* How much time has been spent in kernel mode*/
    Time userCpuUsage;		/* How much time has been spent in user mode. */

    Time childKernelCpuUsage;	/* Sum of time spent in kernel mode for 
				 	 * all terminated children. */
    Time childUserCpuUsage;	/* Sum of time spent in user mode for
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
     *   Virtual Memory fields.
     *
     *-----------------------------------------------------------------
     */
    Vm_SegmentID		vmSegments[VM_NUM_SEGMENTS];


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
    /*
     *---------------------------------------------------------------------
     *
     * Data for process migration.
     *
     *---------------------------------------------------------------------
     */
    int		peerHostID;		 /* If on home node, ID of remote node.
					  * If on remote node, ID of home node.
					  * If not migrated, undefined. */
    Proc_PID	peerProcessID;		 /* If on remote note, process ID on
					  * home node, and vice-versa. */

    unsigned int machID;		/* for use with Mach ms, vminfo, etc */
} Proc_PCBInfo;

/*
 * Define the maximum length of the name and value of each enviroment
 * variable and the maximum size of the environment.
 */

#define	PROC_MAX_ENVIRON_NAME_LENGTH	4096
#define	PROC_MAX_ENVIRON_VALUE_LENGTH	4096
#define	PROC_MAX_ENVIRON_SIZE		100

/*
 * Define the maximum size of the first line of an interpreter file.
 */

#define PROC_MAX_INTERPRET_SIZE		80

/*
 * Definitions for the Proc_G/SetIntervalTimer system calls.
 *
 * Currently, only 1 type of timer is defined:
 *  PROC_TIMER_REAL -  time between intervals is real (a.k.a wall-clock) time.
 *
 * The values and the structure have the same values and layout as their 
 * 4.3BSD equivalents.
 */

#define PROC_TIMER_REAL		0
/*
 * not used yet.
#define PROC_TIMER_VIRTUAL	1
#define PROC_TIMER_PROFILE	2
*/

#define PROC_MAX_TIMER		PROC_TIMER_REAL

/* 
 * Warning: if you change this struct, you should also update the MIG 
 * definition of Proc_TimerInterval in spriteTypes.defs.
 */
typedef struct {
    Time	interval;	/* Amount of time between timer expirations. */
    Time	curValue;	/* Amount of time till the next expiration. */
} Proc_TimerInterval;

/* 
 * Size of the buffer containing arguments, to be passed back to users.  
 */

#define PROC_PCB_ARG_LENGTH 256

/*
 * The following structure is used to transfer fixed-length argument strings
 * from the kernel back to user space.  A typedef simplifies later
 * declarations (and may be the only way to do it?), since 
 *	char *argPtr[PROC_PCB_ARG_LENGTH]
 * would be an array of pointers to strings rather than an array of strings.
 */

typedef struct {
    char argString[PROC_PCB_ARG_LENGTH];
} Proc_PCBArgString;


/* 
 * These definitions match the MIG definitions for moving argument and
 * environment strings to the server for Exec.  The strings are passed in
 * an array of bytes, with an array of offsets telling where each string
 * begins.
 */

typedef int Proc_StringOffset;
typedef Proc_StringOffset *Proc_OffsetTable;
typedef char *Proc_Strings;


/*
 * Define the state of this machine w.r.t accepting migrated processes.
 * A machine must always be willing to accept its own processes if they
 * are migrated home.  Other than that, a host may allow migrations onto
 * it under various sets of criteria, and may allow migrations away from
 * it under similar sets of criteria.
 *
 *	PROC_MIG_IMPORT_NEVER		- never allow migrations to this host.
 *	PROC_MIG_IMPORT_ROOT 		- allow migrations to this host only
 *					  by root.
 *	PROC_MIG_IMPORT_ALL  		- allow migrations by anyone.
 *	PROC_MIG_IMPORT_ANYINPUT 	- don't check keyboard input when
 *					  determining availability.
 *	PROC_MIG_IMPORT_ANYLOAD  	- don't check load average when
 *					  determining availability.
 *	PROC_MIG_IMPORT_ALWAYS  	- don't check either.
 *	PROC_MIG_EXPORT_NEVER    	- never export migrations from this
 * 					  host.
 *	PROC_MIG_EXPORT_ROOT	        - allow only root to export.
 *	PROC_MIG_EXPORT_ALL	        - allow anyone to export.
 *
 * For example, a reasonable default for a file server might be to import
 * and export only for root; for a user's machine, it might be to allow
 * anyone to migrate; and for a compute server, it might never export
 * and import always regardless of load average or keyboard input.  (The
 * load average would not have to be exceptionally low to determine
 * availability; the host still would only be selected if the load average
 * were low enough to gain something by migrating to it.)
 */

#define PROC_MIG_IMPORT_NEVER 			 0
#define PROC_MIG_IMPORT_ROOT    	0x00000001
#define PROC_MIG_IMPORT_ALL     	0x00000003
#define PROC_MIG_IMPORT_ANYINPUT	0x00000010
#define PROC_MIG_IMPORT_ANYLOAD		0x00000020
#define PROC_MIG_IMPORT_ALWAYS  \
			(PROC_MIG_IMPORT_ANYINPUT | PROC_MIG_IMPORT_ANYLOAD)
#define PROC_MIG_EXPORT_NEVER			 0
#define PROC_MIG_EXPORT_ROOT		0x00010000
#define PROC_MIG_EXPORT_ALL		0x00030000

#define PROC_MIG_ALLOW_DEFAULT (PROC_MIG_IMPORT_ALL | PROC_MIG_EXPORT_ALL)

/*
 * Library call declarations.
 */

extern ReturnStatus Proc_SetExitHandler();
extern void	    Proc_Exit();

/*
 * System call declarations.
 */

extern ReturnStatus Proc_Fork();
extern ReturnStatus Proc_Detach();
extern ReturnStatus Proc_Wait();
extern ReturnStatus Proc_RawWait();
extern ReturnStatus Proc_ExecStub();
extern ReturnStatus Proc_ExecEnvStub();

extern ReturnStatus Proc_GetIDsStub();
extern ReturnStatus Proc_SetIDs();
extern ReturnStatus Proc_GetGroupIDs();
extern ReturnStatus Proc_SetGroupIDs();
extern ReturnStatus Proc_GetFamilyID();
extern ReturnStatus Proc_SetFamilyID();

extern ReturnStatus Proc_GetPCBInfo();
extern ReturnStatus Proc_GetResUsage();
extern ReturnStatus Proc_GetPriority();
extern ReturnStatus Proc_SetPriority();

extern ReturnStatus Proc_Debug();
extern ReturnStatus Proc_Profile();

extern ReturnStatus Proc_SetIntervalTimer();
extern ReturnStatus Proc_GetIntervalTimer();

extern ReturnStatus Proc_SetEnviron();
extern ReturnStatus Proc_UnsetEnviron();
extern ReturnStatus Proc_GetEnvironVar();
extern ReturnStatus Proc_GetEnvironRange();
extern ReturnStatus Proc_InstallEnviron();
extern ReturnStatus Proc_CopyEnviron();

extern ReturnStatus Proc_Migrate();

/* 
 * For initial debugging, have the server share a page of memory with 
 * all its clients.  This is the starting address of it.
 */
#define PROC_SHARED_REGION_START 0x1000000 
    
#endif /* _ASM */

#endif /* _PROCUSER */
