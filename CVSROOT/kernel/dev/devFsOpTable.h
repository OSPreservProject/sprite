/*
 * devFsOpTable.h --
 *
 *	Declarations for device access.  The DEVICE operation switch is
 *	defined here.  The I/O handle formas for devices is defined here.
 *
 * Copyright 1987 Regents of the University of California
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

#ifndef _DEVOPTABLE 
#define _DEVOPTABLE

#include "sprite.h"
#include "user/fs.h"
#include "devBlockDevice.h"

/*
 * Device type specific operations.
 *	The arguments to the operations are commented below the
 *	macro definitions used to invoke them.
 */

typedef struct DevFsTypeOps {
    int		 type;	/* One of the device types. See devNumbersInt.h */
    ReturnStatus (*open)();
    ReturnStatus (*read)();
    ReturnStatus (*write)();
    ReturnStatus (*ioControl)();
    ReturnStatus (*close)();
    ReturnStatus (*select)();
    DevBlockDeviceHandle *((*blockDevAttach)());
} DevFsTypeOps;

extern DevFsTypeOps devFsOpTable[];
extern int devNumDevices;

/*
 * DEV_TYPE_INDEX() - Compute the index into the devFsOpTable from the
 *		      type field from of the Fs_Device structure.
 */

#define	DEV_TYPE_INDEX(type)	((type)&0xff)
/*
 * A list of disk device Fs_Device structure that is used when probing for a
 * disk. Initialized in devConfig.c.
 */
extern Fs_Device devFsDefaultDiskPartitions[];
extern int devNumDefaultDiskPartitions;

#endif _DEVOPTABLE
