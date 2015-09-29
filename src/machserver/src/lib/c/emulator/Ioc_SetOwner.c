/* 
 * Ioc_SetOwner.c --
 *
 *	Source code for the Ioc_SetOwner library procedure.
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
static char rcsid[] = "$Header: Ioc_SetOwner.c,v 1.2 88/07/29 18:34:45 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_SetOwner --
 *	Set the ID of the process or group associated with the device.
 *	This is used, for example, to implement control over a tty
 *	device so that background jobs can't read input.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the ID.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_SetOwner(streamID, id, procOrFamily)
    int streamID;
    int id;
    int procOrFamily;
{
    register ReturnStatus status;
    Ioc_Owner owner;

    owner.id = id;
    owner.procOrFamily = procOrFamily;

    status = Fs_IOControl(streamID, IOC_SET_OWNER, sizeof(Ioc_Owner),
			(Address)&owner, 0, (Address)NULL);
    return(status);
}
