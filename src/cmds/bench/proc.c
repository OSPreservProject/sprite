/* 
 * proc.c --
 *
 *	Snapshot the process table before and after a benchmark in order
 *	to account time spent in different processes and the kernel.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /a/newcmds/bench/RCS/proc.c,v 1.2 89/02/22 11:37:25 jhh Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <proc.h>
#include <stdio.h>

void DoOneTime();
void DoDeltaTime();


/*
 *----------------------------------------------------------------------
 *
 * GetProcTable --
 *
 *	Take a snapshot of the process table.
 *
 * Results:
 *	The number of valid entries.
 *
 * Side effects:
 *	Fills in the proc table that is passed in.
 *
 *----------------------------------------------------------------------
 */

int
GetProcTable(num, pcbs, argStrings)
    int num;				/* Size of tables passed in */
    Proc_PCBInfo *pcbs;			/* Array of PCBs */
    Proc_PCBArgString *argStrings;	/* Array of argument strings */
{
    register int status;

    status = Proc_GetPCBInfo(0, num - 1, PROC_MY_HOSTID, sizeof(Proc_PCBInfo),
			    pcbs, argStrings, &num);
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't read process table: %s\n",
		Stat_GetMsg(status));
	exit(1);
    }
    return(num);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintProcStats --
 *
 *	Compute how much time has been spent by all existing processes
 *	between two snapshots.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints out info.
 *
 *----------------------------------------------------------------------
 */

void
PrintProcStats(outStream, numPCB1, pcbs1, argStrings1, numPCB2, pcbs2, argStrings2)
    FILE *outStream;
    int numPCB1;
    Proc_PCBInfo *pcbs1;		/* Initial PCB snapshot */
    Proc_PCBArgString *argStrings1;	/* 	and argument strings */
    int numPCB2;
    Proc_PCBInfo *pcbs2;		/* Final PCB snapshot */
    Proc_PCBArgString *argStrings2;	/* 	and argument strings */
{
    register Proc_PCBInfo *pcb1Ptr;
    register Proc_PCBInfo *pcb2Ptr;
    register int i;
    Time sumUserTime;
    Time sumKernelTime;

    sumUserTime.seconds = 0;
    sumUserTime.microseconds = 0;
    sumKernelTime.seconds = 0;
    sumKernelTime.microseconds = 0;
    /*
     * Loop through final snapshot.  For each entry either subtract
     * out times indicated in the initial snapshot, or print out
     * the total time because the process started after the benchmark did.
     */
    for (i=0, pcb1Ptr = pcbs1, pcb2Ptr = pcbs2 ;
	 i<numPCB2 ;
	 i++, pcb1Ptr++, pcb2Ptr++) {
#ifdef notdef
	/*
	 * HACK BECAUSE proc.h IS CHANGED.
	 */
	if (i != 0) {
	    pcb1Ptr = (Proc_PCBInfo *)((int)pcb1Ptr - 5 * sizeof(int));
	    pcb2Ptr = (Proc_PCBInfo *)((int)pcb2Ptr - 5 * sizeof(int));
	}
	/* END HACK */
#endif notdef
	if (pcb2Ptr->state == PROC_UNUSED) {
	    continue;
	}
	if (NoTimeSpent(pcb2Ptr, pcb1Ptr)) {
	    continue;
	}
	if (i < numPCB1 && pcb2Ptr->processID == pcb1Ptr->processID) {
	    DoDeltaTime(outStream, "U",
		&pcb2Ptr->userCpuUsage,
		&pcb2Ptr->childUserCpuUsage,
		&pcb1Ptr->userCpuUsage,
		&pcb1Ptr->childUserCpuUsage,
		&sumUserTime);
	    DoDeltaTime(outStream, "S",
		&pcb2Ptr->kernelCpuUsage,
		&pcb2Ptr->childKernelCpuUsage,
		&pcb1Ptr->kernelCpuUsage,
		&pcb1Ptr->childKernelCpuUsage,
		&sumKernelTime);
	} else {
	    DoOneTime(outStream, "U",
		&pcb2Ptr->userCpuUsage,
		&pcb2Ptr->childUserCpuUsage,
		&sumUserTime);
	    DoOneTime(outStream, "S",
		&pcb2Ptr->kernelCpuUsage,
		&pcb2Ptr->childKernelCpuUsage,
		&sumKernelTime);
	}
	if (strlen(argStrings2[i].argString) > 58) {
	    argStrings2[i].argString[58] = '\0';
	}
	fprintf(outStream, "%s\n", argStrings2[i].argString);
    }
    fprintf(outStream, "Total User %6d.%06d Total Kernel %6d.%06d\n",
	    sumUserTime.seconds, sumUserTime.microseconds,
	    sumKernelTime.seconds, sumKernelTime.microseconds);
}

/*
 *----------------------------------------------------------------------
 *
 * NoTimeSpent --
 *
 *	See if this process spent any time during the benchmark.
 *
 * Results:
 *	TRUE if the process built up NO time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
NoTimeSpent(pcb2Ptr, pcb1Ptr)
    register Proc_PCBInfo *pcb2Ptr;
    register Proc_PCBInfo *pcb1Ptr;
{
    Time endTime, startTime;
    if (pcb2Ptr->processID == pcb1Ptr->processID) {
	Time_Add(pcb2Ptr->userCpuUsage, pcb2Ptr->childUserCpuUsage,
		&endTime);
	Time_Add(pcb1Ptr->userCpuUsage, pcb1Ptr->childUserCpuUsage,
		&startTime);
	Time_Subtract(endTime, startTime, &endTime);
	if (endTime.seconds + endTime.microseconds != 0) {
	    return(0);	/* Have User CPU usage */
	}
	Time_Add(pcb2Ptr->kernelCpuUsage,
		pcb2Ptr->childKernelCpuUsage, &endTime);
	Time_Add(pcb1Ptr->kernelCpuUsage,
		pcb1Ptr->childKernelCpuUsage, &startTime);
	Time_Subtract(endTime, startTime, &endTime);
	if (endTime.seconds + endTime.microseconds != 0) {
	    return(0);	/* Have Kernel CPU usage */
	}
	return(1);	/* No time */
    } else {
	Time_Add(pcb2Ptr->userCpuUsage, pcb2Ptr->childUserCpuUsage,
		&endTime);
	Time_Add(pcb2Ptr->kernelCpuUsage,
		pcb2Ptr->childKernelCpuUsage, &startTime);
	Time_Add(startTime, endTime, &endTime);
	if (endTime.seconds + endTime.microseconds != 0) {
	    return(0);	/* Have CPU usage */
	}
	return(1);	/* No time */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DoDeltaTime --
 *
 *	This computes the time spent in a process and its children
 *	given the ending and starting times.  It prints the result
 *	and accumulates it in a running total.
 *
 * Results:
 *	Time time computed.
 *
 * Side effects:
 *	Add the time into a total.
 *
 *----------------------------------------------------------------------
 */

void
DoDeltaTime(outStream, string, endTimePtr, endChildTimePtr,
	    startTimePtr, startChildTimePtr, totalTimePtr)
    FILE *outStream;
    char *string;
    register Time *endTimePtr;
    register Time *endChildTimePtr;
    register Time *startTimePtr;
    register Time *startChildTimePtr;
    register Time *totalTimePtr;
{
    register int seconds;
    register int microseconds;

    seconds = endTimePtr->seconds - startTimePtr->seconds +
	      endChildTimePtr->seconds - startChildTimePtr->seconds;
    microseconds = endTimePtr->microseconds - startTimePtr->microseconds +
	     endChildTimePtr->microseconds - startChildTimePtr->microseconds;
    while (microseconds < 0) {
	microseconds += 1000000;
	seconds -= 1;
    }
    fprintf(outStream, "%s %2d.%03d ", string, seconds, microseconds / 1000);
    totalTimePtr->microseconds += microseconds;
    if (totalTimePtr->microseconds > 1000000) {
	totalTimePtr->microseconds -= 1000000;
	seconds += 1;
    }
    totalTimePtr->seconds += seconds;
}

/*
 *----------------------------------------------------------------------
 *
 * DoOneTime --
 *
 *	Add a process's time to its children's time, then print the
 *	result and accumulate it into a total.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Accumulate the time in a total.
 *
 *----------------------------------------------------------------------
 */

void
DoOneTime(outStream, string, endTimePtr, endChildTimePtr, totalTimePtr)
    FILE *outStream;
    char *string;
    register Time *endTimePtr;
    register Time *endChildTimePtr;
    register Time *totalTimePtr;
{
    register int seconds;
    register int microseconds;

    seconds = endTimePtr->seconds + endChildTimePtr->seconds;
    microseconds = endTimePtr->microseconds + endChildTimePtr->microseconds;
    if (microseconds > 1000000) {
	microseconds -= 1000000;
	seconds += 1;
    }
    fprintf(outStream, "%s %2d.%03d ", string, seconds, microseconds / 1000);
    totalTimePtr->microseconds += microseconds;
    if (totalTimePtr->microseconds > 1000000) {
	totalTimePtr->microseconds -= 1000000;
	seconds += 1;
    }
    totalTimePtr->seconds += seconds;
}

