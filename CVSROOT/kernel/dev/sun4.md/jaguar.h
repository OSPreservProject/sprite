/*
 * scsi3.h --
 *
 *	Declarations of interface to the Sun Jaguar driver routines.
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

#ifndef _DEVJaguar
#define _DEVJaguar

#include "devInt.h"
#include "scsiHBA.h"

extern ClientData DevJaguarInit _ARGS_ ((DevConfigController *ctrlLocPtr));
extern Boolean DevJaguarIntr _ARGS_ ((ClientData clientData));
extern ScsiDevice   *DevJaguarAttachDevice _ARGS_ ((Fs_Device *devicePtr,
    void (*insertProc) _ARGS_ ((List_Links *elementPtr,
                                List_Links *elementListHdrPtr))));

#endif /* _DEVJaguar */

