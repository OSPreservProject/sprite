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
#include "spriteTime.h"
#include "timerIntersilInt.h"
#include "sys.h"


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
 * The interval value to represent one second. It must be at least 1000
 * so that one milliscond can be be represented in an interval.
 */

#define ONE_SEC_INTERVAL	1000


/*
 * The maximum amount of time that an interval can represent.
 */

#define MAXINT ((unsigned int) 0xFFFFFFFF)
static Time maxIntervalTime = {
    MAXINT / ONE_SEC_INTERVAL,
    ((MAXINT % ONE_SEC_INTERVAL) * ONE_SECOND) / ONE_SEC_INTERVAL,
};


/*
 * Forward declaration of routines.
 */
static void ConvertTimeToInt();
static void ConvertIntToTime();


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

    ConvertIntToTime((unsigned int) 0xFFFFFFFF, &timer_MaxIntervalTime);

    tmp.seconds = 1;
    tmp.microseconds = 0;
    ConvertTimeToInt(tmp, &timer_IntOneSecond);

    tmp.seconds = 0;
    tmp.microseconds = 1000;
    ConvertTimeToInt(tmp, &timer_IntOneMillisecond);

    timer_IntZeroSeconds	= 0;
    timer_IntOneMinute		= timer_IntOneSecond * 60;
    timer_IntOneHour		= timer_IntOneSecond * 3600;

    bzero((Address)&timer_TicksZeroSeconds, sizeof(timer_TicksZeroSeconds));
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
    Time	tmp;
    /*
     * Since Timer_Ticks is a time value, convert the interval to a time
     * and use Timer_Add. .
     */

    ConvertIntToTime(interval, &tmp);
    Time_Add(absolute, tmp, resultPtr);

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


/*
 *----------------------------------------------------------------------
 *
 *  ConvertTimeToInt --
 *
 *      Converts a standard time value into a 32-bit interval value.
 *
 *  Results:
 *	An interval value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertTimeToInt(time, resultPtr)
    Time time;
    unsigned int *resultPtr;
{
    if (Time_LE(time, maxIntervalTime)) {
	*resultPtr = (time.seconds * ONE_SEC_INTERVAL) + 
		 ((time.microseconds * ONE_SEC_INTERVAL) / ONE_SECOND);
    } else {
	printf( "ConvertTimeToInt: time value too large\n");
	*resultPtr = 0xFFFFFFFF;
    }
}


/*
 *----------------------------------------------------------------------
 *
 *  Timer_TicksToInterval --
 *
 *      Converts a ticks value into a 32-bit interval value.
 *
 *  Results:
 *	An interval value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Timer_TicksToInterval(ticks, resultPtr)
    Timer_Ticks ticks;
    unsigned int *resultPtr;
{
    ConvertTimeToInt(ticks, resultPtr);
}


/*
 *----------------------------------------------------------------------
 *
 *  ConvertIntToTime --
 *
 *      Converts a 32-bit interval value into a standard time value.
 *
 *  Results:
 *	A time value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertIntToTime(counter, resultPtr)
    unsigned int counter;
    Time *resultPtr;
{
    resultPtr->seconds = counter / ONE_SEC_INTERVAL;
    resultPtr->microseconds = (counter % ONE_SEC_INTERVAL) * 
				(ONE_SECOND/ONE_SEC_INTERVAL);
}

