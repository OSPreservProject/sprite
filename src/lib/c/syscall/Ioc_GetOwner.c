/* 
 * Ioc_GetOwner.c --
 *
 *	Source code for the Ioc_GetOwner library procedure.
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
static char rcsid[] = "$Header: Ioc_GetOwner.c,v 1.2 88/07/29 18:43:23 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Ioc_GetOwner --
 *	Return the ID or the process or group that is currently
 *	associated (owns) the device.
 *
 * Results:
 *	*idPtr is set to the ID, and *procOrFamilyPtr is set to
 *	either IOC_OWNER_FAMILY if the ID is a process family ID,
 *	or to IOC_OWNER_PROC if the ID is of one process.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Ioc_GetOwner(streamID, idPtr, procOrFamilyPtr)
    int streamID;
    int *idPtr;
    int *procOrFamilyPtr;
{
    register ReturnStatus status;
    Ioc_Owner owner;

    status = Fs_IOControl(streamID, IOC_GET_OWNER, 0,
			(Address)NULL, sizeof(Ioc_Owner), (Address)&owner);
    *idPtr = owner.id;
    *procOrFamilyPtr = owner.procOrFamily;
    return(status);
}
