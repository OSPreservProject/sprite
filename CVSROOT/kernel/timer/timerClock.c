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
#include "sync.h"
#include "timer.h"
#include "timerInt.h"
#include "spriteTime.h"
#include "timerTick.h"

/*
 *  The time of day (abbreviated TOD, also called Universal Time
 *  or Greenwich Time) is the number of seconds since 1/1/1970.
 *  It is updated whenever Timer_{S,G}etTimeOfDay is called.
 *
 *  The TOD time is maintained for human consumption; it is not used by
 *  the kernel for internal time.  Two values are maintained because the TOD
 *  is obtained by an RPC call to the file server. The RPC module relies on
 *  the Sync and Sched modules, which rely on the Timer module.  If the
 *  current time is changed to TOD once the timer queue has been
 *  initialized, then RPCs will hang for a very long time (i.e.  
 *  1986 - 1970 = 16 years).
 *  
 *  Two TOD values are available. The value returned by
 *  Timer_GetRealTimeOfDay is quite accurate but the routine is slow due
 *  to the conversion from internal Timer_Ticks format to Time format.
 *  The other value is timerTimeOfDay and is returned by
 *  Timer_GetTimeOfDay.  The value is updated at every request timer
 *  interrupt by adding the amount of time between interrupts and hence is
 *  very inexpensive to calculate.  Also, the resolution is limited to the
 *  time between interrupts. However, the value is not always accurate
 *  because a timer interrupt can be delayed, therefore it is a close
 *  approximation to the real time of day.  To keep the value roughly
 *  accurate, every 10 seconds timerTimeOfDay is updated to the real
 *  time of day by a routine called from the timer queue.  The value
 *  of timerTimeOfDay is guaranteed to be monotonically increasing
 *  between calls to Timer_SetTimeOfDay.
 *
 *  When the time of day is initially set with Timer_SetTimeOfDay, the
 *  current system up-time is recorded (in timeWhenTODSet) along with 
 *  the new value for the time of day (in timeOfDay).  When 
 *  Timer_GetTimeOfDay is called, the current TOD is calculated by 
 *  reading the current system up-time and subtracting from it the time 
 *  when Timer_SetTimeOfDay was called. This difference is added to the 
 *  recorded TOD to give the current TOD.
 *  
 *  LocalOffset is used to convert timeOfDay from Universal Time
 *  to local time. It is the number of minutes to add to Universal Time
 *  to compute the local time. For example, localOffset for the Pacific time 
 *  zone is -540 minutes. The local time of day is computed by multiplying
 *  localOffset by 60 and adding the result added to timeOfDay.
 *
 *  DSTAllowed is a flag to indicate if Daylight Savings Time is allowed.
 *  A few states, such as Arizona, do not have DST.
 *  (TRUE == DST is allowed, FALSE == DST is not allowed).
 */

static Timer_Ticks timeWhenTODSet	= { 0, 0 };
static Timer_Ticks timeOfDay		= { 0, 0 };

Time   timerTimeOfDay			= { 0, 0 };
static int localOffset			= 0;
static Boolean DSTAllowed		= TRUE;

/*
 * Semaphore protecting the above time of day variables.
 */

Sync_Semaphore	timerClockMutex;


/*
 * UpdateTimeOfDay() adjusts timerTimeOfDay to the real time of day.
 */

static void UpdateTimeOfDay();
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

    Sync_SemInitDynamic(&timerClockMutex,"Timer:timerClockMutex");

    Timer_CounterInit();

    /*
     * Add the routine to fix the time of day to the timer queue.
     * The routine is called every 10 seconds.
     */

    updateElement.routine = UpdateTimeOfDay;
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
Timer_GetRealTimeOfDay(timePtr, localOffsetPtr, DSTPtr)
    Time *timePtr;		/* Buffer to hold TOD. */
    int  *localOffsetPtr;	/* Optional buffer to hold local offset. */
    Boolean *DSTPtr;		/* Optional buffer to hold DST allowed flag. */
{
    Timer_Ticks	curTime;
    Timer_Ticks	diff;

    /*
     *  Get the current uptime time and subtract from it the uptime when the
     *	the time of day was last set. Add this difference to the value
     *  of the stored time of day to get the current T.O.D.
     */

    MASTER_LOCK(&timerClockMutex);


    Timer_GetCurrentTicks(&curTime);

    Timer_SubtractTicks(curTime, timeWhenTODSet, &diff);
    Timer_AddTicks(diff, timeOfDay, &diff);
    Timer_TicksToTime(diff, timePtr);

    if (localOffsetPtr != (int *) NIL) {
	*localOffsetPtr = localOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = DSTAllowed;
    }
    MASTER_UNLOCK(&timerClockMutex);

}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_GetTimeOfDay --
 *
 *	Retrieves an approximate value for the time of day. 
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
Timer_GetTimeOfDay(timePtr, localOffsetPtr, DSTPtr)
    Time *timePtr;		/* Buffer to hold TOD. */
    int  *localOffsetPtr;	/* Optional buffer to hold local offset. */
    Boolean *DSTPtr;		/* Optional buffer to hold DST allowed flag. */
{

    MASTER_LOCK(&timerClockMutex);

    *timePtr = timerTimeOfDay;

    if (localOffsetPtr != (int *) NIL) {
	*localOffsetPtr = localOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = DSTAllowed;
    }

    MASTER_UNLOCK(&timerClockMutex);

}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_SetTimeOfDay --
 *
 *	Changes the time of day to a new value.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Updates the global variables that stores the TOD and the time
 *	it was set.
 *
 *----------------------------------------------------------------------
 */

void
Timer_SetTimeOfDay(newTOD, newLocalOffset, newDSTAllowed)
    Time newTOD;		/* New value for time of day. */
    int  newLocalOffset;	/* New value for local offset. */
    Boolean newDSTAllowed;	/* New value for DST allowed flag. */
{

    /*
     *  Record when the T.O.D was changed by saving the current time.
     *  Also store the new T.O.D (it has to be converted to ticks),
     *  the new local offset and the DST flag.
     */

    MASTER_LOCK(&timerClockMutex);


    timerTimeOfDay = newTOD;

    Timer_GetCurrentTicks(&timeWhenTODSet);
    Timer_TimeToTicks(newTOD, &timeOfDay);


    localOffset 	= newLocalOffset;
    DSTAllowed 		= newDSTAllowed;

    MASTER_UNLOCK(&timerClockMutex);

}

/*
 *----------------------------------------------------------------------
 *
 * UpdateTimeOfDay --
 *
 *	Called from the timer queue to make timerTimeOfDay close
 *	to the real current time as calculated by Timer_GetTimeOfDay.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	timerTimeOfDay is updated.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateTimeOfDay()
{
	/* 
	 * No need to get the timerClock Mutex lock because 
	 * Timer_GetRealTimeOfDay gets it for us.
	 */
    Timer_GetRealTimeOfDay(&timerTimeOfDay, (int *) NIL, (int *) NIL);
    Timer_ScheduleRoutine(&updateElement, TRUE);
}


