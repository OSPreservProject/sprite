/* 
 * rawvmmach.c --
 *
 *	Routine to print out raw machine dependent vm statistics.
 *	This is the information in Vm_Stat.machDepStat
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.2 89/01/07 04:12:18 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "vmStat.h"


/*
 *----------------------------------------------------------------------
 *
 * VmMach_PrintStats --
 *
 *	Prints out the machine dependent part of the Vm_Stat structure.
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
VmMach_PrintStats(X)
    Vm_Stat *X;
{
	ZeroPrint("stealTLB   %8u\n", X->machDepStat.stealTLB);
	ZeroPrint("stealPID      %8u\n", X->machDepStat.stealPID);    
}

