/*
 * timer.h --
 *
 *     External definitions for the clock timer routines.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcsid: $Header$ SPRITE (Berkeley) 
 */

#ifndef _TIMER
#define _TIMER

#include "list.h"
#include "time.h"

#include "timerTick.h"
/*
 * timerTick.h should define the following types/structures/routine/macros for 
 * the desired architecture:
 *
 *   typedef	Timer_Ticks -	The typedef of the object that can 
 *				hold the value of the machine's timers.
 *				This is should be in a format that
 *				can be quickly generated from the 
 *				timer values provided by the hardware.
 *				The idea here is to leave the timer
 *				values in these "natural" units to
 *				avoid expensive conversions on the
 *				frequent timer interrupts.
 *
 *	Tick arithmetic and conversion routines. May be macros.
 *----------------------------------------------------------------------
 *
 *  Timer_AddTicks --
 *
 * 	Adds two tick values together.
 *
 *  void
 *  Timer_AddTicks(a, b, resultPtr)
 *	Timer_Ticks		a;		/* Addend 1 
 *	Timer_Ticks		b;		/* Addend 2 
 *	Timer_Ticks		*resultPtr;	/* Sum 
 *----------------------------------------------------------------------
 *
 *  Timer_SubtractTicks --
 *
 * 	Subtracts the second parameter from the first parameter. 
 *	The second parameter must be less than the first, otherwise 
 *	a zero tick value is returned.
 *
 *  void
 *  Timer_SubtractTicks(a, b, resultPtr)
 *	Timer_Ticks		a;		/* Minuhend 
 *	Timer_Ticks		b;		/* Subtrahend 
 *	Timer_Ticks		*resultPtr;	/* Difference 
 *    
 *----------------------------------------------------------------------
 *
 *  Timer_AddIntervalToTicks --
 *
 * 	Adds an interval (32-bit value) to an absolute time (64-bit value).
 *
 *    void
 *    Timer_AddIntervalToTicks(absolute, interval, resultPtr)
 *	Timer_Ticks		absolute;	/* Addend 1 
 *	unsigned int     	interval;	/* Addend 2
 *	Timer_Ticks		*resultPtr;	/* Sum 
 *----------------------------------------------------------------------
 *
 *  Timer_GetCurrentTicks --
 *
 *  	Computes the number of ticks since the system was booted.
 *
 *    void
 *    Timer_GetCurrentTicks(ticksPtr)
 *	Timer_Ticks	*ticksPtr;	/* Buffer to place current time. 
 *----------------------------------------------------------------------
 *
 *  Timer_TicksToTime --
 *
 *  	Converts a Timer_Ticks value into a Time value.
 *    void
 *    Timer_TicksToTime(tick, timePtr)
 *	Timer_Ticks	tick;		/* Value to be converted. 
 *	Time	*timePtr;	/* Buffer to hold converted value. 
 *----------------------------------------------------------------------
 *
 *  Timer_TimeToTicks --
 *
 *  	Converts a Time value into a Timer_Ticks value.
 *   void
 *    Timer_TimeToTicks(time, ticksPtr)
 *	Time	time;		/* Value to be converted. 
 *	Timer_Ticks	*ticksPtr;	/* Buffer to hold converted value.
 *----------------------------------------------------------------------
 *
 * TimerTicksInit --
 *
 *	Initializes the various tick and interval values.
 *
 * void
 * TimerTicksInit()
 *----------------------------------------------------------------------
 * Tick Comparisons --
 *
 *	Timer_TickLT(tick1,tick2):	tick1  <   tick2
 *	Timer_TickLE(tick1,tick2):	tick1  <=  tick2
 *	Timer_TickEQ(tick1,tick2):	tick1  ==  tick2
 *	Timer_TickGE(tick1,tick2):	tick1  >=  tick2
 *	Timer_TickGT(tick1,tick2):	tick1  >   tick2
 *
 *----------------------------------------------------------------------
 * Additionally, timerTick.h should provide the following frequently used
 * tick and intervals values. These values should be initialized by the 
 * machine dependent routine TimerTicksInit().  
 * 
 * extern unsigned int 	timer_IntZeroSeconds; - Zero seconds worth of ticks.
 * extern unsigned int 	timer_IntOneMillisecond; - One milliseconds of ticks.  
 * extern unsigned int 	timer_IntOneSecond; - One second of ticks.
 * extern unsigned int 	timer_IntOneMinute; - One minute of ticks.
 * extern unsigned int 	timer_IntOneHour;  - One hour of ticks.
 * extern Timer_Ticks	timer_TicksZeroSeconds; - Zero seconds worth of ticks.
 * extern Time 		timer_MaxIntervalTime; 
 *      - Maximum time for a unsigned 32-bit interger worth of ticks. This
 * is the longest possible interval value.
 */


 /* DATA STRUCTURES: */


/*
 *  TimerQueueElement defines the structure to store info about a
 *  routine so it can be called at its scheduled time.
 *
 *  Only three fields need to be filled in by the client before passing 
 *  the element's address to Timer_ScheduleRoutine:
 *	a) The address of the routine,
 *	b) The time when the routine should be called. 
 *	   If the time value is an "absolute" time and not a time 
 *	   relative to the current time, use the time field.
 *	   If the time value is relative to the current time, use 
 *	   the interval field.
 *	c) Client-specific data to be passed to the routine when it is 
 *	   called.
 *
 *  Since the client controls the storage of the timer queue element, it
 *  could modify fields of the structure while it is on the timer queue.
 *  Once the element is scheduled, it must be treated as read-only!!
 *  Only when the scheduled routine is called or descheduled can the element 
 *  be modified by the client.
 *
 *  Note:  Do not access any part of the structure that is 
 *  	   labeled "private". These fields can be accessed only by 
 *	   the Timer module routines.
 *
 *	   Public fields marked "readonly" must not be modified by 
 *	   the client.
 *
 */


typedef struct {
    List_Links	links;		/* private: */

    void	(*routine)();	/* public:  address of the routine */
    Timer_Ticks	time;		/* public:  time when the routine should be
				 * 	    called. interval field is ignored*/ 
    ClientData	clientData;	/* public:  data passed to the routine when
				 *	    called. */

    Boolean	processed;	/* public, readonly:  TRUE if routine has
				 * 	    been called (at its scheduled time),
				 *	    FALSE if not called yet. */
    unsigned int interval;	/* public:  interval relative from now when 
				 * 	    the routine should be called.
				 *	    The time field becomes private. */
} Timer_QueueElement;

/*
 *  The statistics counts used by the dump routines.
 */

typedef struct {
    int		callback;	/* count of callback timer interrupts */
    int		profile;	/* count of profile timer interrupts */
    int		spurious;	/* count of spurious timer interrupts */
    int		schedule;	/* count of Timer_ScheduleRoutine calls */
    int		resched;	/* count of Timer_RescheduleRoutine calls */
    int		desched;	/* count of Timer_DescheduleRoutine calls */
} Timer_Statistics;

extern Timer_Statistics	timer_Statistics;

extern void Timer_ServiceInterrupt();
extern void Timer_ScheduleRoutine();
extern void Timer_RescheduleRoutine();
extern void Timer_DescheduleRoutine();

extern void Timer_GetTimeOfDay();
extern void Timer_GetRealTimeOfDay();
extern void Timer_SetTimeOfDay();

#endif _TIMER
