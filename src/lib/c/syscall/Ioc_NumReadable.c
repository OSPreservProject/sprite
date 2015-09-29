/* 
 * Ioc_NumReadable.c --
 *
 *	Source code for the Ioc_NumReadable library procedure.
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
static char rcsid[] = "$Header: Ioc_NumReadable.c,v 1.2 88/07/29 17:08:44 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_NumReadable --
 *	Return the number of bytes available on the stream.
 *
 * Results:
 *	The number of bytes available.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_NumReadable(streamID, numPtr)
    int streamID;
    int *numPtr;
{
    register ReturnStatus status;

    status = Fs_IOControl(streamID, IOC_NUM_READABLE, 0,
			(Address)NULL, sizeof(int), (Address) numPtr);
    return(status);
}
