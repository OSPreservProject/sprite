/* 
 * Ioc_Reposition.c --
 *
 *	Source code for the Ioc_Reposition library procedure.
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
static char rcsid[] = "$Header: Ioc_Reposition.c,v 1.2 88/07/29 17:08:46 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_Reposition --
 *
 *	Reposition the access position on a filesystem stream.  The next
 *	read or write will start at the byte offset specified by the
 *	combination of base and offset.  Base has three values corresponding
 *	to the beginning of file, current position, and end of file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the stream access position.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_Reposition(streamID, base, offset, newOffsetPtr)
    int streamID;		/* StreamID returned from Fs_Open */
    int base;			/* IOC_BASE_ZERO for beginning of file,
				 * IOC_BASE_CURRENT for the current position,
				 * IOC_BASE_EOF for end of file. */
    int offset;			/* Byte offset relative to base */
    int *newOffsetPtr;		/* The resulting access position */
{
    register ReturnStatus status;
    Ioc_RepositionArgs args;

    args.base = base;
    args.offset = offset;
    status = Fs_IOControl(streamID, IOC_REPOSITION, sizeof(Ioc_RepositionArgs),
			(Address)&args, sizeof(int), (Address) newOffsetPtr);
    return(status);
}
