/* 
 * schedule.c --
 *
 *  	Routines to implement the fair share scheduler algorithm.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "sched.h"
#include "schedInt.h"
#include "proc.h"
#include "list.h"
#include "timer.h"
#include "sync.h"
#include "sys.h"
#include "dbg.h"
#include "mach.h"

/*
 *  The basic philosophy is that processes that have not executed
 *  as much as other processes deserve to be run first.  Thus we
 *  keep a smoothed average of recent CPU usage (the more recent the
 *  usage, the higher the weighting).  The process with the lowest
 *  recent usage gets highest scheduling priority.  The smoothed
 *  average is maintained by adding CPU usage as the process accumulates
 *  it, then periodically (once a second) reducing all the usages of
 *  all processes by a specific factor.  Thus, if a process stops using
 *  the CPU then its average will gradually decay to zero;  if a process
 *  becomes CPU-intensive, its average will gradually increase, up to
 *  a maximum value.  The controlling parameters are:
 *
 *  FORGET_INTERVAL -	How often to reduce everyone's usage.
 *  FORGET_MULTIPLY -
 *  FORGET_SHIFT -	These two factors determine how CPU usage decays:
 *			every second, everyone's CPU usage is multiplied
 *			by FORGET_MULTIPLY, then shifted right by
 *			FORGET_SHIFT.  Right now, the combined effect of
 *			these two is to "forget" 1/8th of the process's
 *			usage.
 */

#define FORGET_MULTIPLY		14
#define FORGET_SHIFT		4
#define FORGET_INTERVAL		timer_IntOneSecond

/*  
 *  The half-life of the average in seconds can be computed using this formula:
 *
 *        half-life  = ln(2) / ln(F)
 *
 *  where F = (FORGET_MULTIPLY)/(2**FORGET_SHIFT).  For the current settings
 *  the half-life is about 5.1 seconds.  This means that if a process
 *  suddenly stops executing, its usage will decay to half its early value
 *  in about 5 seconds.  The half-life gives an idea of how responsive the
 *  scheduler is to changes in process behavior.  If it responds too slowly,
 *  then a previously-idle process could become CPU-bound and monopolize the
 *  whole CPU for a long time until its usage rises.  If the half-life is
 *  too short, then an interactive process that does anything substantial
 *  (e.g. dragging a selection) will instantly lose its scheduling priority
 *  relative to other compute-bound processes.
 */
 
/*
 * The scheduler module mutex semaphore.  Used in sync module as well,
 * since synchronization involves mucking with the process queues.
 */
Sync_Semaphore sched_Mutex = SYNC_SEM_INIT_STATIC("sched_Mutex");
Sync_Semaphore *sched_MutexPtr = &sched_Mutex;

/*
 * GATHER_INTERVAL is the amount of time in milliseconds between calls to
 * Sched_GatherProcInfo when it is called by the timer module. 
 */
#define GATHER_INTERVAL 	TIMER_CALLBACK_INTERVAL
static unsigned int gatherInterval;

/*
 * Pre-computed ticks to add when timer goes off.
 */
static Timer_Ticks	gatherTicks;

/*
 * QuantumInterval is the interval a process is allowed to run before 
 * possibly being preempted. QUANTUM is the quantum length in milliseconds.
 * QuantumTicks is the quantum interval expressed as the number of calls 
 * to Sched_GatherProcessInfo before the quantum has expired.
 */
#define QUANTUM 100
static unsigned int quantumInterval;
static int quantumTicks;

/*
 * Flag to see if Sched_Init has been called.  Used by Sched_GatherProcessInfo
 * to know when things have been initialized. It's needed because GPI
 * is called from the timer module and possibly before Sched_Init has been
 * called.
 */
static Boolean init = FALSE;

/*
 * Global variable for the timer queue for Sched_ForgetUsage.
 */
static Timer_QueueElement forgetUsageElement;

/*
 * Structure for instrumentation.
 */
Sched_Instrument sched_Instrument;


/*
 * Forward Declarations.
 */
static Proc_ControlBlock *IdleLoop();
static void QuantumEnd();
static void RememberUsage();


/*
 * ----------------------------------------------------------------------------
 *
 * Sched_Init --
 *
 *      Initialize data structures and variables for the scheduler.
 *	Cause Sched_ForgetUsage to be called from timer callback queue.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Global variables are initialized.  Run queue is initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Sched_Init()
{
    quantumInterval = QUANTUM * timer_IntOneMillisecond;
#ifdef TESTING
    quantumInterval = timer_IntOneSecond;
    quantumInterval = timer_IntOneHour;
#endif /* TESTING */

    gatherInterval	= GATHER_INTERVAL * timer_IntOneMillisecond;
    Timer_AddIntervalToTicks(gatherTicks, gatherInterval, &gatherTicks);
    quantumTicks	= quantumInterval / gatherInterval;

    bzero((Address) &(sched_Instrument),sizeof(sched_Instrument));

    List_Init(schedReadyQueueHdrPtr);

    forgetUsageElement.routine		= Sched_ForgetUsage; 
    forgetUsageElement.clientData	= 0;
    forgetUsageElement.interval		= FORGET_INTERVAL;
    Timer_ScheduleRoutine(&forgetUsageElement, TRUE);

    init = TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * Sched_ForgetUsage --
 *
 *	Adjusts the priority for all user processes on the system.
 *  
 *	This routine is called at regular intervals by the 
 *	Timer module TimeOut routine.
 *
 *
 * Results:
 *	none.
 *
 * Side Effects:
 *	Priorities of user processes are modified.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Sched_ForgetUsage(time)
    Timer_Ticks time;	/* The absolute time when this routine is called. 
			 * (not used). */
{
    register Proc_ControlBlock *procPtr;
    register int i;

    /*
     *  Gain exclusive access to usage fields in the process table.
     */
     MASTER_LOCK(sched_MutexPtr);

    /*
     *  Loop through all the processes on the system and
     *  forget some of the CPU usage for them.
     */
    for (i = 0; i < proc_MaxNumProcesses; i++) {
	procPtr = proc_PCBTable[i];
	if (procPtr->state == PROC_UNUSED) {
	    continue;
	}
        procPtr->unweightedUsage = 
		(procPtr->unweightedUsage * FORGET_MULTIPLY) >> FORGET_SHIFT;

	procPtr->weightedUsage =
		(procPtr->weightedUsage * FORGET_MULTIPLY) >> FORGET_SHIFT;
    }

    /*
     *  Schedule this procedure to be called again later.
     */
    Timer_RescheduleRoutine(&forgetUsageElement, TRUE);

    MASTER_UNLOCK(sched_MutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 *  Sched_GatherProcessInfo --
 *
 *	This routine is called at every timer interrupt. It collects
 *	statistics about the running process such as the state of CPU and
 *	CPU usage. 
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Various statistics about the running process are collected in the
 *      process's control block.
 *
 *
 *----------------------------------------------------------------------
 */
void
Sched_GatherProcessInfo()
{
    register Proc_ControlBlock  *curProcPtr;
    register int		cpu;

    if (!init) {
	return;
    }

    MASTER_LOCK(sched_MutexPtr);

    /*
     *  Get a pointer to the current process from the array that keeps
     *  track of running processes on each processor.
     */
    for (cpu = 0; cpu < mach_NumProcessors; cpu++) {

	curProcPtr = proc_RunningProcesses[cpu];

	/*
	 * If no process is currently running on this processor, don't
	 * charge the usage to a particular process but keep track of it.
	 */
	if (curProcPtr == (Proc_ControlBlock *) NIL) {
	    Timer_AddTicks(sched_Instrument.noProcessRunning[cpu], gatherTicks,
		           &(sched_Instrument.noProcessRunning[cpu]));
	    continue;
	}

	/*
	 *  We want to gather statistics about how much CPU time is spent in
	 *  kernel and user states.  The processor state is determined by
	 *  calling a machine-dependent routine.
	 */
	if (Mach_ProcessorState(cpu) == MACH_KERNEL) {
	    Timer_AddTicks(curProcPtr->kernelCpuUsage.ticks, gatherTicks,
		           &(curProcPtr->kernelCpuUsage.ticks));
	} else {
	    Timer_AddTicks(curProcPtr->userCpuUsage.ticks, gatherTicks,
		           &(curProcPtr->userCpuUsage.ticks));
	}

	/*
	 *  Update the CPU usage for scheduling priority calculations
	 *  for the current process.
	 */
	curProcPtr->recentUsage += gatherInterval;

	/*
	 * See if the quantum has expired for the process.  It can go
	 * negative if the user process happened to be running in kernel mode
	 * when the quantum expired for the first time and the process has
	 * not reentered the kernel voluntarily.
	 */
	if ((curProcPtr->genFlags & PROC_USER) && 
	    (curProcPtr->billingRate != PROC_NO_INTR_PRIORITY)) {
	    if (curProcPtr->schedQuantumTicks != 0) {
		curProcPtr->schedQuantumTicks--;
	    }
	    if (curProcPtr->schedQuantumTicks == 0) {
		QuantumEnd(curProcPtr);
	    }
	}
    }

    MASTER_UNLOCK(sched_MutexPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sched_ContextSwitchInt --
 *
 *	Change to a new process.  Set the state of the current process
 *	to the state argument.
 *
 *	If no process is runnable, then loop with interrupts enabled and
 *	the master lock released until one is found.
 *
 *	The master lock is assumed to be held with sched_Mutex when
 *	this routine is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new process is made runnable.  Counters of context switches are
 *	incremented.
 *
 * ----------------------------------------------------------------------------
 */

void
Sched_ContextSwitchInt(state)
    register	Proc_State state;	/* New state of current process */
{
    register Proc_ControlBlock	*curProcPtr;  	/* PCB for currently runnning 
						 * process. */
    register Proc_ControlBlock	*newProcPtr;  	/* PCB for new process. */
    register int cpu;

    cpu = Mach_GetProcessorNumber();
    sched_Instrument.numContextSwitches[cpu]++;

    curProcPtr = Proc_GetCurrentProc();
    /*
     * If we have a context switch pending get rid of it.
     */
    curProcPtr->schedFlags &= ~SCHED_CONTEXT_SWITCH_PENDING;

    /*
     * Adjust scheduling priorities.
     */
    RememberUsage(curProcPtr);

    if (state == PROC_READY) {
	/*
	 * If the current process is PROC_READY, add it to the ready queue and
	 * get the next runnable process.  If that happens to be the current
	 */
	curProcPtr->numQuantumEnds++; 
	if (List_IsEmpty(schedReadyQueueHdrPtr)) {
	    curProcPtr->schedQuantumTicks = quantumTicks;
	    return;
	}

	newProcPtr = Sched_InsertInQueue(curProcPtr, TRUE);
	if (newProcPtr == curProcPtr) {
	    curProcPtr->schedQuantumTicks = quantumTicks;
	    return;
	} 
	curProcPtr->state = PROC_READY;
	/*
 	 * Don't run this process if another processor is already using
	 * its stack.
	 */
	if (newProcPtr->schedFlags & SCHED_STACK_IN_USE) {
		(void) Sched_InsertInQueue(newProcPtr, FALSE);
		newProcPtr = IdleLoop();
	} 
    } else {
	if (state == PROC_WAITING) {
	    curProcPtr->numWaitEvents++; 
	}
	curProcPtr->state = state;
	/*
	 * Drop into the idle loop and come out with a runnable process.
	 * This procedure exists to try and capture idle time when profiling.
	 */
	newProcPtr = IdleLoop();
    }


    /*
     * Set the state of the new process.  
     */
    newProcPtr->state = PROC_RUNNING;
    newProcPtr->processor = cpu;
    Proc_SetCurrentProc(newProcPtr);

    /*
     * Set up the quantum as the number of clocks ticks that this process 
     * is allowed to run berfore it is context-switched.
     * (This field is ignored for kernel processes and user processes with 
     * a billing rate of PROC_NO_INTR_PRIORITY, which allows them to run 
     * forever.)
     */
    newProcPtr->schedQuantumTicks = quantumTicks;

    /*
     * If the current process is continuing, then don't bother to 
     * to do full context switch.  
     */
    if (newProcPtr == curProcPtr) { 
	return;
    }

    sched_Instrument.numFullCS[cpu]++;

    /*
     * Perform the hardware context switch.  After switching, make
     * sure that there is a context for this process.
     */
    Mach_ContextSwitch(curProcPtr, newProcPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * RememberUsage --
 *
 *	Adjusts the weighted and unweighted CPU usages for a kernel or
 *	and user process. A process with the billingRate of 
 *	PROC_NO_INTR_PRIORITY does not get charged for weighted CPU usage,
 *	which is used in deciding priority in the run queue.
 *	
 *	This routine assumes the sched_Mutex master lock is held.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	CPU usages of the process are modified.
 *
 *----------------------------------------------------------------------
 */

static void
RememberUsage(curProcPtr)
    register Proc_ControlBlock *curProcPtr;	/* The process that will be 
						 * adjusted */
{
    register int billingRate = curProcPtr->billingRate;

    /*
     *  We want to calculate the process's CPU usage at this moment.
     *  There are 2 smoothed usage averages that we maintain: an
     *  unweighted value and a weighted value.  The weighted usage is used
     *  for calculating scheduling priority.  The unweighted usage keeps
     *  track of the real smoothed usage.
     */ 

    curProcPtr->unweightedUsage += curProcPtr->recentUsage;

    /*
     *  The billing rate basically specifies a process's scheduling 
     *  priority. It it used to modify the amount of the recent usage
     *  that gets added to the weighted usage.
     *
     *  If the billing rate equals the normal value then the recent usage
     *  is not multiplied or divided by any factor.  If the billing rate
     *  is greater than the normal value then only a faction of the recent
     *  usage is added to the weighted usage.  If the billing rate is less
     *  than the normal value then the recent usage is multiplied by a
     *  power of 2 before it is added to the weighted  usage.
     *
     *  A process with a billing rate of PROC_NO_INTR_PRIORITY does
     *  not get charged for CPU usage.
     */


    if (billingRate >= PROC_NORMAL_PRIORITY) {
	if (billingRate != PROC_NO_INTR_PRIORITY) {
	    curProcPtr->weightedUsage += curProcPtr->recentUsage >> billingRate;
	}
    } else {
	curProcPtr->weightedUsage += curProcPtr->recentUsage << -(billingRate);
    }

    /*
     *  Reset the recent usage back to zero.
     */

    curProcPtr->recentUsage = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * IdleLoop --
 *
 *	This fetches a runnable process from the ready queue and returns it.
 *	If none are available this goes into an idle loop, enabling and
 *	disabling interrupts, and waits for something to become runnable.
 *
 * Results:
 *	A pointer to the next process to run.
 *
 * Side effects:
 *	Momentarily enables interrupts.
 *
 *----------------------------------------------------------------------
 */

static Proc_ControlBlock *
IdleLoop()
{
    register Proc_ControlBlock	*procPtr;
    register Proc_ControlBlock	*lastProcPtr;
    register int cpu;
    register List_Links		*queuePtr;

    cpu = Mach_GetProcessorNumber();
    queuePtr = schedReadyQueueHdrPtr;
    /*
     * Mark in the process control block for the current process that it's 
     * stack is still in use. This prevents the scheduler from scheduling
     * this process (and stack) to another processor.  
     */
    lastProcPtr = Proc_GetCurrentProc();
    lastProcPtr->schedFlags |= SCHED_STACK_IN_USE;
    MASTER_UNLOCK(sched_MutexPtr);
    while (1) {
	Proc_SetCurrentProc((Proc_ControlBlock *) NIL);
	/*
	 * Wait for a process to become runnable.  
	 */
	if (List_IsEmpty(queuePtr) == FALSE) {
	    /*
	     * Looks like there might be something in the queue. We don't
	     * have sched_Mutex down at this point, so this is only a hint.
	     */
	    MASTER_LOCK(sched_MutexPtr);
	    /*
	     * Make sure queue is not empty.
	     */
	    if (List_IsEmpty(queuePtr) == FALSE) {
		/*
		 * There is a ready process. Take a peek at it to insure that
		 * we can execute it.  The only condition preventing a 
		 * processor from executing a process is if its stack is
		 * being used by another processor in the idle loop.
		 */
		procPtr = (Proc_ControlBlock *) List_First(queuePtr);
		if (!(procPtr->schedFlags & SCHED_STACK_IN_USE) ||
		           (procPtr == lastProcPtr)) {
		    lastProcPtr->schedFlags &= ~SCHED_STACK_IN_USE;
		    break; 
		}
	    }
	    MASTER_UNLOCK(sched_MutexPtr);
	}
	/*
	 * Count Idle ticks.  
	 */
	if (sched_Instrument.idleTicksLow[cpu] == (unsigned) 0xffffffff) {
	    sched_Instrument.idleTicksLow[cpu] = 0;
	    sched_Instrument.idleTicksOverflow[cpu]++;
	} else {
	    sched_Instrument.idleTicksLow[cpu]++;
	}
    }
    if (procPtr->state != PROC_READY) {
	/*
	 * Unlock sched_Mutex because panic tries to grab it somewhere.
	 */
	MASTER_UNLOCK(sched_MutexPtr);
	panic("Non-ready process found in ready queue.\n");
	MASTER_LOCK(sched_MutexPtr);
    }
    ((List_Links *)procPtr)->prevPtr->nextPtr =
					    ((List_Links *)procPtr)->nextPtr;
    ((List_Links *)procPtr)->nextPtr->prevPtr =
					    ((List_Links *)procPtr)->prevPtr;
/*
    List_Remove((List_Links *)procPtr);
*/
    sched_Instrument.numReadyProcesses -= 1;
    return(procPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Sched_TimeTicks --
 *
 *	Idle for a few seconds and count the ticks recorded in IdleLoop.
 *	For now, we only do this for one processor. All we're trying to get
 *	is a rough estimate of idleTicksPerSecond.
 *
 *      This procedure is called during boot. The results are pretty much
 *	meaningless if it is not.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Momentarily enables interrupts.
 *
 *----------------------------------------------------------------------
 */

void
Sched_TimeTicks()
{
    register int lowTicks;
    register int cpu;
    Time time;

    cpu = Mach_GetProcessorNumber(); 
    Time_Multiply(time_OneSecond, 5, &time);
    printf("Idling for 5 seconds...");
    lowTicks = sched_Instrument.idleTicksLow[cpu];
    (void) Sync_WaitTime(time);
    lowTicks = sched_Instrument.idleTicksLow[cpu] - lowTicks;
    printf(" %d ticks\n", lowTicks);
    sched_Instrument.idleTicksPerSecond = lowTicks / 5;
}


/*
 *----------------------------------------------------------------------
 *
 * QuantumEnd --
 *
 *	Called by Sched_GatherProcessInfo when a process's quantum has expired.
 *	A global flag is set to indicate that the current process should
 *	be involuntarily context switched at the next available moment.
 *	If the process is executing in kernel mode, then don't force a context
 *	switch now, but instead mark the process as having a context switch
 *	pending.
 *
 *	N.B. This routine assumes the sched mutex is already locked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A context switch is initiated.
 *
 *----------------------------------------------------------------------
 */

static void
QuantumEnd(procPtr)
    register	Proc_ControlBlock 	*procPtr;
{
    procPtr->schedFlags |= SCHED_CONTEXT_SWITCH_PENDING;
    procPtr->specialHandling = 1;
    if (procPtr->processor != Mach_GetProcessorNumber()) {
	/* 
	 * If the process whose quantum has ended is running on a different
	 * processor we need to poke the processor and force it into the
	 * kernel. On its way back to user mode the special handling flag
	 * will be checked and a context switch will occur (assuming that
	 * the offending process is still running).
	 */
	Mach_CheckSpecialHandling(procPtr->processor);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Sched_PrintStat --
 *
 *	Print the sched module statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Do the prints.
 *
 *----------------------------------------------------------------------
 */
void
Sched_PrintStat()
{
    Time  tmp;
    int   i;

    printf("Sched Statistics\n");
    for(i = 0; i < mach_NumProcessors;i++) {
	printf("Processor: %d\n",i);
	printf("numContextSwitches = %d\n",
	       sched_Instrument.numContextSwitches[i]);
	printf("numFullSwitches    = %d\n",
	       sched_Instrument.numFullCS[i]);
	printf("numInvoluntary     = %d\n",
	       sched_Instrument.numInvoluntarySwitches[i]);
	Timer_TicksToTime(sched_Instrument.noProcessRunning[i], &tmp);
	printf("Idle Time          = %d.%06d seconds\n", 
  	       tmp.seconds, tmp.microseconds);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sched_LockAndSwitch --
 *
 *	Acquires the Master Lock and performs a context switch.
 *	Called when a process's quantum has expired and a trace trap
 *	exception has arisen with the sched_ContextSwitchInProgress flag set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A context switch is performed.  The count of involuntary switches is
 *	incremented.
 *
 *----------------------------------------------------------------------
 */

void
Sched_LockAndSwitch()
{
    MASTER_LOCK(sched_MutexPtr);
    sched_Instrument.numInvoluntarySwitches[Mach_GetProcessorNumber()]++;
    Sched_ContextSwitchInt(PROC_READY);
    MASTER_UNLOCK(sched_MutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sched_ContextSwitch --
 *
 *	Acquires the Master Lock and performs a context switch.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A context switch is performed.
 *
 *----------------------------------------------------------------------
 */

void
Sched_ContextSwitch(state)
    Proc_State	state;
{
    MASTER_LOCK(sched_MutexPtr);
    Sched_ContextSwitchInt(state);
    MASTER_UNLOCK(sched_MutexPtr);
}



/*
 *----------------------------------------------------------------------
 *
 * Sched_StartKernProc --
 *
 *	Start a process by unlocking the master lock and calling the
 *	function whose address has been passed to us as an argument.
 *	If the function returns then exit.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The master lock is released.
 *
 *----------------------------------------------------------------------
 */
void
Sched_StartKernProc(func)
    void	(*func)();
{
    MASTER_UNLOCK(sched_MutexPtr);
    func();
    Proc_Exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * Sched_MakeReady --
 *
 *	Put the process on the ready queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	State of given process changed to ready.
 *
 *----------------------------------------------------------------------
 */

void
Sched_MakeReady(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    MASTER_LOCK(sched_MutexPtr);
    procPtr->state = PROC_READY;
    Sched_MoveInQueue(procPtr);
    MASTER_UNLOCK(sched_MutexPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Sched_StartUserProc --
 *
 *	Start a user process running.  This is the first thing that is
 * 	called when a newly created process begins execution.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Sched_StartUserProc(pc)
    Address	pc;	/* Program counter where process is to start
			 * executing. */
{
    register     	Proc_ControlBlock *procPtr;

    MASTER_UNLOCK(sched_MutexPtr);
    procPtr = Proc_GetCurrentProc();

#ifdef notdef
    Proc_Lock(procPtr);
    procPtr->genFlags |= PROC_DONT_MIGRATE;
    Proc_Unlock(procPtr);
#endif
    
    /*
     * Start the process running.  This does not return.
     */
    Mach_StartUserProc(procPtr, pc);
}


/*
 *----------------------------------------------------------------------
 *
 * Sched_DumpReadyQueue --
 *
 *	Print out the contents of the ready queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Output goes to the screen.
 *
 *----------------------------------------------------------------------
 */

void
Sched_DumpReadyQueue()
{
    List_Links *itemPtr;
    Proc_ControlBlock *snapshot[SCHED_MAX_DUMP_SIZE];
    int snapshotCnt;
    int overflow;
    int i;

    if (List_IsEmpty(schedReadyQueueHdrPtr)) {
	printf("\nReady queue is empty.\n");
    } else {
	printf("\n%8s %5s %10s %10s %8s %8s   %s\n",
	    "ID", "wtd", "user", "kernel", "event", "state", "name");
	overflow = FALSE;
	snapshotCnt = 0;
	MASTER_LOCK(sched_MutexPtr);
	LIST_FORALL(schedReadyQueueHdrPtr,itemPtr) {
	    if (snapshotCnt >= SCHED_MAX_DUMP_SIZE) {
		overflow = TRUE;
		break;
	    }
	    snapshot[snapshotCnt++] = (Proc_ControlBlock *) itemPtr;
	}
	MASTER_UNLOCK(sched_MutexPtr);
	for (i = 0; i <snapshotCnt; i++) {
	    Proc_DumpPCB(snapshot[i]);
	}
	if (overflow) {
	    printf("Ready queue too large to snapshot.\n");
	}
    }
}
