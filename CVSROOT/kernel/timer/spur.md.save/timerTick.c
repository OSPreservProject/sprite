/*
 * timerTick.c --
 *
 *	Kernel utility procedures to manipulate time Tick values for the SPUR
 *	hardware.
 *
 *  The routines in this module manipulate time values that are
 *  represented in the Timer_Ticks format.  The Timer_Ticks format is used
 *  for a specific purpose: to make the operations associated with the
 *  callback timer and timer queue run fast. These operations include
 *  starting the timer, scheduling a routine and calling a routine at its
 *  scheduled time.  Unlike the Time format, which represents time in
 *  seconds and microseconds, the Timer_Ticks format represents time in a
 *  machine-dependent way. On the SPUR, Timer_Ticks is a value based on
 *  the hardware's free-running 64bit counter. 
 *
 *  A time value in the Timer_Ticks format is a hardware-dependent 64-bit
 *  number that represents a specific or absolute point in time since some
 *  some event (on the SPUR, since the system was booted).  A time value
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
 * Copyright 1988 Regents of the University of California
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
#include "timerSpurInt.h"
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

Timer_Ticks	timer_TicksZeroSeconds = {0, 0};
unsigned int    timer_IntZeroSeconds;
unsigned int    timer_IntOneSecond;
unsigned int    timer_IntOneMillisecond;
unsigned int    timer_IntOneMinute;
unsigned int    timer_IntOneHour;

/*
 * The largest interval value.
 */
#define	MAXINT	((unsigned int ) 0xffffffff)

/*
 * Scale factor to make integer division more accurate. 
 */
#define	SCALE_FACTOR	100

/*
 * 2 ^ 32.
 */
#define	TWO_TO_32 	4294967296.0

#ifndef FLOATING_POINTER_CONVERT
static	Time	timeHigh;	/* Time per high tick of counter. */
static unsigned int maxMicrosecondMutiple;/* Maximum number that we can
					   * multiply timeHigh.microseconds 
					   * by and still fix in a 32bit 
					   * integer.  */
static unsigned int secondsPerMicrosecondMultiple;
					/* The number of seconds per each 
					 * maxMicrosecondMutiple. 
					 */
/*
 * The maximum number that we can multiply and microsecond value and have it
 * fit an a 32bit integer.
 */
#define	MAX_MICROSEC_MULTIPLE	((int)(MAXINT / ONE_SECOND))
#endif

/*
 * Forward routine declarations. 
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

#ifndef FLOATING_POINT_CONVERT
    /*
     * Initailized values used by counter conversion routines.
     */
    {
	double	intPart, modf();
	timeHigh.microseconds = (int)
		((ONE_SECOND * modf(TWO_TO_32/TIMER_FREQ,&intPart)) + 0.5);
	timeHigh.seconds = (int) intPart;
	maxMicrosecondMutiple = (unsigned int)(TWO_TO_32/timeHigh.microseconds);
	secondsPerMicrosecondMultiple = (unsigned int) (TWO_TO_32/ONE_SECOND);
    }
#endif
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
 *----------------------------------------------------------------------
 */


void
Timer_AddTicks(a, b, resultPtr)
    Timer_Ticks		a;		/* Addend 1 */
    Timer_Ticks		b;		/* Addend 2 */
    Timer_Ticks		*resultPtr;	/* Sum */
{
    resultPtr->low	= a.low  + b.low;
    resultPtr->high	= a.high + b.high;

    if (resultPtr->low < b.low) {
	resultPtr->high++;
    }

}



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
 *----------------------------------------------------------------------
 */


void
Timer_SubtractTicks(a, b, resultPtr)
    Timer_Ticks		a;		/* Minuhend */
    Timer_Ticks		b;		/* Subtrahend */
    Timer_Ticks		*resultPtr;	/* Difference */
{
    if (Timer_TickLE(a, b)) {
	*resultPtr = timer_TicksZeroSeconds;
    } else {
	resultPtr->low = a.low - b.low;

	/*
	 * See if a borrow is necessary from the high-order part.
	 */
	if (a.low < b.low) {
	    resultPtr->high = a.high - b.high - 1;
	} else {
	    resultPtr->high = a.high - b.high;
	}
    }
}


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
    unsigned	int	intervalLow;
    unsigned	int	intervalHigh;

    intervalLow = INTERVAL_LOW(interval);  
    intervalHigh = INTERVAL_HIGH(interval);
    resultPtr->low = absolute.low + intervalLow; 
    resultPtr->high = absolute.high + intervalHigh;

    if (resultPtr->low < intervalLow) {
	 resultPtr->high++;
    } 
    if (resultPtr->high < intervalHigh) {
	Sys_Panic(SYS_WARNING,"Timer_AddIntervalToTicks: overflow\n");
    }

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
 *----------------------------------------------------------------------
 */


void
Timer_TicksToTime(tick, timePtr)
    Timer_Ticks	tick;		/* Value to be converted. */
    Time	*timePtr;	/* Buffer to hold converted value. */
{
#ifdef FLOATING_POINT_CONVERT
    extern	double	modf();
    double	countDouble;

    countDouble = (count.low + TWO_TO_32 * count.high) / TIMER_FREQ;
    resultPtr->microseconds = (int)
			((1000000.0 * modf(countDouble,&countDouble)) + 0.5);
    resultPtr->seconds = (int) countDouble;

#else
    unsigned int	low, frac;

    timePtr->seconds = timeHigh.seconds * tick.high;
    if (tick.high > maxMicrosecondMutiple) { 
	timePtr->seconds += (tick.high / maxMicrosecondMutiple) * 
				secondsPerMicrosecondMultiple;
	low = (tick.high % maxMicrosecondMutiple) * timeHigh.microseconds;
    } else {
	low = tick.high * timeHigh.microseconds;
    }
    if (low > ONE_SECOND) {
	timePtr->seconds += low/ONE_SECOND;
        timePtr->microseconds = low % ONE_SECOND; 
    } else {
        timePtr->microseconds = low; 
    }

    timePtr->seconds += tick.low / TIMER_FREQ;
    frac = tick.low % TIMER_FREQ;
    if (frac < MAX_MICROSEC_MULTIPLE) {
	timePtr->microseconds += (frac * ONE_SECOND) / TIMER_FREQ;
    } else if (frac < MAX_MICROSEC_MULTIPLE*10) {
	timePtr->microseconds += ((frac * (ONE_SECOND/10)) / (TIMER_FREQ/10));
    } else if (frac < MAX_MICROSEC_MULTIPLE*100) {
	timePtr->microseconds += ((frac * (ONE_SECOND/100))/(TIMER_FREQ/100));
    } else if (frac <MAX_MICROSEC_MULTIPLE*1000 ) {
	timePtr->microseconds += ((frac * (ONE_SECOND/1000)) / 	
						(TIMER_FREQ/1000));
    } else {
	timePtr->microseconds += ((frac * (ONE_SECOND/10000)) /
						(TIMER_FREQ/10000));
    }
    if (timePtr->microseconds > ONE_SECOND) {
	timePtr->microseconds -= ONE_SECOND;
	timePtr->seconds++;
    }
#endif
}



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
 *----------------------------------------------------------------------
 */


void
Timer_TimeToTicks(time, ticksPtr)
    Time	time;		/* Value to be converted. */
    Timer_Ticks	*ticksPtr;	/* Buffer to hold converted value. */
{

#ifdef FLOATING_POINT_CONVERT

    double	countDouble;


    countDouble = (time.seconds + (time.microseconds / 1000000.0)) 
			* (double)TIMER_FREQ;

    ticksPtr->high =  (int) (countDouble / TWO_TO_32 );
    ticksPtr->low = (int)((countDouble - (ticksPtr->high * TWO_TO_32)) + 0.5);
#else
    Time	newtime;
    unsigned  	int	ticks;

    if (time.seconds > timeHigh.seconds) {
	unsigned int	high;
	ticksPtr->high = high = (time.seconds / (timeHigh.seconds+1));
	ticksPtr->low = 0;
	Timer_TicksToTime(*ticksPtr,&newtime);
	Time_Subtract(time, newtime, &time);
	Timer_TimeToTicks(time,ticksPtr);
	ticksPtr->high += high; 
	return; 
    } else {
	ticksPtr->high = 0;
    }
    ticks = time.seconds * TIMER_FREQ;
    ticksPtr->low =  (time.microseconds / 100) * (TIMER_FREQ/10000) + ticks;
    if (ticksPtr->low < ticks) {
	ticksPtr->high++;
    }
#endif

}


/*
 *----------------------------------------------------------------------
 *
 *  ConvertTimeToInt --
 *
 *      Converts a standard time value into an interval  value.
 *
 *  Results:
 *	None.
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
    Timer_Ticks		tmp;

    /*
     * Convert time to a ticks value 
     */
    Timer_TimeToTicks(time,&tmp);

    /*
     * Check to see if value is too bit for an interval.
     */
    if (tmp.high > INTERVAL_HIGH(MAXINT) || 
       ((tmp.high == INTERVAL_HIGH(MAXINT)) && 
	(tmp.low > INTERVAL_LOW(MAXINT)))) {
	Sys_Panic(SYS_WARNING, "ConvertTimeToInt: time value too large\n");
	*resultPtr = MAXINT;
    } else {
        *resultPtr = COUNTER_TO_INTERVAL(tmp);
    }
}



/*
 *----------------------------------------------------------------------
 *
 *  ConvertIntToTime --
 *
 *      Converts an interval value into a standard time value.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ConvertIntToTime(interval, resultPtr)
    unsigned int interval;
    Time *resultPtr;
{
    Timer_Ticks		tmp;
    unsigned int	overflow;

    Timer_AddIntervalToTicks(timer_TicksZeroSeconds, interval, &tmp);
    Timer_TicksToTime(tmp,resultPtr);

}


