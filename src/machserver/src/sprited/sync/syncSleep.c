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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/sprited/sync/RCS/syncSleep.c,v 1.2 91/11/14 10:03:05 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <sync.h>
#include <proc.h>
#include <sig.h>
#include <stdlib.h>
#include <timer.h>

static Sync_Lock sleepLock = Sync_LockInitStatic("sleepLock");
#define LOCKPTR	(&sleepLock)

static Boolean WaitTimeSubr _ARGS_((Timer_Ticks wakeupTime));



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
    Time interval;		/* Amount of time the process will sleep */
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

static ENTRY Boolean
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
     * Get the monitor lock, so that the process can't get woken up until 
     * it's gone to sleep.
     */
    LOCK_MONITOR;
    Timer_ScheduleRoutine(&wakeupElement, FALSE);

    /*
     * Sleep until the designated time passes.  This is done in a loop,
     * since a process may be awakened by a spurious broadcast.
     */

    do {
	if (wokeUp) {
#if 0
	    sync_Instrument[pnum].numSpuriousWakeups++;
#ifdef DEBUG
	    printf("Sync_WaitTime: woke up for wrong reason.\n");
#endif /* DEBUG */
#endif /* 0 */
	}
	sigPending = Sync_Wait(&procPtr->sleepCondition, TRUE);
	wokeUp = TRUE;
	Timer_GetCurrentTicks(&currentTime);
    } while (Timer_TickLT(currentTime, wakeupTime) && !sigPending);

    (void) Timer_DescheduleRoutine(&wakeupElement);
    UNLOCK_MONITOR;

    return(sigPending);
}


#if 0
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
#endif /* 0 */


/*
 *----------------------------------------------------------------------
 *
 * Sync_WakeupProcess --
 *
 *	Perform a broadcast to wake up a process after a timer event
 *	has gone off.  
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
void ENTRY
Sync_WakeupProcess(time, procAddress)
    Timer_Ticks time;		/* passed by Timer module but ignored */
    ClientData procAddress;	/* address of PCB of process to wake up */
{
    Proc_ControlBlock *procPtr = (Proc_ControlBlock *)procAddress;

    LOCK_MONITOR;
    Sync_Broadcast(&procPtr->sleepCondition);
    UNLOCK_MONITOR;
}
