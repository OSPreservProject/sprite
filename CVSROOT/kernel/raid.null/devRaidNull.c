/* 
 * devRaidNull.c --
 *
 *	Stub module to be used when it is not desireable to link the RAID
 *	driver into the kernel.
 *
 * Copyright 1989 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "fs.h"
#include "devBlockDevice.h"

DevBlockDeviceHandle *
DevRaidAttach(devicePtr)
    Fs_Device	*devicePtr;	/* The device to attach. */
{
    return (DevBlockDeviceHandle *) NIL;
}

DevBlockDeviceHandle *
DevDebugAttach(devicePtr)
    Fs_Device	*devicePtr;	/* The device to attach. */
{
    return (DevBlockDeviceHandle *) NIL;
}
