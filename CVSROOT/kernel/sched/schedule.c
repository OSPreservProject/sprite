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
#endif not lint

#include "sprite.h"
#include "sched.h"
#include "schedInt.h"
#include "proc.h"
#include "list.h"
#include "devTimer.h"
#include "timer.h"
#include "sync.h"
#include "vmMach.h"
#include "sys.h"
#include "byte.h"
#include "dbg.h"
#include "sunMon.h"
#include "machine.h"

/*
 *  The basic philosophy is that processes that have not executed
 *  as much as other processes deserve to be run first. As a process runs,
 *  its priority will drop, possibly to a level where another process
 *  with a better priority will preempt it.
 *
 *  The priority of a process is based on how much CPU time the process has
 *  obtained recently. A smoothed average of CPU usage is determined by 
 *  1) adding the CPU time when a process completes a quantum or goes to sleep 
 *  and 2) forgetting a portion of the smoothed average once a second.
 *  The formula is:
 *
 *  remembering: new average = 
 * 		 old average + ((CPU usage * REMEMBER_FACTOR)/DENOMINATOR)
 *
 *  forgetting:  new average = (old average * FORGET_FACTOR)/DENOMINATOR
 */
 
#define REMEMBER_FACTOR		3
#define DENOMINATOR		8
#define FORGET_FACTOR		(DENOMINATOR - REMEMBER_FACTOR)
#define FORGET_INTERVAL		timer_IntOneSecond

 
/*  
 *  The half-life of the average in seconds can be computed using this formula:
 *        half-life  = ln 2 / (REMEMBER_FACTOR/DENOMINATOR)   or
 *        half-life ~= (.69 * DENOMINATOR) / REMEMBER_FACTOR
 *
 *  When a process commences execution, its CPU average will be 0 (i.e.
 *  the highest priority) so it will run. If the process is compute-intensive, 
 *  the average will increase until it is preempted by a higher priority
 *  process. While the process is not running, its priority will increase
 *  because part of the CPU average is forgotten at regular intervals.
 *  If the process is I/O-intensive, it will never use much of the CPU, its
 *  usage will be low and its priority will therefore remain high.
 *
 *  The Sched_ForgetUsage routine adjusts scheduling priorities for all
 *  processes in the system once a second by forgiving part of the
 *  smoothed CPU average.  The recording of CPU usage is performed when
 *  the process experiences a quantum end or goes to sleep in Sync_Wait.
 */
 
/*
 * The scheduler module mutex semaphore.  Used in sync module as well,
 * since synchronization involves mucking with the process queues.
 */
int sched_Mutex = 0;
 
/*
 * GATHER_INTERVAL is the amount of time in milliseconds between calls to
 * Sched_GatherProcInfo when it is called by the timer module. 
 */
#define GATHER_INTERVAL 	DEV_CALLBACK_INTERVAL
static unsigned int gatherInterval;

/*
 * Pre-computed ticks to add when timer goes off.
 */
static Timer_Ticks	gatherTicks;
/*
 * It is possible that a process might run and not be charged for any
 * CPU usage. This happens when the process starts and stops between
 * calls to Sched_GatherProcesseInfo. To make sure a process is charged
 * for some CPU use, RememberUsage() uses the value noRecentUsageCharge 
 * when calculating weighted and unweigthed usage.
 */
static unsigned int noRecentUsageCharge;

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
 * Sched_DoContextSwitch is set to force an involuntary context switch.
 * The context switch occurs the next time the process traps.
 */
int sched_DoContextSwitch = 0;

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
#endif TESTING
    
    gatherInterval	= GATHER_INTERVAL * timer_IntOneMillisecond;
    Timer_AddIntervalToTicks(gatherTicks, gatherInterval, &gatherTicks);
    noRecentUsageCharge	= ((gatherInterval / 2) * REMEMBER_FACTOR) /DENOMINATOR;
    quantumTicks	= quantumInterval / gatherInterval;

    Byte_Zero(sizeof(sched_Instrument), (Address) &sched_Instrument);

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
    MASTER_LOCK(sched_Mutex);

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
		(procPtr->unweightedUsage * FORGET_FACTOR) / DENOMINATOR;

	procPtr->weightedUsage =
		(procPtr->weightedUsage * FORGET_FACTOR) / DENOMINATOR;
    }

    /*
     *  Schedule this procedure to be called again later.
     */
    Timer_RescheduleRoutine(&forgetUsageElement, TRUE);

    MASTER_UNLOCK(sched_Mutex);
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
 *  FIXME:
 *	Everything should understand multiprocessors: instrumentation, etc.
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

    MASTER_LOCK(sched_Mutex);

    /*
     *  Get a pointer to the current process from the array that keeps
     *  track of running processes on each processor.
     */
    for (cpu = 0; cpu < sys_NumProcessors; cpu++) {

	curProcPtr = Proc_GetCurrentProc(cpu);

	/*
	 * If no process is currently running on this processor, don't
	 * charge the usage to a particular process but keep track of it.
	 */
	if (curProcPtr == (Proc_ControlBlock *) NIL) {
	    Time_Add(sched_Instrument.noProcessRunning, gatherTicks,
		     &(sched_Instrument.noProcessRunning));
	    continue;
	}

	/*
	 *  We want to gather statistics about how much CPU time is spent in
	 *  kernel and user states.  The processor state is determined by
	 *  calling a machine-dependent routine.
	 */
	if (Sys_ProcessorState(cpu) == SYS_KERNEL) {
	    Time_Add(curProcPtr->kernelCpuUsage, gatherTicks,
		     &(curProcPtr->kernelCpuUsage));
	} else {
	    Time_Add(curProcPtr->userCpuUsage, gatherTicks,
		     &(curProcPtr->userCpuUsage));
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
	    if (curProcPtr->schedQuantumTicks > 0) {
		curProcPtr->schedQuantumTicks--;
	    }
	    if (curProcPtr->schedQuantumTicks == 0) {
		QuantumEnd(curProcPtr);
	    }
	}
    }

    MASTER_UNLOCK(sched_Mutex);
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

    sched_Instrument.numContextSwitches++;

    curProcPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
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
    } else {
	if (state == PROC_WAITING) {
	    curProcPtr->numWaitEvents++; 
	}
	curProcPtr->state = state;
	/*
	 * Drop into the idle loop and come out with a runnable process.
	 * This procedure exists to try and capture idle time when profiling.
	 */
	newProcPtr = IdleLoop(Sys_GetProcessorNumber());
    }


    /*
     * Set the state of the new process.  
     */
    newProcPtr->state = PROC_RUNNING;
    Proc_SetCurrentProc(Sys_GetProcessorNumber(), newProcPtr);

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

    sched_Instrument.numFullCS++;

    /*
     * Perform the hardware context switch.  After switching, make
     * sure that there is a context for this process.
     */
    dbgMaxStackAddr = newProcPtr->stackStart + mach_KernStackSize;
    VmMach_ContextSwitch(curProcPtr, newProcPtr);
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
    register unsigned int recentUsage;
    register int billingRate = curProcPtr->billingRate;

    /*
     *  We want to calculate the process's CPU usage at this moment.
     *  There are 2 smoothed usage averages that we maintain: an
     *  unweighted value and a weighted value.  The weighted usage is used
     *  for calculating scheduling priority.  The unweighted usage keeps
     *  track of the real smoothed usage.
     *
     *  The upper bound on the unweighted usage value is equal to the
     *  interval between calls to Sched_ForgetUsage.
     */ 

    recentUsage = (curProcPtr->recentUsage * REMEMBER_FACTOR) /DENOMINATOR;

    /*
     * Make sure the process gets charged for some CPU usage. We can't
     * determine exactly how much CPU usage the process has accumulated, so
     * charge it some value that's less than the full value (gatherInterval).
     */

    if (recentUsage == 0) {
	recentUsage = noRecentUsageCharge;
    }

    curProcPtr->unweightedUsage += recentUsage;

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
     *  multiple of 2 before it is added to the weighted  usage.
     *
     *  A process with a billing rate of PROC_NO_INTR_PRIORITY does
     *  not get charged for CPU usage.
     */


    if (billingRate >= PROC_NORMAL_PRIORITY) {
	if (billingRate != PROC_NO_INTR_PRIORITY) {
	    curProcPtr->weightedUsage += recentUsage >> billingRate;
	}
    } else {
	curProcPtr->weightedUsage += recentUsage << -(billingRate);
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
IdleLoop(myProcessor)
    int myProcessor;
{
    register Proc_ControlBlock	*procPtr;
    register List_Links		*queuePtr;

    queuePtr = schedReadyQueueHdrPtr;
    while (List_IsEmpty(queuePtr)) {
	/*
	 * Wait for a process to become runnable.  Turn on interrupts, then
	 * turn off interrupts again and see if someone became runnable.
	 */
	Proc_SetCurrentProc(myProcessor, (Proc_ControlBlock *) NIL);
	/*
	 * Count Idle ticks.  This is uni-processor code.
	 */
	if (sched_Instrument.idleTicksLow == (unsigned) 0xffffffff) {
	    sched_Instrument.idleTicksLow = 0;
	    sched_Instrument.idleTicksOverflow++;
	} else {
	    sched_Instrument.idleTicksLow++;
	}
	MASTER_UNLOCK(sched_Mutex);
	MASTER_LOCK(sched_Mutex);
    }
    procPtr = (Proc_ControlBlock *) List_First(queuePtr);
    if (procPtr->state != PROC_READY) {
	Sys_Panic(SYS_FATAL, "Non-ready process found in ready queue.\n");
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
 *
 * Results:
 *	A pointer to the next process to run.
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
    Time time;
    Time_Multiply(time_OneSecond, 5, &time);
    Sys_Printf("Idling for 5 seconds...");
    lowTicks = sched_Instrument.idleTicksLow;
    Sync_WaitTime(time);
    lowTicks = sched_Instrument.idleTicksLow - lowTicks;
    Sys_Printf(" %d ticks\n", lowTicks);
    sched_Instrument.idleTicksPerSecond = (int)((double)lowTicks / 5.);
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
    if (!sys_KernelMode) {
	sched_DoContextSwitch = TRUE;
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
    Sys_Printf("Sched Statistics\n");
    Sys_Printf("numContextSwitches = %d\n",sched_Instrument.numContextSwitches);
    Sys_Printf("numFullSwitches    = %d\n",sched_Instrument.numFullCS);
    Sys_Printf("numInvoluntary     = %d\n",
				      sched_Instrument.numInvoluntarySwitches);
/*    Sys_Printf("numIdles           = %d\n",sched_Instrument.numIdles); */
    Timer_TicksToTime(sched_Instrument.noProcessRunning, &tmp);
    Sys_Printf("Idle Time          = %d.%06d seconds\n", 
				      tmp.seconds, tmp.microseconds);
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
    MASTER_LOCK(sched_Mutex);
    sched_Instrument.numInvoluntarySwitches++;
    Sched_ContextSwitchInt(PROC_READY);
    MASTER_UNLOCK(sched_Mutex);
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
    MASTER_LOCK(sched_Mutex);
    Sched_ContextSwitchInt(state);
    MASTER_UNLOCK(sched_Mutex);
}



/*
 *----------------------------------------------------------------------
 *
 * Sched_StartProcess --
 *
 *	Start a process by unlocking the master lock and returning.  The
 *	stack is arranged so that this procedure will return into the
 *	procedure that is to be invoked.
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
Sched_StartProcess()
{
    MASTER_UNLOCK(sched_Mutex);
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
    MASTER_LOCK(sched_Mutex);
    procPtr->state = PROC_READY;
    Sched_MoveInQueue(procPtr);
    MASTER_UNLOCK(sched_Mutex);
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
Sched_StartUserProc()
{
    register     	Proc_ControlBlock *procPtr;

    MASTER_UNLOCK(sched_Mutex);
    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());

    Proc_Lock(procPtr);
    procPtr->genFlags |= PROC_DONT_MIGRATE;
    Proc_Unlock(procPtr);
    
    /*
     * Disable interrupts.  Note we don't use the macro DISABLE_INTR because
     * there is an implicit enable interrupt on return to user space.
     */
    Sys_DisableIntr();

    /*
     * We need the user proc to return the return code that indicates that
     * this is the child and not the parent.
     */
    procPtr->genRegs[D0] = PROC_CHILD_PROC;

    /*
     * Start the process running.  This does not return.
     */
    Proc_RunUserProc(procPtr->genRegs, procPtr->progCounter, Proc_Exit,
		     procPtr->stackStart + mach_ExecStackOffset);
}
