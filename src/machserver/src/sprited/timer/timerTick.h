/*
 * timerTick.h --
 *
 *      Declarations for compatibility with the native Sprite 
 *      Timer_Ticks stuff.
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
 * rcsid: $Header: /r3/kupfer/spriteserver/src/sprited/timer/RCS/timerTick.h,v 1.3 91/11/14 10:03:09 kupfer Exp $ SPRITE (Berkeley) 
 * 
 * Originally timer/sun3.md/timerTick.h.
 */

#ifndef _TIMERTICK
#define _TIMERTICK

#include <spriteTime.h>

/* DATA STRUCTURES: */


/* 
 * The Mach kernel provides a standard clock format.  There's not much 
 * point in keeping a special count of ticks.
 */

typedef Time Timer_Ticks;


extern Time 		timer_MaxIntervalTime; 



#ifdef lint
#define Timer_AddTicks(a,b,c)      Time_Add((* ((Time *) &a)),\
					    (* ((Time *) &b)), \
					    (Time *) c)
#define Timer_SubtractTicks(a,b,c) Time_Subtract((* ((Time *) &a)), \
						 (* ((Time *) &b)), \
						 (Time *) c)
#else /* lint */
#define Timer_AddTicks(a,b,c)      Time_Add(a,b,c)
#define Timer_SubtractTicks(a,b,c) Time_Subtract(a,b,c)
#endif /* lint */

/*
 * Conversion to Ticks to Time and Time to Ticks is fast too.
 */

#define Timer_TicksToTime(a,b)		*(b) = a;
#define Timer_TimeToTicks(a,b)		*(b) = a;

/*
 * Aliases for backward compatibility.
 */
 
#define timer_IntZeroSeconds	time_ZeroSeconds
#define timer_IntOneMillisecond	time_OneMillisecond
#define	timer_IntOneSecond	time_OneSecond
#define	timer_IntOneMinute	time_OneMinute
#define	timer_IntOneHour	time_OneHour
#define	timer_TicksZeroSeconds	time_ZeroSeconds

#define Timer_AddIntervalToTicks(absolute, interval, resultPtr) \
	Time_Add((absolute), (interval), (resultPtr))



/*
 *----------------------------------------------------------------------
 *
 * Tick Comparisons --
 *
 *	Timer_TickLT:	tick1  <   tick2
 *	Timer_TickLE:	tick1  <=  tick2
 *	Timer_TickEQ:	tick1  ==  tick2
 *	Timer_TickGE:	tick1  >=  tick2
 *	Timer_TickGT:	tick1  >   tick2
 *
 * Results:
 *     TRUE	- the relation holds for the 2 values.
 *     FALSE	- the relation does not hold.
 *
 * Side effects:
 *     None.
 *
 *----------------------------------------------------------------------
 */

#ifdef lint
#define Timer_TickLT(a,b)	Time_LT((* ((Time *) &a)), (* ((Time *) &b)))
#define Timer_TickLE(a,b)	Time_LE((* ((Time *) &a)), (* ((Time *) &b)))
#define Timer_TickEQ(a,b)	Time_EQ((* ((Time *) &a)), (* ((Time *) &b)))
#define Timer_TickGE(a,b)	Time_GE((* ((Time *) &a)), (* ((Time *) &b)))
#define Timer_TickGT(a,b)	Time_GT((* ((Time *) &a)), (* ((Time *) &b)))
#else /* lint */
#define Timer_TickLT(a,b)	Time_LT(a,b)
#define Timer_TickLE(a,b)	Time_LE(a,b)
#define Timer_TickEQ(a,b)	Time_EQ(a,b)
#define Timer_TickGE(a,b)	Time_GE(a,b)
#define Timer_TickGT(a,b)	Time_GT(a,b)
#endif /* lint */

#endif /* _TIMERTICK */

