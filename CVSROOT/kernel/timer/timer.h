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


/* DATA STRUCTURES: */

#ifdef SUN2
/*
 *  Definition of Timer_Ticks: a hardware-dependent 64-bit time value 
 *  	whose format is based on the free-running counter: 
 *
 *  The low field corresponds to the value of the free-running counter. 
 *  If the counter has wrapped around since the last reading, the high field
 *  is incremented.  The maximum value this format can represent is about 
 *  1 million years using a counter frequency of 307200 Hz. This range 
 *  should be sufficient for the near future.
 */

typedef struct {
    unsigned int	low;
    unsigned int	high;
} Timer_Ticks;

#else

/*
 * On the Sun-3, Timer_Ticks is just a Time value because the free-running
 * counter keeps track of time.
 */
typedef Time Timer_Ticks;

#endif SUN2

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


/*
 * Definitions of some frequently used tick and interval values.
 */

extern unsigned int 	timer_IntZeroSeconds;
extern unsigned int 	timer_IntOneMillisecond;
extern unsigned int 	timer_IntOneSecond;
extern unsigned int 	timer_IntOneMinute;
extern unsigned int 	timer_IntOneHour;
extern Time 		timer_MaxTimeForTicks;
extern Timer_Ticks	timer_TicksZeroSeconds;

/* PROCEDURES */

extern void Timer_Init();
extern void Timer_ServiceInterrupt();
extern void Timer_ScheduleRoutine();
extern void Timer_RescheduleRoutine();
extern void Timer_DescheduleRoutine();

#ifdef SUN2
extern void Timer_AddTicks();
extern void Timer_SubtractTicks();
extern void Timer_TicksToTime();
extern void Timer_TimeToTicks();
#endif SUN2

#ifdef SUN3
#define Timer_AddTicks(a,b,c)		Time_Add(a,b,c)
#define Timer_SubtractTicks(a,b,c)	Time_Subtract(a,b,c)
#define Timer_TicksToTime(a,b)		*(b) = a;
#define Timer_TimeToTicks(a,b)		*(b) = a;
#endif SUN3

extern void Timer_AddIntervalToTicks();
extern void Timer_GetCurrentTicks();
extern void Timer_GetTimeOfDay();
extern void Timer_GetRealTimeOfDay();
extern void Timer_SetTimeOfDay();


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

#ifdef SUN3
#define Timer_TickLT(a,b)	Time_LT(a,b)
#define Timer_TickLE(a,b)	Time_LE(a,b)
#define Timer_TickEQ(a,b)	Time_EQ(a,b)
#define Timer_TickGE(a,b)	Time_GE(a,b)
#define Timer_TickGT(a,b)	Time_GT(a,b)
#endif SUN3

#ifdef SUN2

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
#endif SUN2

#endif _TIMER
