/* 
 * rpcDelay.c --
 *
 *	Get perferred machine dependent inter-packet delays for rpcs.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "mach.h"
#include "sys.h"


/*
 *----------------------------------------------------------------------
 *
 * RpcGetMachineDelay --
 *
 *	Get preferred inter-fragment delay for input and output packets.
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
RpcGetMachineDelay(myDelayPtr, outputRatePtr)
	unsigned short	*myDelayPtr;	/* Preferred delay in microseconds
					 * between successive input packets.
					 */
	unsigned short	*outputRatePtr;	/* Delay in microseconds between
					 * successive output packets.
					 */
{

    *myDelayPtr = 500;
    *outputRatePtr = 500;
}

