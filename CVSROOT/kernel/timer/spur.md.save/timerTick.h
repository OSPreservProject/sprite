/*
 * timerTick.h --
 *
 *      Declarations for the Spur machine dependent clock timer.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcsid: $Header$ SPRITE (Berkeley) 
 */

#ifndef _TIMERTICK
#define _TIMERTICK

#include "time.h"
#include "devTimer.h"

/* DATA STRUCTURES: */

/*
 *  Definition of Timer_Ticks: a hardware-dependent 64-bit time value 
 *  	whose format is based on the free-running counter: 
 *  For the SPUR, Timer_Ticks is the same type as read from the counter.
 *
 * See timer.h and devTimer.h for more details.
 */

typedef  DevCounter	Timer_Ticks;

/*
 * Definitions for useful intervals and clock values. 
 * See timer.h for more details.
 */
 
 extern unsigned int 	timer_IntZeroSeconds; 
 extern unsigned int 	timer_IntOneMillisecond;   
 extern unsigned int 	timer_IntOneSecond;
 extern unsigned int 	timer_IntOneMinute;
 extern unsigned int 	timer_IntOneHour; 
 extern Timer_Ticks	timer_TicksZeroSeconds;
 extern Time 		timer_MaxIntervalTime; 



/* PROCEDURES */

extern void Timer_AddTicks();
extern void Timer_SubtractTicks();
extern void Timer_TicksToTime();
extern void Timer_TimeToTicks();
extern void Timer_AddIntervalToTicks();
extern void Timer_GetCurrentTicks();


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


#define Timer_TickLT(tick1, tick2) \
		(((tick1).high     <  (tick2).high) ||  \
		 (((tick1).high    == (tick2).high) &&  \
		  ((tick1).low <  (tick2).low)))

#define Timer_TickLE(tick1, tick2) \
		(((tick1).high     <  (tick2).high) ||  \
		 (((tick1).high    == (tick2).high) &&  \
		  ((tick1).low <= (tick2).low)))

#define Timer_TickEQ(tick1, tick2) \
		(((tick1).high == (tick2).high) &&  \
		 ((tick1).low == (tick2).low))

#define Timer_TickGE(tick1, tick2) \
		(((tick1).high     >  (tick2).high) ||  \
		 (((tick1).high    == (tick2).high) &&  \
		  ((tick1).low >= (tick2).low)))

#define Timer_TickGT(tick1, tick2) \
		(((tick1).high     >  (tick2).high) ||  \
		 (((tick1).high    == (tick2).high) &&  \
		 ((tick1).low >  (tick2).low)))


#endif _TIMERTICK
