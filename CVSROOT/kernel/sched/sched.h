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

#include "timer.h"
#include "proc.h"

/*
 * Flags for the schedFlags in the proc table.
 *
 *	SCHED_CONTEXT_SWITCH_PENDING	A context switch came in while context
 *					switching was disabled.
 *	SCHED_CLEAR_USAGE		Clear usage information for this
 *					process when scheduling it.  Intended
 *					to be used by kernel worker processes.
 */

#define SCHED_CONTEXT_SWITCH_PENDING	0x1
#define	SCHED_CLEAR_USAGE		0x2	

typedef struct Sched_Instrument {
    int	numContextSwitches;		/* Total number of context switches. */
    int	numInvoluntarySwitches;		/* Only those due to end of quantum. */
    int numFullCS;			/* Number of switches that force the
					 * cur. process to be switched from. */
    Timer_Ticks noProcessRunning;	/* Amount of time w/o running proc.  */
    Time idleTime;			/* Converted value of noProcessRunning
					 * that is only computed when this
					 * struct is copied out to user space.*/
    unsigned int idleTicksLow;		/* Free running counter that is ++'d
					 * inside the idle loop.  It is used
					 * to measure CPU utilization */
    unsigned int idleTicksOverflow;	/* Make the counter into 64 bits */
    unsigned int idleTicksPerSecond;	/* Calibrated value */
    int numReadyProcesses;		/* Number of ready processes at time
					 * of call to copy information */
    Time noUserInput;			/* Time since last level-6 interrupt */
} Sched_Instrument;

/*
 * External declarations:
 */
    
extern int  sched_Mutex;		/* Mutual exclusion in scheduler */

extern int  sched_DoContextSwitch;	/* Flag to force a context switch. */

extern Sched_Instrument sched_Instrument;   /* Counters for instrumentation. */

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
extern void 			Sched_MoveInQueue();
extern Proc_ControlBlock	*Sched_InsertInQueue();
extern void 			Sched_PrintStat();
extern void 			Sched_StartProcess();
extern void			Sched_SetClearUsageFlag();

#endif /* _SCHED */
