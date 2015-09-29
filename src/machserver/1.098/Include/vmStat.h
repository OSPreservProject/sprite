/*
 * vmStat.h --
 *
 *	The statistics structure for the vm module.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/lib/include/RCS/vmStat.h,v 8.4 91/03/04 16:09:28 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMSTAT
#define _VMSTAT

#ifdef KERNEL
#include "vmMachStat.h"
#else
#include <kernel/vmMachStat.h>
#endif


/*---------------------------------------------------------------------*/

/*
 * Virtual memory statistics structure.
 */

typedef struct {
    int	numPhysPages;		/* The number of physical pages available. */
    /*
     * Different types of pages
     */
    int	numFreePages;		/* Number of pages on the free list. */
    int	numDirtyPages;		/* Number of pages on the dirty list. */
    int	numReservePages;	/* Number of pages held in reserve for the
				 * kernel. */
    int	numUserPages;		/* Number of pages on the allocate list.  Each
				 * of these pages must be used by user
				 * processes. */
    int	kernStackPages;		/* Number of pages allocated to the kernel.*/
    int kernMemPages;		/* Number of pages allocated to kernel code
				 * + data. */
    /*
     * Page fault statistics.
     */
    int	totalFaults;		/* The total number of page faults that have
				   occured. */
    int	totalUserFaults;	/* The total number of page faults that have
				   occured because of a user bus error. */
    int	zeroFilled;		/* Page faults that were satisfied by zero
				   filling the page. */
    int	fsFilled;		/* Page faults that were satisifed by reading
				   in from the file system. */
    int	psFilled;		/* Page faults that were satisfied by reading
				   in from the page server. */
    int	collFaults;		/* Page faults that collided with another page
				   fault in progress on the same page. */
    int	quickFaults;		/* Page faults that were satisfied by just
				   revalidating the page. */
    int	codeFaults;		/* Code segment faults, ignoring quickFaults.*/
    int	heapFaults;		/* Heap segment faults, ignoring quickFaults.*/
    int	stackFaults;		/* Stack segm't faults, ignoring quickFaults.*/
    /*
     * Page allocation stats.
     */
    int	numAllocs;		/* Total number of page allocations. */
    int	gotFreePage;		/* Number of allocations by using a free page.*/
    int	pageAllocs;		/* Calls to internal page allocator. */
    int	gotPageFromFS;		/* Number of allocations satisfied by 
				 * stealing a page from the file system. */
    int	numListSearches;	/* Number of allocations that actually search
				 * the allocation list. */
    int	usedFreePage;		/* List searches satisfied with free page. */
    int	lockSearched;		/* Number of locked pages encountered 
				 * while searching list. */
    int	refSearched;		/* Number of referenced pages encountered 
				 * while searching list. */
    int	dirtySearched;		/* Number of dirty pages encountered 
				 * while searching list. */
    int	reservePagesUsed;	/* The number of pages held in reserve that */

    /*
     * Dirty page statistics.
     */
    int	pagesWritten;		/* The number of pages that were written out
				   to the page server. */
    int	cleanWait;		/* The number of times that a segment being 
				   cleaned has to wait for a page to finish
				   being cleaned. */
    int	pageoutWakeup;		/* The number of times that the pageout daemon
				   wakes up. */

    int	pageoutNoWork;		/* The number of times that the pageout daemon
				   woke up and there was nothing to do. */
    int pageoutWait;		/* The number of times that a process has to
				   wait for the pageout daemon to finish 
				   because too many pages were dirty. */
    /*
     * Page mapping statistics.
     */
    int	mapPageWait;		/* The number of times that have to wait 
				   because of run out of entries to map 
				   pages. */
    int	accessWait;		/* The number of times that have to wait
				   because of run out of entries to make
				   pages accessible. */
    /*
     * Machine dependent statistics.
     */
    VmMachDepStat	machDepStat;
    /*
     * The minimum number of virtual memory pages
     * that the system guarantees itself. 
     */
    int	minVMPages;
    /*
     * File system mapping stats.
     */
    int	fsAsked;		/* The number of times that the file system
				 * asked us for our reference time. */
    int	haveFreePage;		/* We have a free page when fs asks us for the
				 * reference time. */
    int	fsMap;			/* The number of pages that the file system
				 * got from us. */
    int	fsUnmap;		/* The number of pages that the file system
				 * returned to us. */
    int	maxFSPages;		/* The maximum number of pages given to the
				 * file system. */
    int	minFSPages;		/* The minimum number of pages given to the 
				 * file system. */
    /*
     * Copy-on-write info.
     */
    int	numCOWHeapPages;	/* Heap pages that were made copy-on-write. */
    int	numCOWStkPages;		/* Stack pages that were made copy-on-write. */
    int numCORHeapPages;	/* Heap pages that were made copy-on-ref. */
    int numCORStkPages;		/* Stack pages that were made copy-on-ref. */
    int	numCOWHeapFaults;	/* Heap copy-on-write faults. */
    int	numCOWStkFaults;	/* Stack copy-on-write faults. */
    int	quickCOWFaults;		/* COW faults that were unnecessary. */
    int numCORHeapFaults;	/* Heap copy-on-ref faults. */
    int numCORStkFaults;	/* Stack copy-on-ref faults. */
    int	quickCORFaults;		/* COR faults that were unnecessary. */
    int swapPagesCopied;	/* The number of swap file page copies. */
    int	numCORCOWHeapFaults;	/* Number of copy-on-reference heap pages that
				 * get modified after they are copied. */
    int	numCORCOWStkFaults;	/* Number of copy-on-reference stack pages that
				 * get modified after they are copied. */
    /*
     * Recycled potentially modified page stats.
     */
    int	potModPages;		/* Number of pages that came to the front of
				 * the LRU list, were writeable but were not
				 * modified. */
    int	notModPages;		/* Pages out of potModPages that were never
				 * modified in software or hardware. */
    int	notHardModPages;	/* Pages out of potModPages that were never
				 * modified in hardware. */
    /*
     * Prefetch stats.
     */
    int	codePrefetches;		/* Number of prefetches of code. */
    int	heapSwapPrefetches;	/* Number of prefetches of heap from swap. */
    int	heapFSPrefetches;	/* Number of heap prefetches from the object 
				 * file. */
    int	stackPrefetches;	/* Number of prefetches that hit from the
				 * stack swap file. */
    int	codePrefetchHits;	/* Number of code prefetches that hit. */
    int	heapSwapPrefetchHits;	/* Number of heap prefetches from swap that
				 * hit. */
    int	heapFSPrefetchHits;	/* Number of heap prefetches from the object
				 * file that hit. */
    int	stackPrefetchHits;	/* Number of stack prefetches from swap that
				 * hit. */
    int	prefetchAborts;		/* Number of prefetches aborted because there
    				 * is no memory available. */

} Vm_Stat;

extern	Vm_Stat	vmStat;
#endif /* _VMSTAT */
