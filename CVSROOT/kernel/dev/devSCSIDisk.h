/*
 * devSCSIDisk.h --
 *
 *	External definitions for disks on the SCSI I/O bus. The
 *	only request to the block device attach procedure.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIDISK
#define _DEVSCSIDISK

#include "devBlockDevice.h"

DevBlockDeviceHandle	*DevScsiDiskAttach();


#endif _DEVSCSIDISK
