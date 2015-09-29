/*
 * time.h --
 *
 *     External definitions for the time utility routines.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcsid: $Header: /sprite/src/lib/include/RCS/spriteTime.h,v 1.4 90/09/11 14:40:15 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SPRITETIME
#define _SPRITETIME

#ifndef _SPRITE
#include "sprite.h"
#endif

/* DATA STRUCTURES */

/*
 *  Definition of a time value.
 */

typedef struct {
    int	seconds;
    int	microseconds;
} Time;

typedef struct {
    int year;
    int month;
    int dayOfYear;
    int dayOfMonth;
    int dayOfWeek;
    int	hours;
    int	minutes;
    int	seconds;
    int localOffset;
    Boolean dst;
} Time_Parts;

/* CONSTANTS */

/*
 *  The number of microseconds in one second and one millisecond.
 */

#define ONE_SECOND		1000000
#define TENTH_SECOND		100000
#define HUNDREDTH_SECOND	10000
#define ONE_MILLISECOND		1000

/*
 *  Length of buffers required by the Time conversion routines.
 */

#define TIME_CVT_BUF_SIZE 30

/*
 *  Frequently used time values.
 */

extern Time time_ZeroSeconds;
extern Time time_OneMicrosecond;
extern Time time_OneMillisecond;
extern Time time_TenMilliseconds;
extern Time time_HundredMilliseconds;
extern Time time_HalfSecond;
extern Time time_OneSecond;
extern Time time_TwoSeconds;
extern Time time_TenSeconds;
extern Time time_OneMinute;
extern Time time_OneHour;
extern Time time_OneDay;
extern Time time_OneYear;
extern Time time_OneLeapYear;


/* PROCEDURES */

extern void	Time_Add _ARGS_((Time time1, Time time2, Time *resultPtr));
extern void	Time_Subtract _ARGS_((Time time1, Time time2,
				      Time *resultPtr));
extern void	Time_Multiply _ARGS_((Time time, int factor, Time *resultPtr));
extern void	Time_Divide _ARGS_((Time time, int factor, Time *resultPtr));
extern void	Time_Normalize _ARGS_((Time *timePtr));
extern void	Time_ToAscii _ARGS_((int time, Boolean relativeTime,
				     char *bufferPtr));
extern void	Time_ToParts _ARGS_((int time, Boolean relativeTime,
				     Time_Parts *partsPtr));


/*
 *----------------------------------------------------------------------
 *
 * Time Comparisons --
 *
 *	Time_LT:	time1  <   time2
 *	Time_LE:	time1  <=  time2
 *	Time_EQ:	time1  ==  time2
 *	Time_GE:	time1  >=  time2
 *	Time_GT:	time1  >   time2
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

#define Time_LT(time1, time2) \
		(((time1).seconds     <  (time2).seconds) ||  \
		 (((time1).seconds     == (time2).seconds) &&  \
		  ((time1).microseconds <  (time2).microseconds)))

#define Time_LE(time1, time2) \
		(((time1).seconds     <  (time2).seconds) ||  \
		 (((time1).seconds     == (time2).seconds) &&  \
		  ((time1).microseconds <= (time2).microseconds)))

#define Time_EQ(time1, time2) \
		(((time1).seconds     == (time2).seconds) &&  \
		 ((time1).microseconds == (time2).microseconds))

#define Time_GE(time1, time2) \
		(((time1).seconds     >  (time2).seconds) ||  \
		 (((time1).seconds     == (time2).seconds) &&  \
		  ((time1).microseconds >= (time2).microseconds)))

#define Time_GT(time1, time2) \
		(((time1).seconds     >  (time2).seconds) ||  \
		 (((time1).seconds     == (time2).seconds) &&  \
		  ((time1).microseconds >  (time2).microseconds)))

#endif /* _SPRITETIME */
