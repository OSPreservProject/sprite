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

#include "devInt.h"
#include "scsiHBA.h"

extern ClientData DevSCSI0Init _ARGS_ ((DevConfigController *ctrlLocPtr));
extern Boolean DevSCSI0Intr _ARGS_ ((ClientData clientData));
extern ScsiDevice *DevSCSI0AttachDevice _ARGS_ ((Fs_Device *devicePtr,
    void (*insertProc) _ARGS_ ((List_Links *elementPtr,
                                List_Links *elementListHdrPtr))));

#endif /* _DEVSCSI0 */

