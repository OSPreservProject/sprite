/*
 * rpcHistogram.h --
 *
 *	Definitions for the histograms of event durations.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCHISTOGRAM
#define _RPCHISTOGRAM

#include <spriteTime.h>
#ifdef KERNEL
#include <sync.h>
#else
#include <kernel/sync.h>
#endif /* KERNEL */

/*
 * An empirical time distribution is kept in the following structure.
 * This includes the average, plus an array of calls vs. microseconds
 * with some granularity on the time for each bucket.
 */
typedef struct Rpc_Histogram {
    Sync_Lock lock;		/* Used to monitor access to histogram */
    int numCalls;		/* The total number of calls */
    Time aveTimePerCall;	/* The average interval duration */
    Time totalTime;		/* The total time spent in the calls */
    Time overheadTime;		/* Overhead cost per call */
    int	usecPerBucket;		/* The granularity of the histogram */
    int numHighValues;		/* Count of out-of-bounds values */
    int bucketShift;		/* Used to map from time to bucket */
    int numBuckets;		/* The number of slots in the histogram */
    int *bucket;		/* The array of counters */
} Rpc_Histogram;

/*
 * This is the size of all the histograms kept by the system.
 * Although they could vary in size, one size is used in order to
 * simplify the interface to the user program that prints out
 * the histograms.
 */
#define RPC_NUM_HIST_BUCKETS 50
/*
 * The service time is measured on both the client and the server.
 * These flags enable/disable this measurement.  The macros are used
 * to invoke the tracing procedures, subject to the flags.
 */
extern Boolean rpcServiceTiming;
extern Boolean rpcCallTiming;

#define RPC_CALL_TIMING_START(command, timePtr) \
    if (rpcCallTiming) { \
	Rpc_HistStart(rpcCallTime[command], timePtr); \
    }
#define RPC_CALL_TIMING_END(command, timePtr) \
    if (rpcCallTiming) { \
	Rpc_HistEnd(rpcCallTime[command], timePtr); \
    }
#define RPC_SERVICE_TIMING_START(command, timePtr) \
    if (rpcServiceTiming) { \
	Rpc_HistStart(rpcServiceTime[command], timePtr); \
    }
#define RPC_SERVICE_TIMING_END(command, timePtr) \
    if (rpcServiceTiming) { \
	Rpc_HistEnd(rpcServiceTime[command], timePtr); \
    }

extern Rpc_Histogram *Rpc_HistInit _ARGS_((int numBuckets, int usecPerBucket));
extern void Rpc_HistReset _ARGS_((register Rpc_Histogram *histPtr));
extern void Rpc_HistStart _ARGS_((register Rpc_Histogram *histPtr, register Time *timePtr));
 extern void Rpc_HistEnd _ARGS_((register Rpc_Histogram *histPtr, register Time * timePtr));
extern ReturnStatus Rpc_HistDump _ARGS_((register Rpc_Histogram *histPtr, register Address buffer));
extern void Rpc_HistPrint _ARGS_((register Rpc_Histogram *histPtr));

#endif /* _RPCHISTOGRAM */
