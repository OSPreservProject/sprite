/* 
 * Ioc_Unlock.c --
 *
 *	Source code for the Ioc_Unlock library procedure.
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
static char rcsid[] = "$Header: Ioc_Unlock.c,v 1.1 88/06/19 14:29:27 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_Unlock --
 *	Unlock the device behind the stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The locks indicated by flags are released.  This unblocks
 *	any waiting processes.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_Unlock(streamID, flags)
    int streamID;
    int flags;
{
    register ReturnStatus status;
    Ioc_LockArgs args;

    args.flags = flags;

    status = Fs_IOControl(streamID, IOC_UNLOCK, sizeof(Ioc_LockArgs),
			(Address)&args, 0, (Address)NULL);
    return(status);
}
