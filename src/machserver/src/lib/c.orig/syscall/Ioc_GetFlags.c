/* 
 * Ioc_GetFlags.c --
 *
 *	Source code for the Ioc_GetFlags library procedure.
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
static char rcsid[] = "$Header: Ioc_GetFlags.c,v 1.2 88/07/29 17:08:42 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_GetFlags --
 *	Return the flags associated with the stream.  This is composed
 *	of a standard set of flags in the bits masked by IOC_GENERIC_FLAGS,
 *	plus other bits interpreted soely by the (pseudo) device driver.
 *
 * Results:
 *	A copy of the flags.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_GetFlags(streamID, flagsPtr)
    int streamID;
    int *flagsPtr;
{
    register ReturnStatus status;

    status = Fs_IOControl(streamID, IOC_GET_FLAGS, 0,
			(Address)NULL, sizeof(int), (Address) flagsPtr);
    return(status);
}
