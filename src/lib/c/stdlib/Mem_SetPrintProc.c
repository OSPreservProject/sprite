/* 
 * Mem_SetPrintProc.c --
 *
 *	Source code for the "Mem_SetPrintProc" library procedure.  See memInt.h
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/Mem_SetPrintProc.c,v 1.1 88/05/20 15:49:25 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 *----------------------------------------------------------------------
 *
 * Mem_SetPrintProc --
 *
 *	Changes the default printing routines
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The default printing routine is changed.  Proc must have the
 *	following calling sequence:
 *
 *	int
 *	PrintProc(clientData, format, arg1, arg2, arg3, arg4, arg5)
 *	    ClientData clientData;
 *	    char *format;
 *	{
 *	}
 *
 *	This is identical to the calling sequence for fprintf (and fprintf is
 *	used as the default print procedure).  The first argument is the
 *	clientData argument passed to this procedure, which need not
 *	necessarily be a (FILE *).  There will never be more than 5 arguments.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Mem_SetPrintProc(proc, data)
    int	(*proc)();		/* Address of new print routine. */
    ClientData	data;		/* Data to be passed to proc. */
{
    LOCK_MONITOR;
    memPrintProc = proc;
    memPrintData = data;
    UNLOCK_MONITOR;
}
