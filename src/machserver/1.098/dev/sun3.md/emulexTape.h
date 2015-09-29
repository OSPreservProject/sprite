/*
 * devSCSIEmulex.h
 *
 * Definitions for attach routine for Emulex tape drives.
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
 * $Header: /sprite/src/kernel/dev/sun3.md/RCS/emulexTape.h,v 8.4 89/05/24 07:49:53 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIEMULEX
#define _DEVSCSIEMULEX

extern ReturnStatus DevEmulexAttach _ARGS_ ((Fs_Device *devicePtr,
    ScsiDevice *devPtr, ScsiTape *tapePtr));

#endif /* _DEVSCSIEMULEX */
