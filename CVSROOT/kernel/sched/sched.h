/*
 * sched.h --
 *
 *	Declarations of procedures exported by the sched module.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * rcsid $Header$ SPRITE (Berkeley)
 */

#ifndef _SCHED
#define _SCHED

#ifdef KERNEL
#include <timer.h>
#include <proc.h>
#include <mach.h>
#include <sync.h>
#else
#include <kernel/timer.h>
#include <kernel/proc.h>
#include <kernel/mach.h>
#include <kernel/sync.h>
#endif

/*
 * Flags for the schedFlags in the proc table.
 *
 *	SCHED_CONTEXT_SWITCH_PENDING	A context switch came in while context
 *					switching was disabled.
 *	SCHED_CLEAR_USAGE		Clear usage information for this
 *					process when scheduling it.  Intended
 *					to be used by kernel worker processes.
 *	SCHED_STACK_IN_USE		The stack of this process is being
 *					used by a processor.  The processor 
 *					field of the Proc_ControlBlock 
 *					specifies what processor is using it. 
 */

#define SCHED_CONTEXT_SWITCH_PENDING	0x1
#define	SCHED_CLEAR_USAGE		0x2	
#define	SCHED_STACK_IN_USE		0x4

typedef struct Sched_Instrument {
    /*
     * Per processor numbers.
     */
    struct perProcessor {
	/* 
	 * Total number of context switches. 
	 */
	int	numContextSwitches;		
    
	/* 
	 * Only those due to end of quantum. 
	 */
	int	numInvoluntarySwitches;	
    
	/* 
	 * Number of switches that cause a different process to run.
	 */
	int numFullCS;			

	/* 
	 * Amount of time w/o running process.  
	 */
	Timer_Ticks noProcessRunning;	
    
	/* 
	 * Converted value of noProcessRunning that is only computed when
	 * this struct is copied out to user space.
	 */
	Time idleTime;			
    
	/* 
	 * Free running counter that is ++'d inside the idle loop.  It is used
	 * to measure CPU utilization.
	 */
	unsigned int idleTicksLow;		
	/* 
	 * Make the counter into 64 bits 
	 */
	unsigned int idleTicksOverflow;
    	unsigned int idleTicksPerSecond;	/* Calibrated value */

#if 	(MACH_MAX_NUM_PROCESSORS != 1) 
	/*
	 * Pad the structure to insure that two structure occur in the
	 * same cache block.  This is to prevent pinging of cache blocks
	 * between processor in a multiprocessor such as SPUR while 
	 * incrementing the IdleLoop counter.
	 */
	 Mach_CacheBlockSizeType	pad;
#endif
    } processor[MACH_MAX_NUM_PROCESSORS];

    int numReadyProcesses;		/* Number of ready processes at time
					 * of call to copy information */

    Time noUserInput;			/* Time since last level-6 interrupt */
} Sched_Instrument;


typedef struct {
    Proc_ControlBlock		*procPtr;
#if 	(MACH_MAX_NUM_PROCESSORS != 1) 
   Mach_CacheBlockSizeType	pad;
#endif
} Sched_OnDeck;

extern Sched_OnDeck	sched_OnDeck[MACH_MAX_NUM_PROCESSORS];

/*
 * External declarations:
 */
    
extern Sync_Semaphore sched_Mutex;	/* Mutual exclusion in scheduler */

extern Sync_Semaphore *sched_MutexPtr;
extern Sched_Instrument sched_Instrument;   /* Counters for instrumentation. */
extern int sched_Quantum;		/* Timer interrupts per quantum. */
extern Sched_OnDeck	sched_OnDeck[MACH_MAX_NUM_PROCESSORS];

extern void Sched_MakeReady _ARGS_((register Proc_ControlBlock *procPtr));
extern void Sched_StartUserProc _ARGS_((Address pc));
extern void Sched_StartKernProc _ARGS_((void (*func)()));
extern void Sched_ContextSwitch _ARGS_((Proc_State state));
extern void Sched_ContextSwitchInt _ARGS_((register Proc_State state));
extern void Sched_ForgetUsage _ARGS_((Timer_Ticks time, ClientData clientData));
extern void Sched_GatherProcessInfo _ARGS_((unsigned int interval));
extern void Sched_Init _ARGS_((void));
extern void Sched_TimeTicks _ARGS_((void));
extern void Sched_LockAndSwitch _ARGS_((void));
extern ReturnStatus Sched_StartProcessor _ARGS_((int pnum));
extern ReturnStatus Sched_IdleProcessor _ARGS_((int pnum));
extern void Sched_InsertInQueue _ARGS_((Proc_ControlBlock *procPtr, 
					Proc_ControlBlock **runPtrPtr));
extern void Sched_PrintStat _ARGS_((void));
extern void Sched_SetClearUsageFlag _ARGS_((void));
extern void Sched_DumpReadyQueue _ARGS_((void));
extern void Sched_StartSchedStats _ARGS_((void));
extern void Sched_StopSchedStats _ARGS_((void));

#endif /* _SCHED */
