/* 
 * MemDoTrace.c --
 *
 *	Source code for the "MemDoTrace" procedure, which is used
 *	internally by the memory allocator.  See memInt.h for overall
 *	information about how the allocator works..
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/MemDoTrace.c,v 1.1 88/05/20 15:49:16 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#define MEM_TRACE 1

#include "memInt.h"

/*
 * The array below holds information about what to trace, and how.
 */

MemTraceElement	memTraceArray[MAX_NUM_TRACE_SIZES];
int		memNumSizesToTrace = 0;

/*
 *----------------------------------------------------------------------
 *
 * PrintTrace --
 *
 *	Print out the given trace information about a memory trace record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static void
PrintTrace(allocated, infoPtr, curPC)
    Boolean		allocated;	/* If TRUE, we are called by Mem_Alloc,
					 * otherwise by Mem_Free. */
    register Address	infoPtr;	/* Address of admin. info. */
    Address		curPC;		/* If called by Mem_Free, PC of
					 * call to Mem_Free, NULL otherwise. */
{
    if (allocated) {
	(*memPrintProc)(memPrintData,
		"malloc: PC=0x%x  addr=0x%x  size=%d\n",
		GET_PC(infoPtr), infoPtr+sizeof(AdminInfo), 
		GET_ORIG_SIZE(infoPtr));
    } else {
	(*memPrintProc)(memPrintData,
		"free:  PC=0x%x  addr=0x%x  size=%d *\n",
		curPC, infoPtr+sizeof(AdminInfo), GET_ORIG_SIZE(infoPtr));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MemDoTrace --
 *
 *	Print and/or store a trace record.  Called by malloc and free.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
MemDoTrace(allocated, infoPtr, curPC, size)
    Boolean		allocated;	/* If TRUE, we are called by malloc,
					 * otherwise by free. */
    register Address	infoPtr;	/* Address of admin. info. */
    Address		curPC;		/* If called by free, PC of call to
					 * free, NULL otherwise. */
    int			size;		/* Size actually allocated. */
{
    int				i, j;
    int				origSize;
    Address			callerPC;
    register MemTraceElement	*trPtr;

    if (memNumSizesToTrace == -1) {
	PrintTrace(allocated, infoPtr, curPC);
	return;
    }

    callerPC = GET_PC(infoPtr);

    origSize = GET_ORIG_SIZE(infoPtr);

    for (i = 0, trPtr = memTraceArray; i < memNumSizesToTrace;
	    i++, trPtr++) {
	if (trPtr->traceInfo.flags & MEM_DONT_USE_ORIG_SIZE) {
	    if (trPtr->traceInfo.size != size) {
		continue;
	    }
	} else if (trPtr->traceInfo.size != origSize) {
	    continue;
	}
	if (trPtr->traceInfo.flags & MEM_PRINT_TRACE) {
	    PrintTrace(allocated, infoPtr, curPC);
	}
	if (trPtr->traceInfo.flags & MEM_STORE_TRACE) {
	    if (trPtr->traceInfo.flags & MEM_TRACE_NOT_INIT) {
		for (j = 0; j < MAX_CALLERS_TO_TRACE; j++) {
		    trPtr->allocInfo[j].numBlocks = 0;
		}
		trPtr->traceInfo.flags &= ~MEM_TRACE_NOT_INIT;
	    }
	    for (j = 0; j < MAX_CALLERS_TO_TRACE; j++) {
		if (trPtr->allocInfo[j].numBlocks == 0) {
		    if (allocated) {
			trPtr->allocInfo[j].callerPC = callerPC;
			trPtr->allocInfo[j].numBlocks = 1;
		    }
		    break;
		} else if (trPtr->allocInfo[j].callerPC == callerPC) {
		    if (allocated) {
			trPtr->allocInfo[j].numBlocks++;
		    } else {
			trPtr->allocInfo[j].numBlocks--;
		    }
		    break;
		}
	    }
	}
	break;
    }
}
