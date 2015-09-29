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
 * $Header: /sprite/src/boot/decprom/RCS/fsBoot.h,v 1.1 90/02/16 16:14:23 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _FSBOOT
#define _FSBOOT

#include "fs.h"
#include <kernel/fslcl.h>
#include <kernel/fsdm.h>
#include "fsIndex.h"
#include <kernel/fsio.h>
#include <kernel/fsioFile.h>
#include <kernel/fsioDevice.h>
#include <kernel/ofs.h>
#include "boot.h"

/*
 * The real Fsdm_Domain contains lots of knarly kernel data structures,
 * so it isn't defined outside the kernel.  All we use is the (now
 * non-existent) header pointer, so define our own verion of Fsdm_Domain.
 */

typedef struct {
    Ofs_DomainHeader	*headerPtr;
} Fsdm_Domain;

extern Fs_Device 		fsDevice;
extern	Fsdm_Domain		*fsDomainPtr;
extern Fsio_FileIOHandle	*fsRootHandlePtr;

#define mnew(type)	(type *)malloc(sizeof(type))

#endif _FSBOOT
