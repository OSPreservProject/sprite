/*
 * timerClock.c --
 *
 *	Kernel utility procedures to manipulate clocks values.
 *
 *  The routines in this module provide the interface between routines in 
 *  the human oriented time and the machine dependent representation of 
 *  time, the Timer_Ticks format. The Timer_Ticks format is used
 *  for a specific purpose: to make the operations associated with the
 *  callback timer and timer queue run fast. These operations include
 *  starting the timer, scheduling a routine and calling a routine at its
 *  scheduled time.  Unlike the Time format, which represents time in
 *  seconds and microseconds, the Timer_Ticks format represents time in a
 *  machine-dependent way. Timer_Ticks are defined in timerTicks.h in the 
 *  machine-dependent directorys. Example Timer_Ticks format are as follows:
 *  Sun-2: Timer_Ticks is a value based on the hardware's free-running 
 *  counter and the number of times it has wrapped around. 
 *  Sun-3: the hardware free-running counter format is easily converted
 *  to the Time format, so no distinction is made between Time and Timer_Ticks.
 *
 *  A time value in the Timer_Ticks format is a hardware-dependent 64-bit
 *  number that represents a specific or absolute point in time since some
 *  some event (on the Sun-2, since the system was booted).  A time value
 *  that is relative to an absolute time is called an interval.  By
 *  definition, an interval is a hardware-dependent unsigned 32-bit number.  
 *  The operations * and / can be used on intervals since they are integers.
 *
 *  There are several constraints imposed on the Timer_Ticks format to
 *  decrease complexity and overhead in using the format.  First, it can
 *  not be used to represent negative time values.  Second, the routines
 *  are not general. For example, there are no multiply and divide
 *  routines for Timer_Ticks values.  Full generality is obtained by using
 *  the Time module.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"
#include "sync.h"
#include "timer.h"
#include "timerInt.h"
#include "spriteTime.h"
#include "timerTick.h"
#include "machMon.h"

/*
 *  Universal Time is the number of seconds since 1/1/1970, Greenwich Time.
 *  The time of day is kept in Universal Time. We use the terms "time of
 *  day" and "universal time" interchangably, although the latter is
 *  probably more descriptive. Universal time is mainly used by user-level
 *  programs. "System time" is what the kernel typically uses to measure
 *  time.  System time is the amount of time that the machine has been up.
 *  It is usually maintained by a free-running counter and expressed in
 *  units of ticks. System time is a more useful abstraction since it
 *  is not reset, whereas universal time can be adjusted by a system call.
 *
 *  The term "time" is very overloaded in both the comments and the code.
 *  I've tried to make it as clear as possible which format a particular
 *  time variable is stored in.  Time variables with standard names are
 *  of type "Time".  Time variables with "Tk" appended to their names
 *  are of type "Timer_Ticks".  There may be exceptions to this naming
 *  scheme.
 *
 *  Universal time can be obtained through one of two routines.
 *  Timer_GetRealTimeOfDay is quite accurate but the routine is slow due
 *  to the conversion from internal Timer_Ticks format to Time format.
 *  The other value is timerTimeOfDay and is returned by
 *  Timer_GetTimeOfDay.  The value is updated at every request timer
 *  interrupt by adding the amount of time between interrupts and hence is
 *  very inexpensive to calculate.  Also, the resolution is limited to the
 *  time between interrupts. However, the value is not always accurate
 *  because a timer interrupt can be delayed, therefore it is a close
 *  approximation to the real time of day.  To keep the value roughly
 *  accurate, every 10 seconds timer_UniversalApprox is updated to the real
 *  time of day by a routine called from the timer queue.  The value
 *  of timer_UniversalApprox is guaranteed to be monotonically increasing
 *  between calls to Timer_SetTimeOfDay.
 *
 *  When the universal time is initially set with Timer_SetTimeOfDay, the
 *  current system time is recorded (in systemWhenUniversalSetTk) along with 
 *  the new value for the universal time (in universalWhenSetTk).
 *  This value for universal time is not incremented. When 
 *  Timer_GetTimeOfDay is called, the current universal time is calculated by 
 *  reading the current system time and subtracting from it the system time 
 *  when Timer_SetTimeOfDay was called. This difference is added to the 
 *  recorded universal time to give the current universal time.
 *  
 *  timerUniversalToLocalOffset is used to convert from universal time
 *  to local time. It is the number of minutes to add to universal time
 *  to compute the local time. For example, timerLocalOffset for the 
 *  Pacific time zone is -540 minutes. The local time of day is computed 
 *  by multiplying timerUniversalToLocalOffset by 60 and adding the result 
 *  to the universal time.
 *
 *  timerDSTAllowed is a flag to indicate if Daylight Savings Time is allowed.
 *  A few states, such as Arizona, do not have DST.
 *  (TRUE == DST is allowed, FALSE == DST is not allowed).
 */

static Timer_Ticks systemWhenUniversalSetTk;
static Timer_Ticks universalWhenSetTk;

Time   		timer_UniversalApprox;
int 		timerUniversalToLocalOffset;
Boolean 	timerDSTAllowed;

#ifdef ADJTIME
Time		timer_AdjustDelta;
unsigned	timer_TickAdjust;
int		timer_TickDelta;
#endif

/*
 * Semaphore protecting the above time of day variables.
 */

Sync_Semaphore	timer_ClockMutex;


/*
 * UpdateTimeOfDay() adjusts timerTimeOfDay to the real time of day.
 */

static void UpdateUniversalTimeApprox _ARGS_((Timer_Ticks timeTicks, 
				  ClientData  clientData));
static Timer_QueueElement      updateElement;



/*
 *----------------------------------------------------------------------
 *
 * TimerClock_Init --
 *
 *	Initializes the data structures necessary to manage the timer
 *	modules' time of day clock.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The system counter is initialized and started.
 *
 *----------------------------------------------------------------------
 */

void
TimerClock_Init()
{

    Time	universal;
    int		offset;
    Boolean	DST;

    Sync_SemInitDynamic(&timer_ClockMutex,"Timer:timer_ClockMutex");

    Timer_CounterInit();
#ifdef ADJTIME
    timer_AdjustDelta.seconds = timer_AdjustDelta.microseconds = 0;
    timer_TickAdjust = 10;
    timer_TickDelta = 0;
#endif
    universal.seconds = 0;
    universal.microseconds = 0;
    offset = 0;
    DST = TRUE;
    TimerHardwareUniversalTimeInit(&universal, &offset, &DST);
    TimerSetSoftwareUniversalTime(&universal, offset, DST);
    /*
     * Add the routine to fix the time of day to the timer queue.
     * The routine is called every 10 seconds.
     */

    updateElement.routine = UpdateUniversalTimeApprox;
    updateElement.interval = 10 * timer_IntOneSecond;
    Timer_ScheduleRoutine(&updateElement, TRUE);
}



/*
 *----------------------------------------------------------------------
 *
 *  Timer_GetRealTimeOfDay --
 *
 *	Retrieves an accurate value for the time of day. 
 *	This routine is much slower than Timer_GetTimeOfDay but
 *	returns a much more truthful value for the time of day.
 *
 *  Results:
 *	The time of day is returned.
 *
 *  Side Effects:
 *	Updates the global variables that stores the system up-time.
 *
 *----------------------------------------------------------------------
 */

void
Timer_GetRealTimeOfDay(timePtr, timerLocalOffsetPtr, DSTPtr)
    Time *timePtr;		/* Buffer to hold TOD. */
    int  *timerLocalOffsetPtr;	/* Optional buffer to hold local offset. */
    Boolean *DSTPtr;		/* Optional buffer to hold DST allowed flag. */
{
    Timer_Ticks	curSystemTk;	/* current system time */
    Timer_Ticks	diffTk;

    /*
     *  Get the current system time and subtract from it the system time
     *  when the universal time was last set. Add this difference to the value
     *  of the stored universal time to get the current universal time.
     */

    MASTER_LOCK(&timer_ClockMutex);

    Timer_GetCurrentTicks(&curSystemTk);

    Timer_SubtractTicks(curSystemTk, systemWhenUniversalSetTk, &diffTk);
    Timer_AddTicks(diffTk, universalWhenSetTk, &diffTk);
    Timer_TicksToTime(diffTk, timePtr);

    if (timerLocalOffsetPtr != (int *) NIL) {
	*timerLocalOffsetPtr = timerUniversalToLocalOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = timerDSTAllowed;
    }
    MASTER_UNLOCK(&timer_ClockMutex);

}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_GetRealTimeFromTicks --
 *
 *	Gives an accurate translation of ticks to time.  This routine
 *	returns an absolute time value, rather than the relative time
 *	value since booting returned by Timer_TicksToTime.  This routine
 *	is slower, though.
 *
 *  Results:
 *	The time of the tick value is returned.
 *
 *  Side Effects:
 *	Updates the global variables that stores the system up-time.
 *
 *----------------------------------------------------------------------
 */

void
Timer_GetRealTimeFromTicks(ticks, timePtr, timerLocalOffsetPtr, DSTPtr)
    Timer_Ticks	ticks;		/* Ticks value to convert to time. */
    Time *timePtr;		/* Buffer to hold time value. */
    int  *timerLocalOffsetPtr;	/* Optional buffer to hold local offset. */
    Boolean *DSTPtr;		/* Optional buffer to hold DST allowed flag. */
{
    Timer_Ticks	diffTk;

    /*
     * No masterlock, since we can be called from a call-back and get deadlock.
     */

    /*
     *  Get the tick value and subtract from it the system time
     *  when the universal time was last set. Add this difference to the value
     *  of the stored universal time to get the universal time at that tick
     *  value.
     */

    Timer_SubtractTicks(ticks, systemWhenUniversalSetTk, &diffTk);
    Timer_AddTicks(diffTk, universalWhenSetTk, &diffTk);
    Timer_TicksToTime(diffTk, timePtr);

    if (timerLocalOffsetPtr != (int *) NIL) {
	*timerLocalOffsetPtr = timerUniversalToLocalOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = timerDSTAllowed;
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_GetTimeOfDay--
 *
 *	Retrieves an approximate universal time. 
 *	The value is approximate because it is updated at every
 *	timer interrupt and timer interrupts may be delayed or dropped.
 *	For an accurate value, use Timer_GetRealTimeOfDay.
 *
 *	Though the time of day value may not be accurate, it is
 *	guaranteed to be monotonically increasing (i.e. it never goes
 *	backwards) between calls to Timer_SetTimeOfDay.
 *
 *  Results:
 *	The approximate time of day is returned.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Timer_GetTimeOfDay(timePtr, timerLocalOffsetPtr, DSTPtr)
    Time *timePtr;		/* Buffer to hold TOD. */
    int  *timerLocalOffsetPtr;	/* Optional buffer to hold local offset. */
    Boolean *DSTPtr;		/* Optional buffer to hold DST allowed flag. */
{


    MASTER_LOCK(&timer_ClockMutex);

    *timePtr = timer_UniversalApprox;

    if (timerLocalOffsetPtr != (int *) NIL) {
	*timerLocalOffsetPtr = timerUniversalToLocalOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = timerDSTAllowed;
    }


    MASTER_UNLOCK(&timer_ClockMutex);

}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_SetTimeOfDay --
 *
 *	Changes the universal time to a new value. This is done in
 *	both the software and hardware clocks (if the machine has one).
 *	IMPORTANT: we don't set the hardware clock because it will won't
 *	work on a ds3100. See the comment in TimerSetHardwareUniversalTime.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Universal time is changed.
 *
 *----------------------------------------------------------------------
 */

void
Timer_SetTimeOfDay(newUniversal, newLocalOffset, newDSTAllowed)
    Time newUniversal;		/* New value for time of day. */
    int  newLocalOffset;	/* New value for local offset. */
    Boolean newDSTAllowed;	/* New value for DST allowed flag. */
{
    MASTER_LOCK(&timer_ClockMutex);
    TimerSetSoftwareUniversalTime(&newUniversal, 
	newLocalOffset, newDSTAllowed);
#ifdef NOTDEF
    TimerSetHardwareUniversalTime(&newUniversal, newLocalOffset, 
	newDSTAllowed);
#endif
    MASTER_UNLOCK(&timer_ClockMutex);
}


/*
 *----------------------------------------------------------------------
 *
 *  TimerSetSoftwareUniversalTime --
 *
 *	Changes the universal time to a new value.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Updates the global variables that stores the universal time and the 
 *	system time when it was set.
 *
 *----------------------------------------------------------------------
 */

void
TimerSetSoftwareUniversalTime(newUniversal, newLocalOffset, newDSTAllowed)
    Time *newUniversal;		/* New value for time of day. */
    int  newLocalOffset;	/* New value for local offset. */
    Boolean newDSTAllowed;	/* New value for DST allowed flag. */
{

    /*
     *  Record when the universal time was changed by saving the current 
     *  system time.
     *  Also store the new universal time (it has to be converted to ticks),
     *  the new local offset and the DST flag.
     */



    timer_UniversalApprox = *newUniversal;

    Timer_GetCurrentTicks(&systemWhenUniversalSetTk);
    Timer_TimeToTicks(*newUniversal, &universalWhenSetTk);


    timerUniversalToLocalOffset = newLocalOffset;
    timerDSTAllowed = newDSTAllowed;


}

/*
 *----------------------------------------------------------------------
 *
 * UpdateUniversalTimeApprox --
 *
 *	Called from the timer queue to make timer_UniversalApprox close
 *	to the real current time..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	timerUTApprox is updated.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateUniversalTimeApprox(timeTicks, clientData)
    Timer_Ticks timeTicks;	/* Not used. */
    ClientData	clientData;	/* Not used. */
{
	/* 
	 * No need to get the timerClock Mutex lock because 
	 * Timer_GetRealTimeOfDay gets it for us.
	 */
    Timer_GetRealTimeOfDay(&timer_UniversalApprox, (int *) NIL, (int *) NIL);
    Timer_ScheduleRoutine(&updateElement, TRUE);
}

#ifdef ADJTIME
/*
 *----------------------------------------------------------------------
 *
 * Timer_AdjustTime --
 *
 *	Set a new time delta for adjusting the time, and return
 *	the old one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	timer_AdjustDelta and timer_TickDelta are updated.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Timer_AdjustTime(newDelta, oldDelta)
    Time	*newDelta;
    Time	*oldDelta;
{
    int negative;

    MASTER_LOCK(&timer_ClockMutex);
    if (oldDelta != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(Time),
		(Address) &timer_AdjustDelta, (Address) oldDelta) != SUCCESS) {
	    MASTER_UNLOCK(&timer_ClockMutex);
	    return SYS_ARG_NOACCESS;
	}
    }
    if (newDelta == USER_NIL) {
	MASTER_UNLOCK(&timer_ClockMutex);
	return oldDelta == USER_NIL ? SYS_ARG_NOACCESS : SUCCESS;
    } else if (Proc_ByteCopy(TRUE, sizeof(Time),
	    (Address) newDelta, (Address) &timer_AdjustDelta) != SUCCESS) {
	MASTER_UNLOCK(&timer_ClockMutex);
	return SYS_ARG_NOACCESS;
    }
    /* normalize */
    timer_AdjustDelta.seconds += timer_AdjustDelta.microseconds / ONE_SECOND;
    timer_AdjustDelta.microseconds %= ONE_SECOND;
    negative = timer_AdjustDelta.seconds < 0 ||
	(timer_AdjustDelta.seconds == 0 && timer_AdjustDelta.microseconds < 0);
    if (negative && (timer_AdjustDelta.microseconds > 0)) {
	timer_AdjustDelta.microseconds -= ONE_SECOND;
	++timer_AdjustDelta.seconds;
    }
    if (!negative && (timer_AdjustDelta.microseconds < 0)) {
	timer_AdjustDelta.microseconds += ONE_SECOND;
	--timer_AdjustDelta.seconds;
    }
    if (negative) {
	timer_TickDelta = -timer_TickAdjust;
    } else {
	timer_TickDelta = timer_TickAdjust;
    }
    MASTER_UNLOCK(&timer_ClockMutex);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Timer_GetParams --
 *
 *	Return the current tick adjustment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Timer_GetParams(tickadj)
    unsigned	*tickadj;
{
    MASTER_LOCK(&timer_ClockMutex);
    if (tickadj == USER_NIL || Proc_ByteCopy(FALSE, sizeof(unsigned),
	    (Address) &timer_TickAdjust, (Address) tickadj) != SUCCESS) {
	MASTER_UNLOCK(&timer_ClockMutex);
	return(SYS_ARG_NOACCESS);
    }
    MASTER_UNLOCK(&timer_ClockMutex);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Timer_SetParams --
 *
 *	Set the tick adjustment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	timer_TickAdjust and timer_TickDelta are updated.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Timer_SetParams(tickadj)
    unsigned	tickadj;
{
    MASTER_LOCK(&timer_ClockMutex);
    if (tickadj < TIMER_CALLBACK_INTERVAL_APPROX &&
	    tickadj > 0 && (ONE_SECOND % tickadj) == 0) {
	timer_TickAdjust = tickadj;
	if (timer_AdjustDelta.seconds < 0
	    || (timer_AdjustDelta.seconds == 0
		&& timer_AdjustDelta.microseconds < 0)) {
	    timer_TickDelta = -timer_TickAdjust;
	} else {
	    timer_TickDelta = timer_TickAdjust;
	}
	MASTER_UNLOCK(&timer_ClockMutex);
	return SUCCESS;
    } else {
	MASTER_UNLOCK(&timer_ClockMutex);
	return GEN_INVALID_ARG;
    }
}
#endif
