/*
 * timerTick.c --
 *
 *	Kernel utility procedures to manipulate time Tick values for the sun4c.
 *	This will change when we figure out how to use the TOD chip.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "mach.h"
#include "timerTick.h"
#include "spriteTime.h"
#include "sys.h"

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
    timer_IntOneMillisecond = ONE_MILLISECOND;
    timer_IntOneSecond = ONE_SECOND;
    timer_IntZeroSeconds = 0;
    timer_IntOneMinute = timer_IntOneSecond * 60;
    timer_IntOneHour = timer_IntOneSecond * 3600;
    bzero((Address)&timer_TicksZeroSeconds, sizeof(timer_TicksZeroSeconds));
}

/*
 *----------------------------------------------------------------------
 *
 *  Timer_AddTicks --
 *
 * 	Adds two tick values together.
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
    Time	intTime;

    intTime.seconds = interval / ONE_SECOND;
    intTime.microseconds = interval % ONE_SECOND;
    Time_Add(absolute, intTime, resultPtr);
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
 *	This routine is #defined to be *timePtr = tick;
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
 *  Results:
 *	A time value in ticks.
 *
 *  Side effects:
 *	None.
 *
 *  This routine is #defined to be *ticksPtr = time;
 *
 *  #define Timer_TimeToTicks(a,b)		*(b) = a;
 *----------------------------------------------------------------------
 */

