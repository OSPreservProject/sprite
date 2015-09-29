/* 
 * Ioc_ClearBits.c --
 *
 *	Source code for the Ioc_ClearBits library procedure.
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
static char rcsid[] = "$Header: Ioc_ClearBits.c,v 1.1 88/06/19 14:29:16 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_ClearBits --
 *	Clear the bits indicated in the argument from the stream's
 *	flag field.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the indicated bits.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_ClearBits(streamID, bits)
    int streamID;
    int bits;
{
    register ReturnStatus status;

    status = Fs_IOControl(streamID, IOC_CLEAR_BITS, sizeof(int),
			(Address)&bits, 0, (Address)NULL);
    return(status);
}
