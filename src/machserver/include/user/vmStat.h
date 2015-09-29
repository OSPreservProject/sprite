/*
 * vmStat.h --
 *
 *	The statistics structure for the vm module.
 *
 * Copyright (C) 1986, 1992 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/vmStat.h,v 1.11 92/07/09 15:43:33 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMSTAT
#define _VMSTAT

#include <spriteTime.h>
#ifdef SPRITED
#include <vmTypes.h>
#else
#include <sprited/vmTypes.h>
#endif

/*
 * Virtual memory statistics structure.
 */

typedef struct {
    /* 
     * Misc. stats.
     */
    int syncCalls;		/* number of calls to Vm_SyncAll */
    int	numPhysPages;		/* number of pages in memory */

    /* 
     * External pager stats.
     */
    /* data_request */
    int pagesRead[VM_NUM_SEGMENT_TYPES]; /* pages gotten 100% from FS */
    int partialPagesRead[VM_NUM_SEGMENT_TYPES];	/* pages partially from FS */
    int pagesZeroed[VM_NUM_SEGMENT_TYPES]; /* pages completely zero-filled */
    Time readTime[VM_NUM_SEGMENT_TYPES]; /* time spent in data_request */
    Time readCopyTime[VM_NUM_SEGMENT_TYPES]; /* data_request time for 
					      * segment copy */ 
    /* data_return */
    int returnCalls;		/* calls to data_return */
    int pagesWritten[VM_NUM_SEGMENT_TYPES]; /* pages written */
    int pagesCleaned[VM_NUM_SEGMENT_TYPES]; /* pages written because of 
					     * Sprite cleaning request */
    Time writeTime[VM_NUM_SEGMENT_TYPES]; /* time spent in data_return */
    /* memory_object_init */
    int initCalls;		/* calls to memory_object_init */
    int forcedInits;		/* number of times segment init was forced 
				 * by VmAddrRegion */
    Time initTime;		/* time spent in memory_object_init */
    /* memory_object_terminate */
    int terminateCalls;		/* calls to memory_object_terminate */
    Time terminateTime;		/* time spent in memory_object_terminate */
    /* memory_object_data_unlock */
    int unlockCalls;		/* calls to memory_object_data_unlock */
    /* memory_object_lock_completed */
    int lockCompletedCalls;	/* calls to memory_object_lock_completed */
    Time lockCompletedTime;	/* time spent in lock_completed */

    /* 
     * segment stats.
     */
    int segmentsCreated;
    int segmentsDestroyed;
    int segmentsNeedlesslyDestroyed;
				/* swap segments that could have been 
				 * trivially reused */
    int swapPagesWasted;	/* number of pages that had to be faulted 
				 * in again because the heap/stack segment 
				 * was destroyed at exec */
    int queueOverflows;		/* num times segment had to be removed from 
				 * request port set because it was getting
				 * swamped by requests */
    int segmentLookups;		/* num times segment was sought in main list */
    int segmentsLookedAt;	/* num of segments that had to be looked at 
				 * when doing lookups */ 

    /* 
     * Debugging stats; liable to change.
     */

    /* Stats for segment copies. */
    int segmentCopies;		/* number of copies */
    Time forkTime;		/* total time spent in Vm_Fork */
    Time findRegionTime;	/* Vm_Fork time in vm_region */
    Time segLookupTime;		/* Vm_Fork time in VmSegByName */
    Time segCopyTime;		/* Vm_Fork time in VmSegmentCopy */
    Time regionCopyTime;	/* Vm_Fork time in CopyRegion */
    int sourceCopyRead;		/* source pages read from swap while 
				 * copying segment */ 
    int sourceCopyZeroed;	/* source pages zero-filled */
    int targetCopyRead;		/* target pages read from swap during 
				 * copy, only to be overwritten */
    int targetCopyZeroed;	/* target pages zero-filled before being 
				 * overwritten  */
    int pagesCopied;		/* sum of page sizes copied */
    int swapPagesCopied;	/* sum of page sizes of segment backing 
				 * files  */
    int objPagesCopied;		/* sum of page sizes of heap "init" files */
} Vm_Stat;

#ifdef SPRITED
extern	Vm_Stat	vmStat;
#endif

#endif /* _VMSTAT */
