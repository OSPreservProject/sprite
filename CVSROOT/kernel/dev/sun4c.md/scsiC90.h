/*
 * scsiC90.h --
 *
 *	Declarations of interface to the Sun SCSIC90 driver routines.
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

#ifndef _DEVSCSIC90
#define _DEVSCSIC90

#include "devInt.h" 
#include "scsiDevice.h"

extern ClientData DevSCSIC90Init _ARGS_((DevConfigController *ctrlLocPtr));
extern Boolean DevSCSIC90Intr _ARGS_ ((ClientData	clientDataArg));
/* extern Boolean DevSCSIC90IntrStub(); */
extern ScsiDevice   *DevSCSIC90AttachDevice _ARGS_ ((Fs_Device *devicePtr,
	void (*insertProc) _ARGS_ ((List_Links *elementPtr,
	List_Links *elementListHdrPtr))));
#endif /* _DEVSCSIC90 */

