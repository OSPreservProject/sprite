/*
 * devVMElink.h --
 *
 *	Internal declarations of interface to the Bit-3 VME
 *	link driver routines.
 *
 * Copyright 1990 Regents of the University of California
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

#ifndef _DEVVMELINK
#define _DEVVMELINK

#include "devTypes.h"
#include <sys/param.h>
#include "vmelink.h"

/*
 * This is the maximum number of VME link boards in a system.
 */
#define VMELINK_MAX_BOARDS	8

extern ClientData DevVMElinkInit ();
extern ReturnStatus DevVMElinkOpen ();
extern ReturnStatus DevVMElinkRead ();
extern ReturnStatus DevVMElinkWrite ();
extern ReturnStatus DevVMElinkIOControl ();

#endif /* _DEVVMELINK */
