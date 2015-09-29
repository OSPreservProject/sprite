/*
 * timer.h --
 *
 *     External definitions for the clock timer routines.
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
 * rcsid: $Header: /user5/kupfer/spriteserver/src/sprited/timer/RCS/timer.h,v 1.7 92/04/16 11:46:41 kupfer Exp $ SPRITE (Berkeley) 
 */

#ifndef _TIMER
#define _TIMER

#include <sprite.h>
#include <list.h>

#ifdef SPRITED
#include <spriteTime.h>
#include <timerTick.h>
#include <syncTypes.h>
#else
#include <spriteTime.h>
#include <sprited/timerTick.h>
#include <sprited/syncTypes.h>
#endif

/* 
 * Enable instrumentation of late callbacks from the timer thread.  This is 
 * a public flag because some of the counters are updated outside the timer 
 * module. 
 */
#define TIMER_BACKLOG_STATS	1

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

    void	(*routine) _ARGS_((Timer_Ticks timeTicks, 
				  ClientData  clientData));	
			        /* public:  address of the routine */
    Timer_Ticks	time;		/* public:  time when the routine should be
				 * 	    called. interval field is ignored*/ 
    ClientData	clientData;	/* public:  data passed to the routine when
				 *	    called. */

    Boolean	processed;	/* public, readonly:  TRUE if routine has
				 * 	    been called (at its scheduled time),
				 *	    FALSE if not called yet. */
    Time	interval;	/* public:  interval relative from now when 
				 * 	    the routine should be called.
				 *	    The time field becomes private. */
} Timer_QueueElement;

extern Time timer_Resolution;

#define TIMER_CALLBACK_INTERVAL_APPROX	(timer_Resolution.microseconds)

/* 
 * Aliases for backward compatibility.
 */

#define Timer_GetCurrentTicks(timePtr) \
    Timer_GetTimeOfDay((timePtr), (int *)NULL, (Boolean *)NULL)
#define Timer_GetRealTimeOfDay(timePtr, offsetPtr, dstPtr) \
    Timer_GetTimeOfDay((timePtr), (offsetPtr), (dstPtr))

extern void Timer_Init _ARGS_((void));
extern Boolean Timer_IsScheduled _ARGS_((Timer_QueueElement *elementPtr));
extern void Timer_ScheduleRoutine _ARGS_((register 
			Timer_QueueElement *newElementPtr, Boolean interval));
extern Boolean Timer_DescheduleRoutine _ARGS_((register 
			Timer_QueueElement *elementPtr));
extern void Timer_GetRealTimeFromTicks _ARGS_((Timer_Ticks ticks, 
			Time *timePtr, int *timerLocalOffsetPtr, 
			Boolean *DSTPtr));
extern void Timer_GetTimeOfDay _ARGS_((Time *timePtr, int *timerLocalOffsetPtr,
			Boolean *DSTPtr));

#endif /* _TIMER */
