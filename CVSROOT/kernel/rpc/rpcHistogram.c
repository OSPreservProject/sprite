/*
 * rpcHistogram.c --
 *
 *      Simple histograms of event durations are maintained by the
 *      routines in this module.  The data recorded includes an average of
 *      time samples, and a histogram at some granularity of time
 *      intervals.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "status.h"
#include "sync.h"
#include "timer.h"
#include "byte.h"
#include "rpcHistogram.h"
#include "mem.h"

#define LOCKPTR (&histPtr->lock)


/*
 *----------------------------------------------------------------------
 *
 * Rpc_HistInit --
 *
 *	Initialize the data structure used to keep an empirical time
 *	distribution, or histogram.
 *
 * Results:
 *	A pointer to the data structure, suitable for passing to
 *	Rpc_HistStart and Rpc_HistStop.
 *
 * Side effects:
 *	Allocates memory for the structure.
 *
 *----------------------------------------------------------------------
 */
Rpc_Histogram *
Rpc_HistInit(numBuckets, usecPerBucket)
    int numBuckets;	/* The number of columns in the histogram */
    int usecPerBucket;	/* The time step between columns in the histogram */
{
    register Rpc_Histogram *histPtr;
    register int bound;
    Timer_Ticks startTicks, endTicks;

    histPtr = (Rpc_Histogram *)Mem_Alloc(sizeof(Rpc_Histogram));
    histPtr->numBuckets = numBuckets;
    histPtr->bucket = (int *)Mem_Alloc(numBuckets * sizeof(int));
    histPtr->lock.inUse = FALSE;
    histPtr->lock.waiting = FALSE;
    histPtr->aveTimePerCall.seconds = 0;
    histPtr->aveTimePerCall.microseconds = 0;
    Byte_Zero(sizeof(Time), (Address)&histPtr->totalTime);
    histPtr->numCalls = 0;
    /*
     * Truncate the usecPerBucket to a power of two.  This lets the sampling
     * routines use shifts instead of modulo.
     */
    if (usecPerBucket < 2) {
	usecPerBucket = 2;
    }
    histPtr->bucketShift = 0;
    for (bound = 2 ; bound <= usecPerBucket ; bound <<= 1) {
	histPtr->bucketShift++;
    }
    
    histPtr->usecPerBucket = bound >> 1;
    /*
     * Time the cost of calling the histogram sampling routines.
     */
    Timer_GetCurrentTicks(&startTicks);
    for (bound=0 ; bound<10 ; bound++) {
	Time time;
	Rpc_HistStart(histPtr, &time);
	Rpc_HistEnd(histPtr, &time);
    }
    Timer_GetCurrentTicks(&endTicks);
    Timer_SubtractTicks(endTicks, startTicks, &endTicks);
    Timer_TicksToTime(endTicks, &histPtr->overheadTime);
    Time_Divide(histPtr->overheadTime, 10, &histPtr->overheadTime);
    Rpc_HistReset(histPtr);
    return(histPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_HistReset --
 *
 *	Reset the histograms, so they start fresh for another benchmark.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The counters and average are reset.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Rpc_HistReset(histPtr)
    register Rpc_Histogram *histPtr;
{
    register int i;

    LOCK_MONITOR;

    histPtr->numCalls = 0;
    Byte_Zero(sizeof(Timer_Ticks), (Address)&histPtr->totalTime);
    histPtr->aveTimePerCall.seconds = 0;
    histPtr->aveTimePerCall.microseconds = 0;
    histPtr->numHighValues = 0;
    for (i=0 ; i<histPtr->numBuckets ; i++) {
	histPtr->bucket[i] = 0;
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_HistStart --
 *
 *	Take a time sample to start a measured interval and update
 *	the number of calls.  On a Sun-2 this costs about 650 microseconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Take a time sample and count calls.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Rpc_HistStart(histPtr, timePtr)
    register Rpc_Histogram *histPtr;	/* The histogram */
    register Time *timePtr;		/* Client storage area fro the time
					 * sample */
{
    LOCK_MONITOR;
    Timer_GetRealTimeOfDay(timePtr, (int *)NIL, (Boolean *)NIL);
    histPtr->numCalls++;
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_HistEnd --
 *
 *	Called at the end of an interval, this determines the length of
 *	the interval, keeps a running sum, and updates a counter
 *	in the histogram corresponding to the interval length.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment a counter in the histogram.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Rpc_HistEnd(histPtr, timePtr)
    register Rpc_Histogram *histPtr;	/* The histogram */
    register Time *timePtr;		/* Result from Rpc_HistStart */
{
    Time endTime;
    register int index;
    LOCK_MONITOR;
    Timer_GetRealTimeOfDay(&endTime, (int *)NIL, (Boolean *)NIL);
    Time_Subtract(endTime, *timePtr, timePtr);
    index = (timePtr->seconds * 1000 + timePtr->microseconds) >>
		histPtr->bucketShift;
    if (index >= histPtr->numBuckets) {
	histPtr->numHighValues++;
    } else {
	histPtr->bucket[index]++;
    }
    Time_Add(histPtr->totalTime, *timePtr, &histPtr->totalTime);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_HistDump --
 *
 *	Copy the histogram data structure to the callers buffer.
 *	It is assumed that it is a user space buffer, and that it
 *	is large enough (a lame assumption).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The copy.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Rpc_HistDump(histPtr, buffer)
    register Rpc_Histogram *histPtr;
    register Address buffer;
{
    register ReturnStatus status;
    
    LOCK_MONITOR;
    status = Vm_CopyOut(sizeof(Rpc_Histogram), (Address)histPtr, buffer);
    if (status == SUCCESS) {
	buffer += sizeof(Rpc_Histogram);
	status = Vm_CopyOut(histPtr->numBuckets * sizeof(int),
			    (Address)histPtr->bucket, buffer);
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_HistPrint --
 *
 *	Print the histogram data structure to the console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The copy.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Rpc_HistPrint(histPtr)
    register Rpc_Histogram *histPtr;
{
    register int i;
    LOCK_MONITOR;
    Time_Divide(histPtr->totalTime, histPtr->numCalls,
				    &histPtr->aveTimePerCall);
    Sys_Printf("%d Calls,  ave %d.%06d secs each\n",
		   histPtr->numCalls, histPtr->aveTimePerCall.seconds,
		   histPtr->aveTimePerCall.microseconds);
    for (i=0 ; i<histPtr->numBuckets ; i++) {
	Sys_Printf("%8d ", i * histPtr->usecPerBucket);
    }
    Sys_Printf("Overflow\n");
    for (i=0 ; i<histPtr->numBuckets ; i++) {
	Sys_Printf("%7d  ", histPtr->bucket[i]);
    }
    Sys_Printf("%d\n", histPtr->numHighValues);
    Sys_Printf("\n");
    UNLOCK_MONITOR;
}
