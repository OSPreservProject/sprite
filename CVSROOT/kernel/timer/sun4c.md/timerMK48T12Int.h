/*
 * timerMK48T12Int.h --
 *
 *	Declarations of Mostek MK48T12-15 Zeropower/Timekeeper NVRAM and
 *	time of day clock.  
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _TIMERMK48T12INT
#define _TIMERMK48T12INT

/* constants */

/* data structures */

typedef struct TimerRegs  {
    unsigned char control; /* TOD chip control registers. Defined below. */
    unsigned char second;  /* TOD second of minute 0 to 59. */
    unsigned char minutes; /* TOD minute of hour 0 to 59. */
    unsigned char hour;    /* TOD hour of day 0 to 23. */
    unsigned char weekday; /* TOD day of week 1 to 7. */
    unsigned char day;     /* TOD day of month 1 to 31. */
    unsigned char month;   /* TOD month of year 1 to 12. */
    unsigned char year;    /* TOD year 0 to 99. */
} TimerRegs;


#define	TOD_WRITE_CTRL	0x80	/* Write TOD registers. */
#define	TOD_READ_CTRL	0x40    /* Read TOD registers. */
#define	TOD_SIGN_CTRL	0x20	/* Sign control. */

#endif /* _TIMERMK48T12INT */

