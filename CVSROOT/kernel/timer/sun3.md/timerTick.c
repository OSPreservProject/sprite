/*
 * timerTick.c --
 *
 *	Kernel utility procedures to manipulate time Tick values for the Sun3.
 *
 *  The routines in this module manipulate time values that are
 *  represented in the Timer_Ticks format.  The Timer_Ticks format is used
 *  for a specific purpose: to make the operations associated with the
 *  callback timer and timer queue run fast. These operations include
 *  starting the timer, scheduling a routine and calling a routine at its
 *  scheduled time.  Unlike the Time format, which represents time in
 *  seconds and microseconds, the Timer_Ticks format represents time in a
 *  machine-dependent way. On the Sun-3, the hardware free-running
 *  counter format is easily converted to the Time format, so no
 *  distinction is made between Time and Timer_Ticks.
 *
 *
 *  There are several constraints imposed on the Timer_Ticks format to
 *  decrease complexity and overhead in using the format.  First, it can
 *  not be used to represent negative time values.  Second, the routines
 *  are not general. For example, there are no multiply and divide
 *  routines for Timer_Ticks values.  Full generality is obtained by using
 *  the Time module.
 *
 * Copyright 1986, 1988 Regents of the University of California
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
#endif not lint

#include "sprite.h"
#include "mach.h"
#include "timerTick.h"
#include "time.h"
#include "devTimer.h"
#include "sys.h"
#include "byte.h"


/*
 *  Definition of the maximum number of seconds and microseconds that the
 *  hardware free-running counter can count to.  To make intervals fit in
 *  a 32-bit integer, they are constrained to be less than or equal to
 *  timer_MaxIntervalTime.seconds. To use intervals greater than this
 *  value, one must use the Time_ module arithmetic routines.
 */

Time timer_MaxIntervalTime;

/*
 * Some commonly used values for intervals.
 */

Timer_Ticks	timer_TicksZeroSeconds;
unsigned int    timer_IntZeroSeconds;
unsigned int    timer_IntOneSecond;
unsigned int    timer_IntOneMillisecond;
unsigned int    timer_IntOneMinute;
unsigned int    timer_IntOneHour;


/*
 *----------------------------------------------------------------------
 *
 * TimerTicksInit --
 *
 *	Initializes the various tick and interval values.
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
TimerTicksInit()
{
    Time tmp;

    Dev_CounterIntToTime((unsigned int) 0xFFFFFFFF, &timer_MaxIntervalTime);

    tmp.seconds = 1;
    tmp.microseconds = 0;
    Dev_CounterTimeToInt(tmp, &timer_IntOneSecond);

    tmp.seconds = 0;
    tmp.microseconds = 1000;
    Dev_CounterTimeToInt(tmp, &timer_IntOneMillisecond);

    timer_IntZeroSeconds	= 0;
    timer_IntOneMinute		= timer_IntOneSecond * 60;
    timer_IntOneHour		= timer_IntOneSecond * 3600;

    Byte_Zero(sizeof(timer_TicksZeroSeconds), (Address)&timer_TicksZeroSeconds);
}

/*
 *----------------------------------------------------------------------
 *
 *  Timer_AddTicks --
 *
 * 	Adds two tick values together.
 *	For the Sun-3, this routine is #defined to be Time_Add().
 *
 *
 *  Results:
 *	A time in ticks.
 *
 *  Side effects:
 *	None.
 *
 * This is macro defined in timerTick.h
 * #define Timer_AddTicks(a,b,c)		Time_Add(a,b,c)
 *
 *----------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------
 *
 *  Timer_SubtractTicks --
 *
 * 	Subtracts the second parameter from the first parameter. 
 *	The second parameter must be less than the first, otherwise 
 *	a zero tick value is returned.
 *	For the Sun-3, this routine is #defined to be Time_Subtract().
 *
 *  Results:
 *	An absolute time in ticks.
 *
 *  Side effects:
 *	None.
 *
 * This is macro defined in timerTick.h
 * #define Timer_SubtractTicks(a,b,c)	Time_Subtract(a,b,c)
 *----------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------
 *
 *  Timer_AddIntervalToTicks --
 *
 * 	Adds an interval (32-bit value) to an absolute time (64-bit value).
 *
 *  Results:
 *	An absolute time in ticks.
 *
 *  Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Timer_AddIntervalToTicks(absolute, interval, resultPtr)
    Timer_Ticks		absolute;	/* Addend 1 */
    unsigned int	interval;	/* Addend 2 */
    Timer_Ticks		*resultPtr;	/* Sum */
{
    unsigned int	overflow;
    
    Dev_CounterAddIntToCount(absolute,interval,resultPtr,&overflow);

}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_GetCurrentTicks --
 *
 *  	Computes the number of ticks since the system was booted
 *	by reading the free-running counter.
 *
 *
 *  Results:
 *	The system up-time in ticks.
 *
 *  Side effects:
 *	Node
 *
 *----------------------------------------------------------------------
 */


void
Timer_GetCurrentTicks(ticksPtr)
    Timer_Ticks	*ticksPtr;	/* Buffer to place current time. */
{
    DISABLE_INTR();
    Dev_CounterRead(ticksPtr);
    ENABLE_INTR();
}



/*
 *----------------------------------------------------------------------
 *
 *  Timer_TicksToTime --
 *
 *  	Converts a Timer_Ticks value into a Time value.
 *
 *  Results:
 *	A time value in Time format.
 *
 *  Side effects:
 *	None.
 *
 *	For the Sun-3, this routine is #defined to be *timePtr = tick;
 * #define Timer_TicksToTime(a,b)		*(b) = a;
 *----------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------
 *
 *  Timer_TimeToTicks --
 *
 *  	Converts a Time value into a Timer_Ticks value.
 *
 *
 *  Results:
 *	A time value in ticks.
 *
 *  Side effects:
 *	None.
 *
 *	For the Sun-3, this routine is #defined to be *ticksPtr = time;
 *
 *  #define Timer_TimeToTicks(a,b)		*(b) = a;
 *
 *----------------------------------------------------------------------
 */

