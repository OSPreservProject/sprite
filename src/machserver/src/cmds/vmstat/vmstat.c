/* 
 * vmstat.c --
 *
 *	Statistics generation for the virtual memory module.
 *
 * Copyright 1986, 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/cmds/vmstat/RCS/vmstat.c,v 1.13 92/07/13 14:09:44 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <bstring.h>
#include <vm.h>
#include <vmStat.h>
#include <option.h>
#include <spriteTime.h>
#include <stdio.h>
#include "vmTotals.h"

/*
 * Flags to command-line options:
 */
int	checkTime = -1;			/* If != -1 indicates how often to
					 * print a line of stats.  0 means 
					 * just print one line and quit, 
					 * non-zero means print a line
					 * every checkTime seconds. */
int	printSegInfo = 0;		/* Non-zero means print info for
					 * all segments. */
int	whenToPrintHdr = 25;		/* Number of lines of stats after 
					 * which to print the header. */
int	faultThreshold = 0;		/* Number of page faults after which
					 * to print line of stats. */
int	pageOutThreshold = 0;		/* Number of page outs after which
					 * to print line of stats. */
int	relToLastPrint = 0;		/* Non-zero means print stats relative
					 * to last time printed, not last
					 * interval of lengh checkTime. */
int	printMod = 0;			/* Non-zero means replace printing out
					 * of kernel stack pages with number
					 * of modified pages in memory. */
int	maxTimes = -1;			/* Maximum number can skip printing
					 * a line of info because of 
					 * faultThreshold or pageOutThreshold.*/
int	zeroCounters = 0;		/* Reset counters to zero */
#ifdef DO_VERBOSE
int	verbose = 0;			/* Print extra more obscure vm 
					 * stats. */
#endif

Option optionArray[] = {
#ifdef DO_SEG_INFO
    {OPT_TRUE, "s", (Address)&printSegInfo,
	"Print out information about all in-use segments"},
#endif
#ifdef DO_VERBOSE
    {OPT_TRUE, "v", (Address)&verbose,
	"Print out extra, more obscure vm statistics"},
#endif
    {OPT_INT, "t", (Address)&checkTime,
	"Print out incremental vm info at this interval"},
    {OPT_INT, "T", (Address)&maxTimes,
	"Maximum times skip printing because of -f or -p flags. Used with -t."},
    {OPT_INT, "l", (Address)&whenToPrintHdr,
      "Lines to print before should print header (default: 25). Used with -t."},
#ifdef DO_FAULT_THRESHOLD
    {OPT_INT, "f", (Address)&faultThreshold, 
   "Page faults per second before print out info (default: 0).  Used with -t."},
#endif
#ifdef DO_PAGEOUT_THRESHOLD
    {OPT_INT, "p", (Address) &pageOutThreshold,
    "Page outs per second before print out info (default: 0).  Used with -t."},
#endif
    {OPT_TRUE, "P", (Address)&relToLastPrint,
 "Print out all info since last print, not since last interval. Used with -t."},
#ifdef DO_MOD_PAGES
    {OPT_TRUE, "m", (Address)&printMod,
    "Print out number of modified pages, not kern stack pages.  Used with -t."},
#endif
    {OPT_TRUE, "z", (Address)&zeroCounters,
	 "Reset the current VM counters to zero."},
};
int	numOptions = Opt_Number(optionArray);

Vm_Stat 	vmStat;
Vm_Stat		prevStat;	/* previous copy of vmStat */

int	totalPagesRead;		/* sum of vmStat.pagesRead counts */
int	totalPartialPagesRead;	/* etc. */
int	totalPagesZeroed;
int	totalPagesWritten;
int	totalPagesCleaned;
int	prevPagesRead;		/* previous totalPagesRead */
int	prevPartialPagesRead;	/* etc. */
int	prevPagesZeroed;
int	prevPagesWritten;

int		kbPerPage;

/* forward references: */

static void PutTime();
static void PutPageCounts();
static void PutPageTimes();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	The main program for vmstat.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints information on standard output.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    int		pageSize;

    (void)Opt_Parse(argc, argv, optionArray, numOptions, 0);
    (void)Vm_PageSize(&pageSize);
    kbPerPage = pageSize / 1024;

    if (zeroCounters) {
	(void)Vm_Cmd(VM_CLEAR_COUNTERS, 0, NULL);
    } else if (printSegInfo) {
	PrintSegInfo();
    } else if (checkTime == 0) {
	PrintWide();
	printf("\n");
    } else if (checkTime == -1) {
	PrintSummary();
    } else {
	while (1) {
	    PrintWide();
	    sleep(checkTime);
	}
    }
    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintSegInfo --
 *
 *	Print vm info about all segments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints information on standard output.
 *
 *----------------------------------------------------------------------
 */
PrintSegInfo()
{
}


/*
 *----------------------------------------------------------------------
 *
 * PrintWide --
 *
 *	Print a one summary of vm stats.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints information on standard output.
 *
 *----------------------------------------------------------------------
 */
PrintWide()
{
    static	int	numLines = 0;
    static	int	numCalls = 0;

    (void)Vm_Cmd(VM_GET_STATS, sizeof(vmStat), &vmStat);

#if defined(DO_FAULT_THRESHOLD) || defined(DO_PAGEOUT_THRESHOLD)
    if (checkTime != 0 && numCalls != maxTimes) {
	if ((vmStat.totalFaults - prevStat.totalFaults) / checkTime < 
		faultThreshold &&
	    (vmStat.pagesWritten - prevStat.pagesWritten) / checkTime < 
	        pageOutThreshold) {
	    if (relToLastPrint) {
		bcopy((Address)&vmStat, (Address)&prevStat, sizeof(Vm_Stat));
	    }
	    numCalls++;
	    return;
	}
    }
#endif

    TotalPageCounts(&vmStat, &totalPagesRead, &totalPartialPagesRead,
	       &totalPagesZeroed, &totalPagesWritten, &totalPagesCleaned);
    numCalls = 0;
    if (numLines % whenToPrintHdr == 0) {
	if (numLines > 0) {
	    printf("\n");
	}
	printf("%-8s%-8s%-8s%-8s%-8s%-8s",
	       "  PGS RD", " PGS ZRD", " PGS WRT", " SEG CRT", " SEG DES",
	       " S LKUPS");
    }
    numLines++;
    printf("\n%8d%8d%8d%8d%8d%8d",
	   (totalPagesRead - prevPagesRead +
	    totalPartialPagesRead - prevPartialPagesRead),
	   totalPagesZeroed - prevPagesZeroed,
	   totalPagesWritten - prevPagesWritten,
	   vmStat.segmentsCreated - prevStat.segmentsCreated,
	   vmStat.segmentsDestroyed - prevStat.segmentsDestroyed,
	   vmStat.segmentLookups - prevStat.segmentLookups);

    bcopy((Address)&vmStat, (Address)&prevStat, sizeof(Vm_Stat));
    prevPagesRead = totalPagesRead;
    prevPartialPagesRead = totalPartialPagesRead;
    prevPagesZeroed = totalPagesZeroed;
    prevPagesWritten = totalPagesWritten;
    fflush(stdout);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintSummary --
 *
 *	Print a verbose summary of all vm statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints information on standard output.
 *
 *----------------------------------------------------------------------
 */
PrintSummary()
{
    char timeBuf[1024];
    Time totalReadTime;		/* sum of Vm_Stat page read times */
    Time totalReadCopyTime;	/* etc. */
    Time totalWriteTime;

    (void)Vm_Cmd(VM_GET_STATS, sizeof(vmStat), &vmStat);
    TotalPageCounts(&vmStat, &totalPagesRead, &totalPartialPagesRead,
		    &totalPagesZeroed, &totalPagesWritten,
		    &totalPagesCleaned);
    TotalPageTimes(&vmStat, &totalReadTime, &totalReadCopyTime,
		   &totalWriteTime);
    printf("MEMORY STATS:\n");
    printf("\tPage Size:\t%-8d\n", 1024 * kbPerPage);
    printf("\tMemory Size:\t%-8d\n", vmStat.numPhysPages * kbPerPage);
    printf("\tSync calls:\t%-8d\n", vmStat.syncCalls);

    printf("PAGER STATS:\n");
    printf("  Full pages reads: %d\n", totalPagesRead);
    PutPageCounts(vmStat.pagesRead);
    printf("  Partial pages reads: %d\n", totalPartialPagesRead);
    PutPageCounts(vmStat.partialPagesRead);
    printf("  Pages zero-filled: %d\n", totalPagesZeroed);
    PutPageCounts(vmStat.pagesZeroed);
    printf("  Read request time: %d.%-03d\n    Avg:\t%.2f ms\n",
	   totalReadTime.seconds,
	   (totalReadTime.microseconds + 500) / 1000,
	   (float)Time_Average(Time_ToMs(totalReadTime),
			       (totalPagesRead + totalPartialPagesRead + 
				totalPagesZeroed)));
    PutPageTimes(vmStat.readTime);
    printf("  Read copy time: %d.%-03d\n",
	   totalReadCopyTime.seconds,
	   (totalReadCopyTime.microseconds + 500) / 1000);
    PutPageTimes(vmStat.readCopyTime);
    printf("  Pages written: %d\n", totalPagesWritten);
    PutPageCounts(vmStat.pagesWritten);
    printf("  Pages cleaned: %d\n", totalPagesCleaned);
    PutPageCounts(vmStat.pagesCleaned);
    printf("  Write request time: %d.%-03d\n    Avg:\t%.2f ms\n",
	   totalWriteTime.seconds,
	   (totalWriteTime.microseconds + 500) / 1000,
	   (float)Time_Average(Time_ToMs(totalWriteTime),
			       totalPagesWritten));
    PutPageTimes(vmStat.writeTime);
    printf("  init: %d calls, %d.%-03d sec (%.2f ms avg.)\n",
	   vmStat.initCalls, vmStat.initTime.seconds,
	   (vmStat.initTime.microseconds + 500) / 1000,
	   (float)Time_Average(Time_ToMs(vmStat.initTime),
			       vmStat.initCalls));
    printf("    forced init: %d calls\n", vmStat.forcedInits++);
    printf("  terminate: %d calls, %d.%-03d sec (%.2f ms avg.)\n",
	   vmStat.terminateCalls, vmStat.terminateTime.seconds,
	   (vmStat.terminateTime.microseconds + 500) / 1000,
	   (float)Time_Average(Time_ToMs(vmStat.terminateTime),
			       vmStat.terminateCalls));
    printf("  return calls: %d\n", vmStat.returnCalls);
    printf("  lock complete: %d calls, %d.%-03d sec (%.2f ms avg.)\n",
	   vmStat.lockCompletedCalls, vmStat.lockCompletedTime.seconds,
	   (vmStat.lockCompletedTime.microseconds + 500) / 1000,
	   (float)Time_Average(Time_ToMs(vmStat.lockCompletedTime),
			       vmStat.lockCompletedCalls));

    printf("SEGMENT STATS:\n");
    printf("\tSegments created:\t%-8d\tDestroyed:\t%-8d\n",
	   vmStat.segmentsCreated, vmStat.segmentsDestroyed);
    printf("\t\tSegments needlessly destroyed: %d\n",
	   vmStat.segmentsNeedlesslyDestroyed);
    printf("\t\tPages wasted: %d\n", vmStat.swapPagesWasted);
    printf("\tQueue overflows:\t%-8d\n", vmStat.queueOverflows);
    printf("\tSegment lookups:\t%-8d\n", vmStat.segmentLookups);
    printf("\tSegments looked at:\t%-8d\tAvg per lookup:\t%-8.2f\n",
	   vmStat.segmentsLookedAt,
	   (vmStat.segmentLookups != 0
	    ? (float)vmStat.segmentsLookedAt / vmStat.segmentLookups
	    : 0.));
    printf("\tSegments copied: %-8d\tPages copied: %-8d\n",
	   vmStat.segmentCopies, vmStat.pagesCopied);
    printf("\tAvg. init file: %-8.2f pages\tAvg. swap file: %-8.2f pages\n",
	   (float)vmStat.objPagesCopied / vmStat.segmentCopies,
	   (float)vmStat.swapPagesCopied / vmStat.segmentCopies);
    Time_ToAscii(vmStat.forkTime.seconds, TRUE, timeBuf);
    printf("\tTime in Vm_Fork: %s.%03d    Avg per fork:\t%-8.2f msec\n",
	   timeBuf,
	   (vmStat.forkTime.microseconds+500)/1000,
	   (float)Time_Average(Time_ToMs(vmStat.forkTime),
			       vmStat.segmentCopies / 2));
    printf("\t\tvm_region: %d.%03d (%-8.2f msec avg)\n",
	   vmStat.findRegionTime.seconds,
	   (vmStat.findRegionTime.microseconds+500)/1000,
	   (float)Time_Average(Time_ToMs(vmStat.findRegionTime),
			       vmStat.segmentCopies / 2));
    printf("\t\tVmSegByName: %d.%03d (%-8.2f msec avg)\n",
	   vmStat.segLookupTime.seconds,
	   (vmStat.segLookupTime.microseconds+500)/1000,
	   (float)Time_Average(Time_ToMs(vmStat.segLookupTime),
			       vmStat.segmentCopies / 2));
    printf("\t\tVmSegmentCopy: %d.%03d (%-8.2f msec avg)\n",
	   vmStat.segCopyTime.seconds,
	   (vmStat.segCopyTime.microseconds+500)/1000,
	   (float)Time_Average(Time_ToMs(vmStat.segCopyTime),
			       vmStat.segmentCopies / 2));
    printf("\t\tCopyRegion: %d.%03d (%-8.2f msec avg)\n",
	   vmStat.regionCopyTime.seconds,
	   (vmStat.regionCopyTime.microseconds+500)/1000,
	   (float)Time_Average(Time_ToMs(vmStat.regionCopyTime),
			       vmStat.segmentCopies / 2));
    printf("\tSource faults during copy: %d zero %d swap\n",
	   vmStat.sourceCopyZeroed, vmStat.sourceCopyRead);
    printf("\tTarget faults during copy: %d zero %d swap\n",
	   vmStat.targetCopyZeroed, vmStat.targetCopyRead);
}


/*
 *----------------------------------------------------------------------
 *
 * PutPageCounts --
 *
 *	Write out each of the per-segment-type page counts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PutPageCounts(countsPtr)
    int *countsPtr;
{
    if (VM_NUM_SEGMENT_TYPES != 5) {
	fprintf(stderr, "vmstat: how many segment types are there?\n");
    }
    printf("%8d sys %8d code %8d heap %8d stack %8d shared\n",
	   countsPtr[VM_SYSTEM], countsPtr[VM_CODE], countsPtr[VM_HEAP],
	   countsPtr[VM_STACK], countsPtr[VM_SHARED]);
}


/*
 *----------------------------------------------------------------------
 *
 * PutPageTimes --
 *
 *	Write out each of the per-segment-type times.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PutPageTimes(timesPtr)
    Time *timesPtr;
{
    PutTime(timesPtr[VM_SYSTEM]);
    printf(" sys ");
    PutTime(timesPtr[VM_CODE]);
    printf(" code ");
    PutTime(timesPtr[VM_HEAP]);
    printf(" heap ");
    PutTime(timesPtr[VM_STACK]);
    printf(" stack ");
    PutTime(timesPtr[VM_SHARED]);
    printf(" shared\n");
}


/*
 *----------------------------------------------------------------------
 *
 * PutTime --
 *
 *	Print out a single time value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PutTime(aTime)
    Time aTime;
{
    printf("%4d.%-03d", aTime.seconds, (aTime.microseconds + 500) / 1000);
}
