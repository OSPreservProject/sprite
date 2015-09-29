/*
 * fsBoot.h --
 *
 *      Boot program include fileto get needed FS header files.
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
 * $Header: /sprite/src/kernel/fs/RCS/fsInt.h,v 8.9 89/05/14 13:59:20 brent Exp Locker: brent $ SPRITE (Berkeley)
 */

#ifndef _FSBOOT
#define _FSBOOT

#include "fs.h"
#include "fslcl.h"
#include "fsdm.h"
#include "fsIndex.h"
#include "fsio.h"
#include "fsioFile.h"
#include "fsioDevice.h"
#include "boot.h"

extern Fs_Device 		fsDevice;
extern	Fsdm_Domain		*fsDomainPtr;
extern Fsio_FileIOHandle	*fsRootHandlePtr;

#define mnew(type)	(type *)malloc(sizeof(type))

#endif _FSBOOT
