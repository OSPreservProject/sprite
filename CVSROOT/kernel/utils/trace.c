/* 
 * trace.c --
 *
 *	This files implements a generalized tracing facility.  A Trace_Buffer
 *	contains information about the number and size of the elements in a
 *	circular buffer that is dynamically allocated by Trace_Init.  Calls
 *	to Trace_Insert add a time-stamped trace record to the buffer.
 *	A circular buffer of trace records can be dumped to a file via calls
 *	to Trace_Dump.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "trace.h"
#include "timer.h"
#include "mem.h"
#include "byte.h"
#include "sys.h"


/*
 *----------------------------------------------------------------------
 *
 * Trace_Init --
 *
 *	Allocate and initialize a circular trace buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated for the individual buffer records.  Note 
 *	that the Trace_Header is assumed to be allocated
 *	(statically or dynamically) by the calling routine, and its values
 *	are initialized in this routine.
 *
 *----------------------------------------------------------------------
 */

void
Trace_Init(traceHdrPtr, numRecords, size, flags)
    register Trace_Header *traceHdrPtr;	/* pointer to tracing info */
    int numRecords;			/* number of records to allocate */
    int size;				/* size of each client-specific area */
    int flags;				/* TRACE_NO_TIMES */
{
    register Address clientPtr;
    register Trace_Record *recordPtr;
    int i;
    
    traceHdrPtr->numRecords = numRecords;
    traceHdrPtr->currentRecord = 0;
    traceHdrPtr->flags = flags & ~TRACE_INHIBIT;
    traceHdrPtr->dataSize = size;

    recordPtr = (Trace_Record *) Mem_Alloc(numRecords * sizeof(Trace_Record));
    traceHdrPtr->recordArray = recordPtr;

    if (size > 0) {
	clientPtr = Mem_Alloc(numRecords * size);
    }
    for (i = 0; i < numRecords; i++) {
	recordPtr[i].flags = TRACE_UNUSED;
	recordPtr[i].event = NIL;
	if (size > 0) {
	    recordPtr[i].traceData = (ClientData *) clientPtr;
	    clientPtr += size;
	} else {
	    recordPtr[i].traceData = (ClientData *) NIL;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Trace_Insert --
 *
 *	Save a time stamp and any client-specific data in a circular buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Record the information in a trace record and advance the
 *	circular buffer pointer.
 *
 *----------------------------------------------------------------------
 */
void
Trace_Insert(traceHdrPtr, event, dataPtr)
    Trace_Header *traceHdrPtr;
    int event;
    ClientData *dataPtr;
{
    Trace_Record *recordPtr;
    int size;
    Timer_Ticks ticks;
    
    if (traceHdrPtr == (Trace_Header *)NIL ||
	(traceHdrPtr->flags & TRACE_INHIBIT)) {
	return;
    }

    recordPtr = &(traceHdrPtr->recordArray[traceHdrPtr->currentRecord]);
    if ( ! (traceHdrPtr->flags & TRACE_NO_TIMES)) {
	Timer_GetCurrentTicks(&ticks);
	Timer_TicksToTime(ticks, &(recordPtr->time));
    }
    size = traceHdrPtr->dataSize;
    if ((size > 0) && (dataPtr != (ClientData *) NIL)) {
	Byte_Copy(size, (Address) dataPtr, (Address) recordPtr->traceData);
	recordPtr->flags = TRACE_DATA_VALID;
    } else {
	if (recordPtr->traceData != (ClientData *) NIL) {
	    Byte_Zero(size, (Address) recordPtr->traceData);
	}
	recordPtr->flags = TRACE_DATA_INVALID;
    }
    recordPtr->event = event;
    traceHdrPtr->currentRecord =
	    (traceHdrPtr->currentRecord + 1) % traceHdrPtr->numRecords;
}


/*
 *----------------------------------------------------------------------
 *
 * Trace_Dump --
 *
 *	Dump trace records into the user's address space.  Data is copied
 *	in the following order:
 *
 *	(1) The number of records being copied is copied.
 *	(2) numRecs Trace_Records are copied.
 *	(3) If traceData is non-NIL, numRecs records of traceData are copied.
 *
 * Results:
 *	The result from Vm_CopyOut is returned, in addition to the
 *	data specified above.
 *
 * Side effects:
 *	Data is copied out to the user's address space.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Trace_Dump(traceHdrPtr, numRecs, addr)
    register Trace_Header *traceHdrPtr;
    int numRecs;
    Address addr;
{
    ReturnStatus status;
    int size;
    int earlyRecs = 0;
    int lateRecs;
    int current;

    if (traceHdrPtr == (Trace_Header *) NIL) {
	Sys_Panic(SYS_WARNING, "Trace_Dump: trace buffer not initialized.\n");
	numRecs = 0;
	status = Vm_CopyOut(sizeof(int), (Address) &numRecs, addr);
	return(status);
    }

    traceHdrPtr->flags |= TRACE_INHIBIT;

    if (numRecs > traceHdrPtr->numRecords) {
	numRecs = traceHdrPtr->numRecords;
    }

    /*
     * Check the current record to see if it's unused.  If so, then all
     * records before it are valid and all records after it are invalid,
     * since once we wrap around once all records are used forever.
     */
    
    current = traceHdrPtr->currentRecord;
    lateRecs = traceHdrPtr->numRecords - current;
    if (traceHdrPtr->recordArray[current].flags == TRACE_UNUSED) {
	numRecs = current;
	earlyRecs = current;
	lateRecs = 0;
    } else if (numRecs > lateRecs) {
	earlyRecs = numRecs - lateRecs;
    }
    
    status = Vm_CopyOut(sizeof(int), (Address) &numRecs, addr);
    if (status != SUCCESS || numRecs == 0) {
	goto done;
    }
    addr += sizeof(int);

    /*
     * Copy the general trace records.  First copy the ones from "current"
     * to the end of the buffer, then copy the ones at the front.
     */

    if (lateRecs > 0) {
	size = lateRecs * sizeof(Trace_Record);
	status = Vm_CopyOut(size, (Address) &traceHdrPtr->recordArray[current],
			    addr);
	if (status != SUCCESS) {
	    goto done;
	}
	addr += size;
    }
    if (earlyRecs > 0) {
	size = earlyRecs * sizeof(Trace_Record);
	status = Vm_CopyOut(size, (Address) &traceHdrPtr->recordArray[0],
			    addr);
	if (status != SUCCESS) {
	    return(status);
	}
	addr += size;
    }

    /*
     * Copy the client-specific data, if there is any.
     */
    
    if (traceHdrPtr->dataSize == 0) {
	goto done;
    }
    if (lateRecs > 0) {
	size = lateRecs * traceHdrPtr->dataSize;
	status = Vm_CopyOut(size, (Address)
			      traceHdrPtr->recordArray[current].traceData,
			    addr);
	if (status != SUCCESS) {
	    goto done;
	}
	addr += size;
    }
    if (earlyRecs > 0) {
	size = earlyRecs * traceHdrPtr->dataSize;
	status = Vm_CopyOut(size,
			    (Address) traceHdrPtr->recordArray[0].traceData,
			    addr);
	if (status != SUCCESS) {
	    goto done;
	}
	addr += size;
    }

done:
    traceHdrPtr->flags &= ~TRACE_INHIBIT;
    return(SUCCESS);
    
}

/*
 *----------------------------------------------------------------------
 *
 * Trace_Print --
 *
 *	Print trace records using a client's printing procedure to
 *	format the client data.  The N most recent records are displayed.
 *	The interface to the printing procedure is:
 *	(*printRecord)(clientData, printHeaderFlag)
 *		ClientData clientData;
 *		Boolean printHeaderFlag;
 *	where the flag should cause column headers to be printed.
 *	Careful, ClientData might be NIL if the flag is TRUE.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Sys_Printf's to the screen.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Trace_Print(traceHdrPtr, numRecs, printProc)
    register Trace_Header *traceHdrPtr;	/* Trace record */
    int numRecs;		/* Number of most recent records to print */
    int (*printProc)();		/* See above doc. for this call-back */
{
    register int traceLength;
/*    int lastRec; */		/* Index of last record to be printed */
/*    int firstRec; */		/* Index of first record to be printed */
    int index;
/*    Boolean printHeader; */	/* Passed to output routine */
    Time deltaTime;		/* Time difference between trace records */
    Time baseTime;		/* Used to calculate deltaTime */
    Trace_Record *recordPtr;

    traceHdrPtr->flags |= TRACE_INHIBIT;

    baseTime.seconds = 0;
    baseTime.microseconds = 0;
    /*
     * Print the header.
     * Loop through the records starting numRecs modulo-before the end.
     * Print the header.
     */
    (*printProc)((ClientData *)NIL, TRUE);
    traceLength = traceHdrPtr->numRecords;
    if (numRecs > traceLength) {
	numRecs = traceLength;
    }
    index = (traceHdrPtr->currentRecord - numRecs + traceLength)
		% traceLength;
    for ( ; numRecs > 0; numRecs--, index = (index + 1) % traceLength) {
	recordPtr = &traceHdrPtr->recordArray[index];
	if (recordPtr->flags & TRACE_UNUSED) {
	    continue;
	}
	if ( ! (traceHdrPtr->flags & TRACE_NO_TIMES)) {
	    Time_Subtract(recordPtr->time, baseTime, &deltaTime);
	    if (baseTime.seconds + baseTime.microseconds > 0) {
		Sys_Printf("%2d.%04d ", deltaTime.seconds,
				    deltaTime.microseconds / 100);
	    } else {
		Sys_Printf("           ");
	    }
	    baseTime = recordPtr->time;
	}
	(*printProc)(recordPtr->traceData, recordPtr->event, FALSE);
	Sys_Printf("\n");
    }
    (*printProc)((ClientData *)NIL, TRUE);

    traceHdrPtr->flags &= ~TRACE_INHIBIT;
    return(SUCCESS);
}

