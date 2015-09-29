/* 
 * Mem_PrintInUse.c --
 *
 *	Source code for the "Mem_PrintInUse" library procedure.  See memInt.h
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_PrintInUse.c,v 1.2 89/06/15 22:36:49 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"


/*
 * ----------------------------------------------------------------------------
 *
 * Mem_PrintInUse --
 *
 *      Uses a default procedure to print out the non-binned blocks in
 *	the allocator that are still in use.  The original size and the
 *	PC of the call to Mem_Alloc are printed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Stuff gets printed (?) by passing it to memPrintProc.  See the
 *	documentation in Mem_PrintStatsSubr for the calling sequence to
 *	memPrintProc.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Mem_PrintInUse()
{
    register Address ptr;

    LOCK_MONITOR;

    if (!memInitialized) {
	(*memPrintProc)(memPrintData, "Allocator not initialized yet.\n");
	return;
    }

    (*memPrintProc)(memPrintData, "Large objects still in use:\n");
    (*memPrintProc)(memPrintData, "    Location   Orig. Size   Alloc.PC\n");

    for (ptr = memFirst; ptr != memLast; ptr += SIZE(GET_ADMIN(ptr))) {

	if (!IS_DUMMY(GET_ADMIN(ptr)) && IS_IN_USE(GET_ADMIN(ptr))) {
#ifdef MEM_TRACE
	    (*memPrintProc)(memPrintData,"%#12x %10d %#12x\n",
		    ptr, GET_ORIG_SIZE(ptr), GET_PC(ptr));
#else
	    (*memPrintProc)(memPrintData,"%#12x %10d %12s\n", ptr,
		    SIZE(GET_ADMIN(ptr)) - sizeof(AdminInfo), "??");
#endif MEM_TRACE

	}
    }
    UNLOCK_MONITOR;
}
