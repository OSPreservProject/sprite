/* 
 * Ioc_Lock.c --
 *
 *	Source code for the Ioc_Lock library procedure.
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
static char rcsid[] = "$Header: Ioc_Lock.c,v 1.1 88/06/19 14:29:19 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_Lock --
 *	Lock the device behind the stream.  The flags specify if the
 *	lock is shared (IOC_LOCK_SHARED) or exclusive (IOC_LOCK_EXCLUSIVE).
 *	A shared lock can co-exist with other shared locks, but not with
 *	any exclusive locks.  Exclusive can't co-exist with any other locks.
 *	If the lock can't be obtained right away, the process gets blocked
 *	unless the IOC_LOCK_NO_BLOCK flag is specified.  In this case
 *	FS_WOULD_BLOCK gets returned if the lock can't be taken.
 *
 * Results:
 *	SUCCESS or FS_WOULD_BLOCK
 *
 * Side effects:
 *	Grabs the lock associated with the device underlying the stream.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_Lock(streamID, flags)
    int streamID;
    int flags;
{
    register ReturnStatus status;
    Ioc_LockArgs args;

    args.flags = flags;

    status = Fs_IOControl(streamID, IOC_LOCK, sizeof(Ioc_LockArgs),
			(Address)&args, 0, (Address)NULL);
    return(status);
}
