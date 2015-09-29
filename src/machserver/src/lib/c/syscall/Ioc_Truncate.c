/* 
 * Ioc_Truncate.c --
 *
 *	Source code for the Ioc_Truncate library procedure.
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
static char rcsid[] = "$Header: Ioc_Truncate.c,v 1.1 88/06/19 14:29:26 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_Truncate --
 *	Truncate the stream to a specified length.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The underlying device or file gets truncated.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_Truncate(streamID, length)
    int streamID;
    int length;
{
    register ReturnStatus status;

    status = Fs_IOControl(streamID, IOC_TRUNCATE, sizeof(int),
			(Address)&length, 0, (Address)NULL);
    return(status);
}
