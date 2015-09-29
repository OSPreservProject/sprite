/*
 * printStats.c --
 *	Routines to print out program execution times, and filesystem stats.
 */

#include "sprite.h"
#include "status.h"
#include "sys/ioctl.h"
#include "sys/file.h"
#include "stdio.h"
#include "proc.h"
#include "vm.h"
#include "sysStats.h"
#include "kernel/fs.h"
#include "kernel/fsStat.h"
#include "kernel/sched.h"
#include "kernel/vm.h"



/*
 *----------------------------------------------------------------------
 *
 * PrintTimes --
 *
 *	Print the resource usage (user and kernel CPU time) and elapsed time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the specified stream
 *
 *----------------------------------------------------------------------
 */
void
PrintTimes(stream, usagePtr, timePtr)
    FILE *stream;
    Proc_ResUsage *usagePtr;
    Time *timePtr;
{
    Time delta;
    if (usagePtr != NULL) {
	Time_Add(usagePtr->userCpuUsage, usagePtr->childUserCpuUsage,
					 &delta);
	fprintf(stream, "%d.%03du ", delta.seconds,
			       delta.microseconds / 1000);
	Time_Add(usagePtr->kernelCpuUsage, usagePtr->childKernelCpuUsage,
					 &delta);
	fprintf(stream, "%d.%03ds ", delta.seconds,
			       delta.microseconds / 1000);
    }
    if (timePtr != NULL) {
	int seconds = timePtr->seconds;
	if (seconds >= 3600) {
	    fprintf(stream, "%d:", seconds / 3600);
	    seconds = seconds % 3600;
	}
	if (seconds >= 60) {
	    fprintf(stream, "%d:", seconds / 60);
	    seconds = seconds % 60;
	}
	fprintf(stream, "%d.%03d", seconds,
			       timePtr->microseconds / 1000);
    }
    fprintf(stream, "\n");
}


/*
 *----------------------------------------------------------------------
 *
 * PrintIdleTime --
 *
 *	Given two samples sched module statistics, this computes
 *	the differenc in idle ticks and, using the time, computes
 *	a utilization.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the specified stream
 *
 *----------------------------------------------------------------------
 */
void
PrintIdleTime(stream, startSchedPtr, endSchedPtr, timePtr)
    FILE *stream;
    Sched_Instrument *startSchedPtr, *endSchedPtr;
    Time *timePtr;
{
    register 	highTicks;
    double 	lowTicks;
    int		cpu;

    Sched_Instrument zeroStats;
    if (startSchedPtr == NULL) {
	bzero(&zeroStats, sizeof(Sched_Instrument));
	startSchedPtr = &zeroStats;
    }

    for (cpu=0; cpu<MACH_MAX_NUM_PROCESSORS; cpu++) {
    	highTicks = endSchedPtr->processor[cpu].idleTicksOverflow -
		startSchedPtr->processor[cpu].idleTicksOverflow;
    	lowTicks = endSchedPtr->processor[cpu].idleTicksLow 
		- startSchedPtr->processor[cpu].idleTicksLow;

    	if (highTicks != 0) {
		fprintf(stream, "(High ticks = %d)", highTicks);
    	}
    	if (timePtr->seconds == 0 && timePtr->microseconds == 0) {
		fprintf(stream, "Idle ticks --/-- = 100%% Idle, Elapsed time ");
    	} else {
		lowTicks /= 
	  	(double)timePtr->seconds 
			+ (double) (timePtr->microseconds)/1000000.;
		fprintf(stream, "Idle ticks %0.0f/%d = %6.2f%% Idle, Context Sw. %d inv %d full %d, Elapsed time ",
	       		lowTicks,
			endSchedPtr->processor[cpu].idleTicksPerSecond,
			(double)lowTicks/(double)endSchedPtr->processor[cpu].idleTicksPerSecond * 100.,
			endSchedPtr->processor[cpu].numContextSwitches 
			    - startSchedPtr->processor[cpu].numContextSwitches,
	       		endSchedPtr->processor[cpu].numInvoluntarySwitches 
			    - startSchedPtr->processor[cpu].numInvoluntarySwitches,
	       		endSchedPtr->processor[cpu].numFullCS 
			    - startSchedPtr->processor[cpu].numFullCS);
    	}
    	PrintTimes(stream, (Proc_ResUsage *)0, timePtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrintFsStats --
 *
 *	Print out the filesystem statistics.  If both a start and end
 *	sample of the statistics are given then the differences between
 *	the two are printed.  To just print the total cumulative statistics
 *	from one sample, specify a single FsStats buffer with the 'end'
 *	parameter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the specified stream
 *
 *----------------------------------------------------------------------
 */
void
PrintFsStats(stream, start, end, verbose)
    FILE *stream;	/* Output stream */
    FsStats *start;	/* 0, or address of "before run" statistics */
    FsStats *end;	/* End of run statistics */
    int verbose;	/* If true, everything is dumped */
{
    register int t1, t2, t3, t4, t5;
    FsStats zeroStats;

    if (start == (FsStats *)0) {
	bzero(&zeroStats, sizeof(FsStats));
	start = &zeroStats;
    }
    /*
     * Print cache size
     */
    fprintf(stream, "Cache blocks max %d min %d number %d/%d free %d/%d limit %d\n",
			   end->blockCache.maxCacheBlocks,
			   end->blockCache.minCacheBlocks,
			   start->blockCache.numCacheBlocks,
			   end->blockCache.numCacheBlocks,
			   start->blockCache.numFreeBlocks,
			   end->blockCache.numFreeBlocks,
			   end->blockCache.maxNumBlocks);

    /*
     * Print bytes read traffic ratio
     */
    t1 = end->blockCache.bytesRead - start->blockCache.bytesRead;
    t2 = end->blockCache.dirBytesRead - start->blockCache.dirBytesRead;
    t3 = end->gen.remoteBytesRead - start->gen.remoteBytesRead;
    t4 = end->gen.fileBytesRead - start->gen.fileBytesRead;
    t5 = end->gen.physBytesRead - start->gen.physBytesRead;
    fprintf(stream, "Bytes read %d+%d remote %d disk %d+%d",
		       t1, t2, t3, t4, t5);
    if (t1 + t2 > 0) {
#ifdef stupid_compiler
	fprintf(stream, "\ttraffic ratio %%%d\n",
		       (int)((double)(t3+t4+t5)/(double)(t1+t2) * 100.));
#else
	fprintf(stream, "\n");
#endif
    } else {
	fprintf(stream, "\n");
    }

    /*
     * Print bytes written traffic ratio
     */
    t1 = end->blockCache.bytesWritten - start->blockCache.bytesWritten +
	(end->blockCache.fileDescWrites - start->blockCache.fileDescWrites +
	 end->blockCache.indBlockWrites - start->blockCache.indBlockWrites) *
	FS_BLOCK_SIZE;
    t2 = end->blockCache.dirBytesWritten - start->blockCache.dirBytesWritten;
    t3 = end->gen.remoteBytesWritten - start->gen.remoteBytesWritten;
    t4 = end->gen.fileBytesWritten - start->gen.fileBytesWritten;
    t5 = end->gen.physBytesWritten - start->gen.physBytesWritten;
    fprintf(stream, "Bytes written %d+%d remote %d disk %d+%d",
			   t1, t2, t3, t4, t5);
#ifdef stupid_compiler
    if (t1 + t2 > 0) {
	fprintf(stream, "\ttraffic ratio %%%d",
			   (int)((double)(t3+t4+t5)/(double)(t1+t2) * 100.));
    }
#endif
    fprintf(stream, "\n");

    if (verbose) {
	/*
	 * Print device bytes and zero fills
	 */
	t1 = end->gen.deviceBytesWritten - start->gen.deviceBytesWritten;
	t2 = end->gen.deviceBytesRead - start->gen.deviceBytesRead;
	fprintf(stream, "Dev bytes read %d written %d\n", t1, t2);
	t1 = end->blockCache.readZeroFills - start->blockCache.readZeroFills;
	t2 = end->blockCache.writeZeroFills1 -
	    start->blockCache.writeZeroFills1;
	t3 = end->blockCache.writeZeroFills2 -
	    start->blockCache.writeZeroFills2;
	t4 = end->blockCache.fragZeroFills - start->blockCache.fragZeroFills;
	fprintf(stream, "Zero Fills read %d write1 %d write2 %d frag %d\n",
			       t1, t2, t3, t4);
	t1 = end->blockCache.appendWrites - start->blockCache.appendWrites;
	t2 = end->blockCache.overWrites - start->blockCache.overWrites;
	t3 = end->blockCache.domainReadFails -
	    start->blockCache.domainReadFails;
	fprintf(stream, "Appends %d Overwrites %d Failed Reads %d\n",
			       t1, t2, t3);
    }
    t1 = end->blockCache.readAccesses - start->blockCache.readAccesses +
	 end->blockCache.fragAccesses - start->blockCache.fragAccesses +
	 end->blockCache.fileDescReads - start->blockCache.fileDescReads +
	 end->blockCache.indBlockAccesses - start->blockCache.indBlockAccesses +
	 end->blockCache.dirBlockAccesses - start->blockCache.dirBlockAccesses;
    t2 = end->blockCache.readHitsOnDirtyBlock -
	start->blockCache.readHitsOnDirtyBlock;
    t3 = end->blockCache.readHitsOnCleanBlock -
	start->blockCache.readHitsOnCleanBlock;
    t4 = end->blockCache.fragHits - start->blockCache.fragHits +
	 end->blockCache.fileDescReadHits - start->blockCache.fileDescReadHits +
	 end->blockCache.indBlockHits - start->blockCache.indBlockHits +
	 end->blockCache.dirBlockHits - start->blockCache.dirBlockHits;
    fprintf(stream, "Cache reads %d hits: dirty %d clean %d other %d",
			   t1, t2, t3, t4);
#ifdef stupid_compiler
    if (t1 != 0) {
	fprintf(stream, "\thit ratio %%%d",
		       (int)((double)(t2+t3+t4)/(double)t1 * 100.));
    }
#endif
    fprintf(stream, "\n");

    t1 = end->blockCache.readAheads - start->blockCache.readAheads;
    t2 = end->blockCache.readAheadHits - start->blockCache.readAheadHits;
    t3 = end->blockCache.allInCacheCalls - start->blockCache.allInCacheCalls;
    t4 = end->blockCache.allInCacheTrue - start->blockCache.allInCacheTrue;
    if (t1 > 0) {
	fprintf(stream, "Read Ahead: hits %d/%d all-in-cache %d/%d\n",
			t2, t1, t4, t3);
    }

    t1 = end->blockCache.writeAccesses - start->blockCache.writeAccesses +
	 end->blockCache.fileDescWrites - start->blockCache.fileDescWrites +
	 end->blockCache.indBlockWrites - start->blockCache.indBlockWrites +
	 end->blockCache.dirBlockWrites - start->blockCache.dirBlockWrites;
    t2 = end->blockCache.partialWriteHits - start->blockCache.partialWriteHits +
	end->blockCache.fileDescWriteHits - start->blockCache.fileDescWriteHits;
    t3 = end->blockCache.partialWriteMisses -
	start->blockCache.partialWriteMisses;
    t4 = end->blockCache.blocksWrittenThru -
	start->blockCache.blocksWrittenThru;
    fprintf(stream, "Cache writes %d hits %d misses %d thru %d",
			   t1, t2, t3, t4);
#ifdef stupid_compiler
    if (t1 != 0) {
	fprintf(stream, "\ttraffic ratio %%%d",
			   (int)((double)(t3+t4)/(double)t1 * 100.));
    }
#endif
    fprintf(stream, "\n");
    
    fprintf(stream, "Write thru %d data %d indirect %d desc %d dir %d\n",
			   t4,
			   end->blockCache.dataBlocksWrittenThru -
			   start->blockCache.dataBlocksWrittenThru,
			   end->blockCache.indBlocksWrittenThru -
			   start->blockCache.indBlocksWrittenThru,
			   end->blockCache.descBlocksWrittenThru -
			   start->blockCache.descBlocksWrittenThru,
			   end->blockCache.dirBlocksWrittenThru -
			   start->blockCache.dirBlocksWrittenThru);
    if (end->blockCache.fileDescReads > 0) {
	fprintf(stream, "File descriptor reads %d hits %d writes %d hits %d\n",
			       end->blockCache.fileDescReads -
			       start->blockCache.fileDescReads,
			       end->blockCache.fileDescReadHits -
			       start->blockCache.fileDescReadHits,
			       end->blockCache.fileDescWrites -
			       start->blockCache.fileDescWrites,
			       end->blockCache.fileDescWriteHits -
			       start->blockCache.fileDescWriteHits);
    }
    if (end->blockCache.indBlockAccesses > 0) {
	fprintf(stream, "Indirect block reads %d hits %d writes %d\n",
			   end->blockCache.indBlockAccesses -
			   start->blockCache.indBlockAccesses,
			   end->blockCache.indBlockHits -
			   start->blockCache.indBlockHits,
			   end->blockCache.indBlockWrites -
			   start->blockCache.indBlockWrites);
    }
    if (end->blockCache.dirBlockAccesses > 0) {
	fprintf(stream, "Directory block reads %d hits %d writes %d\n",
			       end->blockCache.dirBlockAccesses -
			       start->blockCache.dirBlockAccesses,
			       end->blockCache.dirBlockHits -
			       start->blockCache.dirBlockHits,
			       end->blockCache.dirBlockWrites -
			       start->blockCache.dirBlockWrites);
    }
    if (end->blockCache.vmRequests > 0) {
	fprintf(stream, "VM requests %d tried %d gave %d\n",
			       end->blockCache.vmRequests -
			       start->blockCache.vmRequests,
			       end->blockCache.triedToGiveToVM -
			       start->blockCache.triedToGiveToVM,
			       end->blockCache.vmGotPage -
			       start->blockCache.vmGotPage);
    }
    fprintf(stream, "Cache blocks created %d, alloc from free %d part %d lru %d\n",
			       end->blockCache.unmapped -
			       start->blockCache.unmapped,
			       end->blockCache.totFree -
			       start->blockCache.totFree,
			       end->blockCache.partFree -
			       start->blockCache.partFree,
			       end->blockCache.lru -
			       start->blockCache.lru);
    if (end->alloc.blocksAllocated > 0) {
	fprintf(stream, "Disk blocks alloc %d free %d search %d/%d hash %d\n",
			       end->alloc.blocksAllocated -
			       start->alloc.blocksAllocated,
			       end->alloc.blocksFreed -
			       start->alloc.blocksFreed,
			       end->alloc.cylsSearched -
			       start->alloc.cylsSearched,
			       end->alloc.cylBitmapSearches -
			       start->alloc.cylBitmapSearches,
			       end->alloc.cylHashes -
			       start->alloc.cylHashes);
	fprintf(stream, "Fragments alloc %d free %d upgrade %d blocks made %d used %d, bad hints %d\n",
			       end->alloc.fragsAllocated -
			       start->alloc.fragsAllocated,
			       end->alloc.fragsFreed -
			       start->alloc.fragsFreed,
			       end->alloc.fragUpgrades -
			       start->alloc.fragUpgrades,
			       end->alloc.fragToBlock -
			       start->alloc.fragToBlock,
			       end->alloc.fullBlockFrags -
			       start->alloc.fullBlockFrags,
			       end->alloc.badFragList -
			       start->alloc.badFragList);
    }
    if (end->nameCache.accesses > 0) {
	fprintf(stream, "Name cache entries %d accesses %d hits %d replaced %d\n",
			   end->nameCache.size,
			   end->nameCache.accesses -
			   start->nameCache.accesses,
			   end->nameCache.hits -
			   start->nameCache.hits,
			   end->nameCache.replacements -
			   start->nameCache.replacements);
    }
    fprintf(stream, "Handles %d created %d installed %d hits %d old %d version %d flush %d\n",
			   end->handle.exists,
			   end->handle.created -
			   start->handle.created,
			   end->handle.installCalls -
			   start->handle.installCalls,
			   end->handle.installHits -
			   start->handle.installHits,
			   0,
			   end->handle.versionMismatch -
			   start->handle.versionMismatch,
			   end->handle.cacheFlushes -
			   start->handle.cacheFlushes);
    fprintf(stream, "\tfetched %d hits %d released %d locks %d/%d wait %d\n",
			   end->handle.fetchCalls -
			   start->handle.fetchCalls,
			   end->handle.fetchHits -
			   start->handle.fetchHits,
			   end->handle.release -
			   start->handle.release,
			   end->handle.locks -
			   start->handle.locks,
			   end->handle.locks -
			   start->handle.locks,
			   end->handle.lockWaits -
			   start->handle.lockWaits);
    fprintf(stream, "Segments fetched %d hits %d\n",
			   end->handle.segmentFetches -
			   start->handle.segmentFetches,
			   end->handle.segmentHits -
			   start->handle.segmentHits);
    fprintf(stream, "Lookup relative %d absolute %d redirect %d found %d loops %d timeouts %d stale %d\n",
			   end->prefix.relative -
			   start->prefix.relative,
			   end->prefix.absolute -
			   start->prefix.absolute,
			   end->prefix.redirects -
			   start->prefix.redirects,
			   end->prefix.found -
			   start->prefix.found,
			   end->prefix.loops -
			   start->prefix.loops,
			   end->prefix.timeouts -
			   start->prefix.timeouts,
			   end->prefix.stale -
			   start->prefix.stale);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintDiskStats --
 *
 *	Print out statistics for the disks.  If both a start and end
 *	sample of the statistics are given then the differences between
 *	the two are printed.  To just print the total cumulative statistics
 *	from one sample, specify a single VmStats buffer with the 'end'
 *	parameter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the specified stream
 *
 *----------------------------------------------------------------------
 */
void
PrintDiskStats(stream, start, end)
    FILE		*stream;	/* Output stream */
    Sys_DiskStats	*start;	/* 0, or address of "before run" statistics */
    Sys_DiskStats	*end;	/* End of run statistics */
{
    int	i = 0;
    while (1) {
	if (end[i].name[0] == 0) {
	    return;
	}
	if (start == 0) {
	    fprintf(stream, "Disk (%s, %d): %0.2f%% Idle Reads %d Writes %d\n",
		    end[i].name, end[i].controllerID,
		    100 * ((float)end[i].idleCount / (float)end[i].numSamples),
		    end[i].diskReads, end[i].diskWrites);
	} else {
	    fprintf(stream, "Disk (%s, %d) %0.0f%% Idle Reads %d Writes %d\n",
		    end[i].name, end[i].controllerID,
		    100 * ((float)(end[i].idleCount - start[i].idleCount) /
		           (float)(end[i].numSamples - start[i].numSamples)),
		    end[i].diskReads - start[i].diskReads,
		    end[i].diskWrites - start[i].diskWrites);
	}
	i++;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrintVmStats --
 *
 *	Print out VM statistics.  If both a start and end
 *	sample of the statistics are given then the differences between
 *	the two are printed.  To just print the total cumulative statistics
 *	from one sample, specify a single VmStats buffer with the 'end'
 *	parameter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the specified stream
 *
 *----------------------------------------------------------------------
 */
void
PrintVmStats(stream, start, end)
    FILE *stream;	/* Output stream */
    Vm_Stat *start;	/* 0, or address of "before run" statistics */
    Vm_Stat *end;	/* End of run statistics */
{
    register	int	*diffPtr;
    register	int	*startPtr;
    register	int	*endPtr;
    int			i;
    int			inusePages;
    int			totPages;
    int			numModifiedPages;
    Vm_Stat		diffStat;
    int			totPercent;
    int			totFaults;
    int			heapPercent;
    int			stkPercent;
    int			quickPercent;	
    int			totHits;
    int			totPrefetches;
    int			hitPct;

    startPtr = (int *)start;
    endPtr = (int *)end;
    diffPtr = (int *)&diffStat;

    for (i = 0; 
         i < sizeof(Vm_Stat) / sizeof(int); 
	 i++, startPtr++, endPtr++, diffPtr++) {
	*diffPtr = *endPtr - *startPtr;
    }

    (void)Vm_Cmd(VM_COUNT_DIRTY_PAGES, &numModifiedPages);
    fprintf(stream, "Kernel VM Pages: %d (Code+Data=%d Stacks=%d)\n",
	     end->kernMemPages + end->kernStackPages,
	     end->kernMemPages, end->kernStackPages);
    inusePages = end->numDirtyPages + end->numUserPages;
    totPages = end->numFreePages + inusePages;
    fprintf(stream, "User VM Pages:   %d (Free=%d Dirty=%d Res=%d Alloc-list=%d)\n",
	    end->numFreePages + end->numDirtyPages + 
	    end->numReservePages + end->numUserPages, 
	    end->numFreePages, end->numDirtyPages,
	    end->numReservePages,
	    end->numUserPages);
    fprintf(stream, "Modified pages: Total=%d %%Tot-dirty=%0.2f %%Inuse-dirty=%0.2f\n",
	    numModifiedPages,
	    (float) (numModifiedPages) / (float)totPages * 100.0,
	    (float) (numModifiedPages) / (float)inusePages * 100.0);
    fprintf(stream, "FS Pages: Current=%d Max=%d Min=%d\n", 
		end->fsMap - end->fsUnmap, end->maxFSPages, end->minFSPages);
    fprintf(stream,
	     "Faults: %8d (Zero=%d FS=%d Swap=%d Quick=%d Coll=%d)\n", 
	     diffStat.totalFaults, diffStat.zeroFilled, diffStat.fsFilled,
	     diffStat.psFilled,diffStat.quickFaults, diffStat.collFaults);
    fprintf(stream, "        %8d (Code=%d Heap=%d Stack=%d)\n", 
	     diffStat.totalFaults, diffStat.codeFaults, diffStat.heapFaults,
	     diffStat.stackFaults);
    fprintf(stream, 
	    "Mod page stats:  Pot-mod=%d Not-mod=%d Not-hard-mod=%d\n",
	    diffStat.potModPages, diffStat.notModPages, 
	    diffStat.notHardModPages);

    /*
     * Copy on write. 
     */
    totPages = diffStat.numCOWStkPages + diffStat.numCOWHeapPages;
    totFaults = diffStat.numCOWStkFaults + diffStat.numCOWHeapFaults;
    if (diffStat.numCOWHeapPages > 0) {
	heapPercent = 100.0 * ((float)diffStat.numCOWHeapFaults / 
				      diffStat.numCOWHeapPages);
    } else {
	heapPercent = 0;
    }
    if (diffStat.numCOWStkPages > 0) {
	stkPercent = 100.0 * ((float)diffStat.numCOWStkFaults / 
				      diffStat.numCOWStkPages);
    } else {
	stkPercent = 0;
    }
    if (totPages > 0) {
	totPercent = 100.0 * ((float)totFaults / totPages);
    } else {
	totPercent = 0;
    }
    if (totFaults > 0) {
	quickPercent = 100.0 * ((float)diffStat.quickCOWFaults / totFaults);
    } else {
	quickPercent = 0;
    }
    fprintf(stream, 
	    "COW: Heap (%d/%d)=%d%% Stk (%d/%d)=%d%% Tot (%d/%d)=%d%%\n",
	    diffStat.numCOWHeapFaults, diffStat.numCOWHeapPages, heapPercent,
	    diffStat.numCOWStkFaults, diffStat.numCOWStkPages, stkPercent,
	    totFaults, totPages, totPercent);
    fprintf(stream, "     Quick (%d/%d)=%d%%\n",
	    diffStat.quickCOWFaults, totFaults, quickPercent);
    /*
     * Copy on reference.
     */
    totPages = diffStat.numCORStkPages + diffStat.numCORHeapPages;
    totFaults = diffStat.numCORStkFaults + diffStat.numCORHeapFaults;
    if (diffStat.numCORHeapPages > 0) {
	heapPercent = 100.0 * ((float)diffStat.numCORHeapFaults / 
				      diffStat.numCORHeapPages);
    } else {
	heapPercent = 0;
    }
    if (diffStat.numCORStkPages > 0) {
	stkPercent = 100.0 * ((float)diffStat.numCORStkFaults / 
				      diffStat.numCORStkPages);
    } else {
	stkPercent = 0;
    }
    if (totPages > 0) {
	totPercent = 100.0 * ((float)totFaults / totPages);
    } else {
	totPercent = 0;
    }
    fprintf(stream,
            "COR: Heap (%d/%d)=%d%% Stk (%d/%d)=%d%% Tot (%d/%d)=%d%%\n",
	    diffStat.numCORHeapFaults, diffStat.numCORHeapPages, heapPercent,
	    diffStat.numCORStkFaults, diffStat.numCORStkPages, stkPercent,
	    totFaults, totPages, totPercent);
    totPages = diffStat.numCORStkFaults + diffStat.numCORHeapFaults;
    totFaults = diffStat.numCORCOWStkFaults + diffStat.numCORCOWHeapFaults;
    if (diffStat.numCORCOWHeapFaults > 0) {
	heapPercent = 100.0 * ((float)diffStat.numCORCOWHeapFaults / 
				      diffStat.numCORHeapFaults);
    } else {
	heapPercent = 0;
    }
    if (diffStat.numCORCOWStkFaults > 0) {
	stkPercent = 100.0 * ((float)diffStat.numCORCOWStkFaults / 
				      diffStat.numCORStkFaults);
    } else {
	stkPercent = 0;
    }
    if (totPages > 0) {
	totPercent = 100.0 * ((float)totFaults / totPages);
    } else {
	totPercent = 0;
    }
    fprintf(stream,
            "COR-mod: Heap(%d/%d)=%d%% Stk (%d/%d)=%d%% Tot (%d/%d)=%d%%\n",
	    diffStat.numCORCOWHeapFaults, diffStat.numCORHeapFaults,heapPercent,
	    diffStat.numCORCOWStkFaults, diffStat.numCORStkFaults, stkPercent,
	    diffStat.numCORCOWHeapFaults + diffStat.numCORCOWStkFaults,
	    diffStat.numCORHeapFaults + diffStat.numCORStkFaults, totPercent);

    fprintf(stream, "Swap pages copied: %d\n", diffStat.swapPagesCopied);
    fprintf(stream,
             "Vm allocs: %d (Free=%d From-FS=%d From-alloc-list=%d)\n",
	     diffStat.numAllocs, diffStat.gotFreePage, diffStat.gotPageFromFS, 
	     diffStat.pageAllocs);
    fprintf(stream, 
	     "VM-FS stats: Asked=%d Free-pages=%d Allocs=%d Frees=%d\n",
	     diffStat.fsAsked, diffStat.haveFreePage, diffStat.fsMap, 
	     diffStat.fsUnmap);
    fprintf(stream, "Alloc-list searches: %d (Free=%d In-use=%d)\n",
	     diffStat.numListSearches, diffStat.usedFreePage, 
	     diffStat.numListSearches - diffStat.usedFreePage);
    fprintf(stream, "Extra-searches: %d (Lock=%d Ref=%d Dirty=%d)\n",
	     diffStat.lockSearched + diffStat.refSearched + 
	     diffStat.dirtySearched,
	     diffStat.lockSearched, diffStat.refSearched, 
	     diffStat.dirtySearched);
    fprintf(stream, "Pages written %d\n", diffStat.pagesWritten);

    totPrefetches = diffStat.codePrefetches + diffStat.heapFSPrefetches +
		    diffStat.heapSwapPrefetches + diffStat.stackPrefetches;
    if (totPrefetches > 0) {
	totHits = diffStat.codePrefetchHits + diffStat.heapFSPrefetchHits +
		  diffStat.heapSwapPrefetchHits + diffStat.stackPrefetchHits;
	fprintf(stream, "Prefetch stats:\n");
	if (diffStat.codePrefetches > 0) {
	    hitPct = 100 * ((float)diffStat.codePrefetchHits / 
			    (float)diffStat.codePrefetches);
	    fprintf(stream, "    code (%d/%d)=%d%%\n",
		    diffStat.codePrefetchHits, diffStat.codePrefetches, hitPct);
	}
	if (diffStat.heapFSPrefetches > 0) {
	    hitPct = 100 * ((float)diffStat.heapFSPrefetchHits / 
			    (float)diffStat.heapFSPrefetches);
	    fprintf(stream, "    heap-fs (%d/%d)=%d%%\n",
		diffStat.heapFSPrefetchHits, diffStat.heapFSPrefetches, hitPct);
	}
	if (diffStat.heapSwapPrefetches > 0) {
	    hitPct = 100 * ((float)diffStat.heapSwapPrefetchHits / 
			    (float)diffStat.heapSwapPrefetches);
	    fprintf(stream, "    heap-swp (%d/%d)=%d%%\n",
		diffStat.heapSwapPrefetchHits, diffStat.heapSwapPrefetches, 
		hitPct);
	}
	if (diffStat.stackPrefetches > 0) {
	    hitPct = 100 * ((float)diffStat.stackPrefetchHits / 
			    (float)diffStat.stackPrefetches);
	    fprintf(stream, "    stack (%d/%d)=%d%%\n",
		diffStat.stackPrefetchHits, diffStat.stackPrefetches, hitPct);
	}
	hitPct = 100 * ((float)totHits / (float)totPrefetches);
	fprintf(stream, "    total (%d/%d)=%d%%\n",
		totHits, totPrefetches, hitPct);
	fprintf(stream, "    aborts=   %d\n", diffStat.prefetchAborts);
    }
}

