/* 
 * Mem_SetTraceSizes.c --
 *
 *	Source code for the "Mem_SetTraceSizes" library procedure.  See
 *	memInt.h for overall information about how the allocator works.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_SetTraceSizes.c,v 1.1 88/05/20 15:49:26 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 *----------------------------------------------------------------------
 *
 * Mem_SetTraceSizes --
 *
 *	Defines a list of sizes to trace and causes tracing to start.
 *	If the numSizes is zero or the array ptr is NULL, tracing is
 *	turned off.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Traces of Mem_Alloc and Mem_Free start or end.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Mem_SetTraceSizes(numSizes, arrayPtr)
    int		  numSizes;		/* # of elements in arrayPtr */
    Mem_TraceInfo *arrayPtr;		/* Array of block sizes to trace. */
{
    int	i;

    LOCK_MONITOR;

    if (numSizes <= 0 || (arrayPtr == (Mem_TraceInfo *) NULL) 
	    || arrayPtr == (Mem_TraceInfo *)NULL) {
	if (numSizes == -1) {
	    memNumSizesToTrace = -1;
	} else {
	    memNumSizesToTrace = 0;
	}
	UNLOCK_MONITOR;
	return;
    }  

    if (numSizes > MAX_NUM_TRACE_SIZES) {
	numSizes = MAX_NUM_TRACE_SIZES;
    }
    memNumSizesToTrace = numSizes;
    for (i = 0; i < numSizes; i++) {
	memTraceArray[i].traceInfo = arrayPtr[i];
	memTraceArray[i].traceInfo.flags |= MEM_TRACE_NOT_INIT;
    }
    UNLOCK_MONITOR;
}
