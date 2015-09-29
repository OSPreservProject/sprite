/* 
 * Ioc_WriteBack.c --
 *
 *	Source code for the Ioc_WriteBack library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/syscall/RCS/Ioc_WriteBack.c,v 1.1 89/08/30 08:38:10 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_WriteBack --
 *
 *	Write back cached data for a stream.  The arguments indicate
 *	a byte range that should be written back.  The caller
 *	has the option of blocking until the data is written out,
 *	or the caller can let the write back happen asynchronously.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Writes out cache data.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_WriteBack(streamID, firstByte, lastByte, shouldBlock)
    int streamID;		/* StreamID returned from Fs_Open */
    int firstByte;		/* Index of first byte to write back */
    int lastByte;		/* Index of last byte ot write back */
    Boolean shouldBlock;	/* If TRUE, block until write back is done */
{
    register ReturnStatus status;
    Ioc_WriteBackArgs args;

    args.firstByte = firstByte;
    args.lastByte = lastByte;
    args.shouldBlock = shouldBlock;
    status = Fs_IOControl(streamID, IOC_WRITE_BACK, sizeof(Ioc_WriteBackArgs),
			(Address)&args, 0, (Address) 0);
    return(status);
}
