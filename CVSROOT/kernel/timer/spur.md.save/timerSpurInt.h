/*
 * timerSpurInt.h --
 *
 *	Types and constants for the SPUR Cache Controller timers.
 *
 *	The definitions which define the bit fields for each register are
 *	not fully explained in this file.  For a detailed explanation of the 
 *	chip's functionality and the definitions listed below, see the
 *	SPUR Memory System Architecture by Wood, Eggers, and Gibson
 *	UCB/CSD 87/394
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
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVTIMERINT
#define _DEVTIMERINT

#include "mach.h"
#include "timerTick.h"

/*
 * In order to convert a free running timer to some time we can use
 * to compute the time of day, we need to know the frequency of the updates
 * in cycles per second. This depends on the clock speed of the CPU. 
 * For best results, this variable should be a defined constant so that 
 * constant folding can take place. Currently, we import the value from
 * the mach module. For reference, the SPUR CPU is documented to run at
 * speeds between 100ns and 150ns. This makes TIMER_FREQ in the range
 * of 10000000.0 to 6666666.66 cycles per second. 
 */
#ifdef notdef
#define	TIMER_FREQ	((unsigned int) mach_CycleTime)
#endif

/*
 * For the first spur kernel - 144ns.
 */

/* #define	TIMER_FREQ	((unsigned int) 5813953) */
#define	TIMER_FREQ	((unsigned int) 6944444) 


/*
 * Timer interval times are 32 bit unsigned numbers.  At a 100ns cycle time,
 * a interval will only hold around 7.16 minutes. Intervals should hold
 * at least one hour of time.  TIMER_INTERVAL_SHIFT is the factor to shift
 * the timer to the right to get an interval value. In other words, an
 * interval is represented in units of TIMER_FREQ>>TIMER_INTERVAL_SHIFT.
 *  
 *			ticks	 microsec	32 bit
 * TIMER_INTERVAL_SHIFT	per sec	 per tick	max. time
 *	0		10000000 	.1	7.158 minutes	
 *	1		 2500000	.4	28.63 minutes
 *	2		 1250000	.8	57.27 minutes
 *	3		  625000	1.6	114.53 minutes
 *	4		  312500	3.2	3.82 hours
 *	5		  156250	6.4	7.64 hours
 *	6		   78125	12.8	15.28 hours
 *	7		   39062	25.6	30.59 hours
 *	8		   19531	51.2	2.66 days
 */
#define	TIMER_INTERVAL_SHIFT	3

/*
 * Some macros for converting intervals to Counter low and high words.
 */

#define	INTERVAL_LOW(interval)	((interval) << TIMER_INTERVAL_SHIFT)
#define	INTERVAL_HIGH(interval)	((interval) >> (32-TIMER_INTERVAL_SHIFT))

/*
 * Macro for converting a counter to an interval. Warning: does NOT check
 * for overflows.
 */

#define	COUNTER_TO_INTERVAL(counter) \
	(((counter).low >> TIMER_INTERVAL_SHIFT) + \
		((counter).high << (32-TIMER_INTERVAL_SHIFT)))

/* 
 * Timer/counter assigment. We assignment the free running counter to 
 * T0. T1 is the call back timer and T2 is the profile timer.
 * For each timer we need:
 *	ADDR - The address in the Cache Controllor of the timer.
 *	MASK_BIT - A word set to the bit in the Istatus/Imask registers
 *		   for the timer.
 *	MODE_BIT - A word set to the bit in the mode register for the 
 *		   timer.
 */

/*
 * The CALL BACK Queue timer is T1.
 */

#define	CALLBACK_TIMER_ADDR	MACH_T_10
#define CALLBACK_TIMER_MASK_BIT	MACH_TIMER_T1_INTR
#define	CALLBACK_TIMER_MODE_BIT	MACH_MODE_T1_ENABLE

/*
 * Profiling is done using timer T2.
 */

#define	PROFILE_TIMER_ADDR	MACH_T_20
#define PROFILE_TIMER_MASK_BIT	MACH_TIMER_T2_INTR
#define	PROFILE_TIMER_MODE_BIT	MACH_MODE_T2_ENABLE

/*
 * The free running timer is T0.  There is no interrupt bit from the T0
 * timer.
 */

#define	FREERUNNING_TIMER_ADDR	MACH_T_00
#define	FREERUNNING_TIMER_MODE_BIT	MACH_MODE_T0_ENABLE

/*
 * Declarations of the .s functions for access the timers. The routines are
 * private the timer module. Routines can be found in timerT0.s.
 */

extern ReturnStatus Timer_T0Read();
extern ReturnStatus Timer_T0Write();


#endif _DEVTIMERINT
