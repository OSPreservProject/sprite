/*
 * scsi0.h --
 *
 *	Declarations of interface to the Sun SCSI0 driver routines.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSI0
#define _DEVSCSI0

#include "scsiHBA.h"

extern ClientData DevSCSI0Init();
extern Boolean DevSCSI0Intr();
extern Boolean DevSCSI0IntrStub();
extern ScsiDevice   *DevSCSI0AttachDevice();

#endif /* _DEVSCSI0 */

