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
#include "timer.h"
#include "proc.h"
#include "mach.h"
#include "sync.h"
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

extern void			Sched_MakeReady();
extern void			Sched_StartUserProc();
extern void			Sched_StartKernProc();
extern void 			Sched_ContextSwitch();
extern void 			Sched_ContextSwitchInt();
extern void 			Sched_ForgetUsage();
extern void 			Sched_GatherProcessInfo();
extern void 			Sched_Init();
extern void 			Sched_TimeTicks();
extern void 			Sched_LockAndSwitch();
extern void 			Sched_MakeReady();
extern void 			Sched_InsertInQueue();
extern void 			Sched_PrintStat();
extern void 			Sched_StartProcess();
extern void			Sched_SetClearUsageFlag();
extern void			Sched_DumpReadyQueue();
extern ReturnStatus		Sched_StartProcessor();
extern ReturnStatus		Sched_IdleProcessor();
extern void			Sched_StartSchedStats();
extern void			Sched_StopSchedStats();

#endif /* _SCHED */
