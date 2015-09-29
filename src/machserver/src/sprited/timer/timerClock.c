/* 
 * timerClock.c --
 *
 *	Utility routines for dealing with the clock.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/timer/RCS/timerClock.c,v 1.6 92/04/29 22:07:48 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <device/device.h>
#include <device/device_types.h>
#include <mach/mach_host.h>
#include <mach_error.h>
#include <spriteTime.h>

#include <dev.h>
#include <machCalls.h>
#include <sync.h>
#include <sys.h>
#include <timer.h>

/* 
 * This is the format that a mapped time-of-day clock uses.  This is 
 * equivalent to the Mach mapped_time_value (<mach/time_value.h>), which 
 * unfortunately isn't in the MK43 sources.
 */
typedef struct {
    long seconds;
    long microseconds;
    long checkSeconds;		/* check value so can read w/o lock */
} TimerMappedTime;

/* 
 * The monitor lock is to ensure that access to the time of day and various 
 * state variables is done atomically.
 */

static Sync_Lock clockLock = Sync_LockInitStatic("clockLock");
#define	LOCKPTR &clockLock

/* 
 * If we are able to map the timer into our VM, the variable points to it.  
 */
static volatile TimerMappedTime *mappedTimePtr = NULL;

/* 
 *  timerUniversalToLocalOffset is used to convert from universal time
 *  to local time. It is the number of minutes to add to universal time
 *  to compute the local time. For example, timerLocalOffset for the 
 *  Pacific time zone is -480 minutes. The local time of day is computed 
 *  by multiplying timerUniversalToLocalOffset by 60 and adding the result 
 *  to the universal time (in seconds).  Default to Pacific time, at least 
 *  for now.
 */
int 		timerUniversalToLocalOffset = -480;

/* 
 *  timerDSTAllowed is a flag to indicate if Daylight Savings Time is allowed.
 *  A few states, such as Arizona, do not have DST.
 *  (TRUE == DST is allowed, FALSE == DST is not allowed).
 */
Boolean 	timerDSTAllowed = TRUE;


/*
 *----------------------------------------------------------------------
 *
 * TimerClock_Init --
 *
 *	Map the time-of-day clock into our memory and initialize any 
 *	necessary data structures.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     If mappedTimePtr is non-NULL, it means we were able to map in the 
 *     clock.  Otherwise we must ask the host whenever we want to know the 
 *     current time. 
 *
 *----------------------------------------------------------------------
 */

void
TimerClock_Init()
{
    kern_return_t kernStatus;
    mach_port_t clockPort;	/* handle for the clock */
    mach_port_t clockPager;	/* memory object to map the clock */ 
    
    /* 
     * Get a memory object for the clock and use it to map in the clock.
     */
    kernStatus = device_open(dev_ServerPort, 0, "time", &clockPort);
    if (kernStatus != D_SUCCESS) {
	printf("TimerClock_Init: can't open clock device: %s\n",
	       mach_error_string(kernStatus));
	return;
    }
    kernStatus = device_map(clockPort, VM_PROT_READ, 0,
			    sizeof(*mappedTimePtr), &clockPager, 0);
    if (kernStatus != D_SUCCESS) {
	printf("TimerClock_Init: can't map clock device: %s\n",
	       mach_error_string(kernStatus));
	return;
    }
    if (clockPager == MACH_PORT_NULL) {
	printf("TimerClock_Init: can't map clock device.\n");
	return;
    }
    kernStatus = vm_map(mach_task_self(), (vm_address_t *)&mappedTimePtr,
			sizeof(*mappedTimePtr), 0, TRUE, clockPager, 0, FALSE,
			VM_PROT_READ, VM_PROT_READ, VM_INHERIT_NONE);
    if (kernStatus != D_SUCCESS) {
	printf("TimerClock_Init: can't vm_map clock: %s\n",
	       mach_error_string(kernStatus));
    }
    if (mappedTimePtr == 0) {
	panic("%s: mapping succeeded but gave us null pointer.\n",
	      "TimerClock_Init");
    }
    
    /* 
     * We don't need the pager anymore, so get rid of it.
     */
    (void)mach_port_deallocate(mach_task_self(), clockPager);
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_GetRealTimeFromTicks --
 *
 *	Routine for source compatibility with native Sprite code.
 *
 * Results:
 *	Returns the given ticks value as a time.  These are different units 
 *	in native Sprite but the same thing in the Sprite server.  If the 
 *	local offset or DST pointers are non-nil, stores the local offset 
 *	from GMT or the "DST allowed" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Timer_GetRealTimeFromTicks(ticks, timePtr, timerLocalOffsetPtr, DSTPtr)
    Timer_Ticks	ticks;		/* Ticks value to convert to time. */
    Time *timePtr;		/* OUT: Buffer to hold time value. */
    int  *timerLocalOffsetPtr;	/* OUT: optional buffer to hold local offset */
    Boolean *DSTPtr;		/* OUT: Optional buffer for DST allowed flag */
{
    /*
     * No masterlock, since we can be called from a call-back and get deadlock.
     */

    *timePtr = ticks;
    if (timerLocalOffsetPtr != (int *) NIL) {
	*timerLocalOffsetPtr = timerUniversalToLocalOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = timerDSTAllowed;
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_GetTimeOfDay --
 *
 *	Get the time of day.
 *
 * Results:
 *	Fills in the time of day, local offset (from Universal Time), and 
 *	"DST allowed" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Timer_GetTimeOfDay(timePtr, timerLocalOffsetPtr, DSTPtr)
    Time *timePtr;		/* OUT: Buffer to hold TOD. */
    int  *timerLocalOffsetPtr;	/* OUT: buffer to hold local offset (may be 
				 * nil) */
    Boolean *DSTPtr;		/* OUT: buffer to hold DST allowed flag 
				 * (may be nil) */
{
    kern_return_t kernStatus;

    /* 
     * Only use the monitor lock to protect the local offset from GMT and 
     * the daylight savings flag.  Rely on Mach to provide adequate 
     * protection for the time-of-day service that it provides.
     */
    if (mappedTimePtr != NULL) {
	do {
	    timePtr->seconds = mappedTimePtr->seconds;
	    timePtr->microseconds = mappedTimePtr->microseconds;
	} while (timePtr->seconds != mappedTimePtr->checkSeconds);
    } else {
	kernStatus = host_get_time(mach_host_self(), (time_value_t *)timePtr);
	if (kernStatus != KERN_SUCCESS) {
	    panic("Timer_GetTimeOfDay: couldn't get time: %s\n",
		  mach_error_string(kernStatus));
	}
    }

    if (timerLocalOffsetPtr == (int *)NIL &&
	    DSTPtr == (Boolean *)NIL) {
	return;
    }

    LOCK_MONITOR;
    if (timerLocalOffsetPtr != (int *) NIL) {
	*timerLocalOffsetPtr = timerUniversalToLocalOffset;
    }
    if (DSTPtr != (Boolean *) NIL) {
	*DSTPtr = timerDSTAllowed;
    }
    UNLOCK_MONITOR;
}
