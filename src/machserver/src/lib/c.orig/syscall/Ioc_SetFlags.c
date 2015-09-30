/* 
 * Ioc_SetFlags.c --
 *
 *	Source code for the Ioc_SetFlags library procedure.
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
static char rcsid[] = "$Header: Ioc_SetFlags.c,v 1.1 88/06/19 14:29:24 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_SetFlags --
 *	Set the flags for the stream.  In this case the flags argument
 *	completely specifies what the stream flags should look like,
 *	ie. it replaces the stream's flags if that is acceptable to
 *	the kernel and the device driver.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the flags field of the stream.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_SetFlags(streamID, flags)
    int streamID;
    int flags;
{
    register ReturnStatus status;

    status = Fs_IOControl(streamID, IOC_SET_FLAGS, sizeof(int),
			(Address)&flags, 0, (Address)NULL);
    return(status);
}