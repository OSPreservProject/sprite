/* 
 * syncSleep.c --
 *
 *	Interface between the timer module and processes that wish to sleep
 * 	for a specified period of time.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "sync.h"
#include "proc.h"
#include "sched.h"
#include "sig.h"
#include "stdlib.h"
#include "timer.h"
#include "vmMach.h"

static Boolean WaitTimeSubr();



/*
 *----------------------------------------------------------------------
 *
 * Sync_WaitTime --
 *
 *	Sleep for the specified period of time.
 *
 * Results:
 *	TRUE if aborted because of a signal, FALSE otherwise.
 *
 * Side effects:
 *	Process is put to sleep.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sync_WaitTime(time)
    Time time;		/* Amount of time the process will sleep */
{
    Timer_Ticks ticks;
    Timer_Ticks wakeupTime;
    Timer_Ticks	currentTime;		

    /*
     * Convert the passed-in time to ticks format.
     */

    Timer_TimeToTicks(time, &ticks);
    Timer_GetCurrentTicks(&currentTime);
    Timer_AddTicks(currentTime, ticks, &wakeupTime);

    return(WaitTimeSubr(wakeupTime));
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_WaitTimeInTicks --
 *
 *	Sleep for the specified period of time. 
 *
 * Results:
 *	TRUE if aborted because of a signal, FALSE otherwise.
 *
 * Side effects:
 *	Process is put to sleep.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sync_WaitTimeInTicks(time)
    Timer_Ticks time;		/* Amount of time the process will sleep */
{
    Timer_Ticks	currentTime;		
    Timer_Ticks wakeupTime;

    Timer_GetCurrentTicks(&currentTime);
    Timer_AddTicks(currentTime, time, &wakeupTime);

    return(WaitTimeSubr(wakeupTime));
}


/*
 *----------------------------------------------------------------------
 *
 * Sync_WaitTimeInterval --
 *
 *	Sleep for the specified period of time.
 *
 * Results:
 *	TRUE if aborted because of a signal, FALSE otherwise.
 *
 * Side effects:
 *	Process is put to sleep.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sync_WaitTimeInterval(interval)
    unsigned int interval;	/* Amount of time the process will sleep */
{
    Timer_Ticks wakeupTime;
    Timer_Ticks	currentTime;		

    /*
     * Convert the passed-in time to ticks format.
     */

    Timer_GetCurrentTicks(&currentTime);
    Timer_AddIntervalToTicks(currentTime, interval, &wakeupTime);

    return(WaitTimeSubr(wakeupTime));
}


/*
 *----------------------------------------------------------------------
 *
 * WaitTimeSubr --
 *
 *	Sleep until the specified time.  This is done by
 *	atomically scheduling a call to Sync_WakeupProcess and causing
 *	the current process to wait on an event corresponding to its
 *	process control block pointer.
 *
 * Results:
 *	TRUE if aborted because of a signal, FALSE otherwise.
 *
 * Side effects:
 *	Process is put to sleep.
 *
 *----------------------------------------------------------------------
 */

static Boolean
WaitTimeSubr(wakeupTime)
    Timer_Ticks wakeupTime;		/* Time when process will wakeup */
{
    Proc_ControlBlock 	*procPtr;	
    Timer_QueueElement 	wakeupElement;
    Boolean 		wokeUp = FALSE;
    Timer_Ticks		currentTime;		
    Boolean		sigPending;

    
    procPtr = Proc_GetCurrentProc();
    wakeupElement.routine    = Sync_WakeupProcess; 
    wakeupElement.clientData = (ClientData) procPtr;
    wakeupElement.time       = wakeupTime;

    /*
     * Get the scheduler master lock here.  This is done to ensure that
     * the wakeup call cannot precede the process's going to sleep.
     * SyncEventWaitInt expects the sched_MutexPtr lock to be held upon entry.
     */

    MASTER_LOCK(sched_MutexPtr);
    Timer_ScheduleRoutine(&wakeupElement, FALSE);

    /*
     * Sleep until the designated time passes.  This is done in a loop,
     * since a process may be awakened by a spurious broadcast.
     */

    do {
	if (wokeUp) {
	    sync_Instrument.numSpuriousWakeups++;
#ifdef DEBUG
	    printf("Sync_WaitTime: woke up for wrong reason.\n");
#endif /* DEBUG */
	}
	sigPending = SyncEventWaitInt((unsigned int) procPtr, TRUE);
	wokeUp = TRUE;
	Timer_GetCurrentTicks(&currentTime);
    } while (Timer_TickLT(currentTime, wakeupTime) && !sigPending);

    Timer_DescheduleRoutine(&wakeupElement);
    MASTER_UNLOCK(sched_MutexPtr);

    return(sigPending);
}



/* STUB procedure */
ENTRY ReturnStatus
Sync_Sleep(time)
    Time time;				  /* time process will sleep */
{
    Timer_Ticks ticks;
    /*
     * Convert the passed-in time to ticks format.
     */
    Timer_TimeToTicks(time, &ticks);
    if (Sync_WaitTimeInTicks(ticks)) {
	return(GEN_ABORTED_BY_SIGNAL);
    } else {
	return(SUCCESS);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Sync_WakeupProcess --
 *
 *	Perform a broadcast to wake up a process after a timer event
 *	has gone off.  
 *
 *	This routine is equivalent to Sync_SlowBroadcast without the wait flag
 *	and with interrupts disabled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Make the process that was waiting for the timer event runnable.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
void
Sync_WakeupProcess(time, procAddress)
    Timer_Ticks time;		/* passed by Timer module but ignored */
    ClientData procAddress;	/* address of PCB of process to wake up */
{
    Sync_EventWakeup((unsigned int) procAddress);
}
