/* 
 * procTimer.c --
 *
 *	Routines to manipulate the interval timers of a process.
 *
 * Copyright 1987, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "proc.h"
#include "timer.h"
#include "sys.h"
#include "sig.h"
#include "stdlib.h"

/*
 * Information about the state of an interval timer for a process.
 * There are several types of interval timers and the info for all of
 * them are kept in an array pointed to by the PCB entry.
 */
typedef struct ProcIntTimerInfo {
    Timer_Ticks	expire;		/* When the timer is to expire. */
    Timer_Ticks	interval;	/* Amount of time between expirations.
				 * If zero, then the timer is not restarted 
				 * after the first expiration. */
    ClientData	token;		/* Token returned by Proc_CallFuncAbsTime. 
				 * Used by the signal-sending func to check if
				 * a timer has been cancelled. */
} ProcIntTimerInfo;


static ReturnStatus	GetCurrentTimer();
static void		SendTimerSigFunc();



/*
 *----------------------------------------------------------------------
 *
 * Proc_GetIntervalTimer --
 *
 *	Retrieves the current value of the interval timer. If the timer
 *	is not set, zero time values are returned.
 *	
 *
 * Results:
 *	SUCCESS			- the timer value was returned.
 *	GEN_INVALID_ARG		- unknown timer type.
 *	SYS_ARG_NOACCESS	- the timer value could not be accessed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetIntervalTimer(timerType, userTimerPtr)
    int			timerType;	/* What type of timer: one of 
					 * PROC_TIMER_REAL, PROC_TIMER_VIRTUAL,
					 * PROC_TIMER_PROFILE. */
    Proc_TimerInterval	*userTimerPtr;	/* Buffer to store the current value 
					 * of the interval timer. */
{
    ReturnStatus status;
    Proc_ControlBlock	*procPtr;

    if (timerType < 0 || timerType > PROC_MAX_TIMER) {
	return(GEN_INVALID_ARG);
    }

    if (userTimerPtr == USER_NIL) {
	return(SYS_ARG_NOACCESS);
    }

    procPtr = Proc_GetEffectiveProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_GetIntervalTime: procPtr == NIL\n");
    }
    Proc_Lock(procPtr);

    status = GetCurrentTimer(procPtr, timerType, userTimerPtr);

    Proc_Unlock(procPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * GetCurrentTimer --
 *
 *	An internal routine to get the current value of an interval timer
 *	and copy it to the user's address space.
 *
 *	Note: this routine assumes the process's proc table entry is locked.
 *
 * Results:
 *	SUCCESS			- the timer value was returned.
 *	SYS_ARG_NOACCESS	- the timer value could not be accessed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
GetCurrentTimer(procPtr, timerType, userTimerPtr)
    Proc_ControlBlock	*procPtr;	/* Process to get the timer value 
					 * from. */
    int			timerType;	/* What type of timer: one of 
					 * PROC_TIMER_REAL, PROC_TIMER_VIRTUAL,
					 * PROC_TIMER_PROFILE. */
    Proc_TimerInterval	*userTimerPtr;	/* Buffer to store the current value 
					 * of the interval timer. */
{
    register ProcIntTimerInfo	*timerPtr;
    Proc_TimerInterval	timer;

    timerPtr = &procPtr->timerArray[timerType];
    if ((procPtr->timerArray == (ProcIntTimerInfo  *) NIL) ||
	(timerPtr->token == (ClientData) NIL)) {

	/*
	 * No timer is scheduled. Just return zero values.
	 */
	timer.interval.seconds = 0;
	timer.interval.microseconds = 0;
	timer.curValue.seconds = 0;
	timer.curValue.microseconds = 0;
    } else {
	Timer_Ticks temp;

	Timer_TicksToTime(timerPtr->interval, &timer.interval);

	/*
	 * Get the amount of time remaining before the timer's expiration.
	 */
	Timer_GetCurrentTicks(&temp);
	Timer_SubtractTicks(timerPtr->expire, temp, &temp);
	Timer_TicksToTime(temp, &timer.curValue);
    }

    if (Proc_ByteCopy(FALSE, sizeof(timer), 
	    (Address) &timer, (Address) userTimerPtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_SetIntervalTimer --
 *
 *	Start or cancel an interval timer for a process.
 *
 * Results:
 *	SUCCESS			- the timer was started or stopped.
 *	GEN_INVALID_ARG		- unknown timer type or invalid time value.
 *	SYS_ARG_NOACCESS	- a timer value could not be accessed.
 *
 * Side effects:
 *	A CallFunc process might be scheduled. The process's PCB entry
 *	is updated.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_SetIntervalTimer(timerType, newTimerPtr, oldTimerPtr)
    int			timerType;	/* What type of timer: one of 
					 * PROC_TIMER_REAL, PROC_TIMER_VIRTUAL,
					 * PROC_TIMER_PROFILE. */
    Proc_TimerInterval	*newTimerPtr;	/* Buffer that holds a new value for
					 * the interval timer. */
    Proc_TimerInterval	*oldTimerPtr;	/* Buffer to hold the former value of
					 * the timer. */
{
    register ProcIntTimerInfo	*timerPtr;
    register Proc_ControlBlock	*procPtr;
    Proc_TimerInterval	newTimer;

    if (timerType < 0 || timerType > PROC_MAX_TIMER) {
	return(GEN_INVALID_ARG);
    }

    procPtr = Proc_GetEffectiveProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_SetIntervalTime: procPtr == NIL\n");
    }
    Proc_Lock(procPtr);

    if (procPtr->timerArray == (ProcIntTimerInfo  *) NIL) {
	int j;

	/*
	 * The table hasn't been initialized yet. Allocate enough entries
	 * for all the timers. The memory won't be deallocated when the
	 * process dies so it can be reused by the next process using the
	 * PCB entry.
	 */
	procPtr->timerArray = (ProcIntTimerInfo  *)
	    malloc(sizeof(ProcIntTimerInfo) * (PROC_MAX_TIMER +1));

	for (j = 0; j <= PROC_MAX_TIMER; j++) {
	    procPtr->timerArray[j].token = (ClientData) NIL;
	}
    }
    timerPtr = &procPtr->timerArray[timerType];

    /*
     * Return the current value if the user wants it.
     */
    if (oldTimerPtr != USER_NIL) {
	if (GetCurrentTimer(procPtr, timerType, oldTimerPtr) != SUCCESS) {
	    Proc_Unlock(procPtr);
	    return(SYS_ARG_NOACCESS);
	}
    }

    /*
     * Copy the new timer value from user space.
     */
    if (Proc_ByteCopy(TRUE, sizeof(newTimer), 
	    (Address) newTimerPtr, (Address) &newTimer) != SUCCESS) {
	Proc_Unlock(procPtr);
	return(SYS_ARG_NOACCESS);
    }

    if ((newTimer.curValue.seconds == 0) && 
	(newTimer.curValue.microseconds == 0)) {

	/*
	 * The user wants to cancel the timer.  Invalidate the token for
	 * the existing expiration routine.  The routine, when called,
	 * will match its token against this one and it will discover the
	 * mismatch so it will do nothing.
	 */
	timerPtr->token = (ClientData) NIL;

    } else {
	Timer_Ticks curTime;

	/*
	 * Make sure the times are valid and within the clock's resolution.
	 */
	if ((newTimer.curValue.seconds < 0) || 
	    (newTimer.curValue.microseconds < 0) ||
	    (newTimer.curValue.microseconds > ONE_SECOND) ||
	    (newTimer.interval.seconds < 0) || 
	    (newTimer.interval.microseconds < 0) ||
	    (newTimer.interval.microseconds > ONE_SECOND)) {

	    Proc_Unlock(procPtr);
	    return(GEN_INVALID_ARG);
	}
	if ((newTimer.curValue.seconds == 0) && 
	    (newTimer.curValue.microseconds < TIMER_CALLBACK_INTERVAL)) {
	    newTimer.curValue.microseconds = TIMER_CALLBACK_INTERVAL;
	}
	if ((newTimer.interval.seconds == 0) && 
	    (newTimer.interval.microseconds > 0) &&
	    (newTimer.interval.microseconds < TIMER_CALLBACK_INTERVAL)) {
	    newTimer.interval.microseconds = TIMER_CALLBACK_INTERVAL;
	}

	Timer_TimeToTicks(newTimer.interval, &timerPtr->interval);
	Timer_TimeToTicks(newTimer.curValue, &timerPtr->expire);

	Timer_GetCurrentTicks(&curTime);
	Timer_AddTicks(curTime, timerPtr->expire, &timerPtr->expire);

	/*
	 * Setting the token implicitly cancels a previous expiration
	 * routine's callback.
	 */
	timerPtr->token = Proc_CallFuncAbsTime(SendTimerSigFunc, 
			(ClientData) procPtr->processID, timerPtr->expire);
    }

    Proc_Unlock(procPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * SendTimerSigFunc --
 *
 *	Called when one of a process's interval timers has expired. This
 *	routine sends a SIG_TIMER signal to process (the signal subcode is
 *	the timer type, as defined in user/proc.h).
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A signal is sent to the process. A call-func process may be 
 *	scheduled to send a signal in the future.
 *
 *----------------------------------------------------------------------
 */

static void
SendTimerSigFunc(data, infoPtr)
    ClientData		data;		/* Really the ID of the process that
					 * should get the signal. */
    Proc_CallInfo	*infoPtr;	/* Used to compare the token in the
					 * PCB entry to make sure the signal
					 * is wanted. */
{
    Proc_ControlBlock	*procPtr;
    register ProcIntTimerInfo	*timerPtr;

    /*
     * If the process has died, the procPtr will NIL. 
     */

    procPtr = Proc_LockPID((Proc_PID) data);
    if (procPtr != (Proc_ControlBlock *) NIL) {
	int i;

	/*
	 * Scan all the timer state info to see which timer expired.
	 * If our token matches the one in the timer table entry, then
	 * send the signal (the subcode is the timer type). If there's no 
	 * match after scanning all the timers, then the user cancelled the
	 * a timer so there's nothing to do.
	 */
	for (i = 0; i <= PROC_MAX_TIMER; i++) {

	    if (procPtr->timerArray == (ProcIntTimerInfo  *) NIL) {
		/*
		 * This should not happen: why did we get scheduled if
		 * there aren't any timers?
		 */
		panic("SendTimerSigFunc: null timer table!\n");
		break;
	    }

	    timerPtr = &procPtr->timerArray[i];
	    if (timerPtr->token == infoPtr->token) {
		(void) Sig_SendProc(procPtr, SIG_TIMER, i);

		/*
		 * See if the signal is supposed to be repeated in the future.
		 */
		if (Timer_TickEQ(timerPtr->interval, timer_TicksZeroSeconds)){
		    /*
		     * Nope -- all done.
		     */
		    timerPtr->token = (ClientData) NIL;
		} else {
		    /*
		     * A signal is wanted in "interval" seconds from now.
		     * Add the interval to the expiration time instead of
		     * the current time to prevent drift.
		     */

		    Timer_AddTicks(timerPtr->interval, timerPtr->expire, 
				    &timerPtr->expire);
		    timerPtr->token = Proc_CallFuncAbsTime(SendTimerSigFunc,
					data, timerPtr->expire);
		}
		break;
	    }
	}
	Proc_Unlock(procPtr);
    }
}
