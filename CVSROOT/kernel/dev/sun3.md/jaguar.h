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

#include "scsiHBA.h"

extern ClientData DevJaguarInit();
extern Boolean DevJaguarIntr();
extern ScsiDevice   *DevJaguarAttachDevice();

#endif /* _DEVJaguar */

