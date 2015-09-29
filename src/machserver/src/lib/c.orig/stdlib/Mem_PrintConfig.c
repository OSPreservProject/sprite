/* 
 * Mem_PrintConfig.c --
 *
 *	Source code for the "Mem_PrintConfig" library procedure.  See memInt.h
 *	for overall information about how the allocator works..
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_PrintConfig.c,v 1.2 89/01/30 15:39:35 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintConfig --
 *
 *	Prints out the exact configuration of the dynamic memory allocator
 *	using the default printing routine.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Stuff gets printed.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Mem_PrintConfig()
{
    register Address ptr;
    int i, j;

    LOCK_MONITOR;

    if (!memInitialized) {
	(*memPrintProc)(memPrintData, "Allocator not initialized yet.\n");
	return;
    }

#define VERBOSE 2
#ifdef VERBOSE
    (*memPrintProc)(memPrintData, "Small object allocator:\n");
    for (i = 2; i < BIN_BUCKETS; i++) {
	if ((memFreeLists[i] == (Address) NULL)
		|| (memFreeLists[i] == NOBIN)) {
	    continue;
	}
	(*memPrintProc)(memPrintData, "    %d bytes:", INDEX_TO_BLOCKSIZE(i));
	j = 5;
	for (ptr = memFreeLists[i]; ptr != (Address) NULL; 
		ptr = (Address) GET_ADMIN(ptr)) {
	    if (j == 5) {
		(*memPrintProc)(memPrintData, "\n    ");
		j = 0;
	    } else {
		j += 1;
	    }
	    (*memPrintProc)(memPrintData, "%12#x", ptr);
	}
	(*memPrintProc)(memPrintData, "\n");
    }
#endif VERBOSE

    (*memPrintProc)(memPrintData, "Large object allocator:\n");

#ifdef MEM_TRACE
    (*memPrintProc)(memPrintData, "    Location   Orig. Size   State\n");
#else
    (*memPrintProc)(memPrintData, "    Location       Size     State\n");
#endif MEM_TRACE

    for (ptr = memFirst; ptr != memLast; ptr += SIZE(GET_ADMIN(ptr))) {

#ifdef MEM_TRACE
	(*memPrintProc)(memPrintData, "%12#x %10d", ptr, GET_ORIG_SIZE(ptr));
	if (IS_DUMMY(GET_ADMIN(ptr))) {
	    (*memPrintProc)(memPrintData, "     Dummy\n");
	} else if (IS_IN_USE(GET_ADMIN(ptr))) {
	    (*memPrintProc)(memPrintData, "     In use (PC=0x%x)\n", GET_PC(ptr));
#else
	(*memPrintProc)(memPrintData, "%12#x %10d", ptr, SIZE(GET_ADMIN(ptr)));
	if (IS_DUMMY(GET_ADMIN(ptr))) {
	    (*memPrintProc)(memPrintData, "     Dummy\n");
	} else if (IS_IN_USE(GET_ADMIN(ptr))) {
	    (*memPrintProc)(memPrintData, "     In use\n");
#endif MEM_TRACE
	} else {
	    (*memPrintProc)(memPrintData, "     Free\n");
	}
    }
    UNLOCK_MONITOR;
}
