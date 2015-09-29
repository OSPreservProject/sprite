/*
 * timerStat.h --
 *
 *	Statistics for the timer module.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/timer/RCS/timerStat.h,v 1.1 92/04/16 11:58:39 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _TIMERSTAT
#define _TIMERSTAT

#include <spriteTime.h>

/* 
 * One of the counts is the number of calls by the timer thread that happen 
 * long after they were supposed to.  Break down this count according to
 * what system action is being called late.
 */

#define TIMER_FSUTIL_SYNC		0
#define TIMER_OTHER_SERVERPROC		1 /* other Proc_ServerProc calls */
#define TIMER_RPC_DAEMON_WAKEUP		2
#define TIMER_RECOV_PING_INTERVAL	3
#define TIMER_NUM_LATE_TYPES		(TIMER_RECOV_PING_INTERVAL + 1)

typedef struct {
    int		schedule;	/* count of Timer_ScheduleRoutine calls */
    int		desched;	/* count of Timer_DescheduleRoutine calls */
    int		failedDesched;	/* desched calls where the element was 
				 * already gone */
    int		skipSleep;	/* count of times TimerThread didn't bother 
				 * sleeping */ 
    int		resched;	/* count of times we had to interrupt the
				 * sleeping timer thread */
    int		totalLateCalls;	/* total count of late calls */
    int		lateCalls[TIMER_NUM_LATE_TYPES]; /* counts of late calls to 
						  * selected routines */
} Timer_Statistics;

extern Timer_Statistics timer_Statistics;
extern Time timer_QueueMaxBacklog;

#endif /* _TIMERSTAT */
