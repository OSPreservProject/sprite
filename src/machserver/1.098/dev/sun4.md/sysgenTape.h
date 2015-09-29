/*
 * devSCSISysgen.h
 *
 * Definitions for the attach procedure for  Sysgen tape drives.
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
 * $Header: /sprite/src/kernel/dev/sun3.md/RCS/sysgenTape.h,v 8.3 89/05/24 07:50:28 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVSCSISYSGEN
#define _DEVSCSISYSGEN

extern ReturnStatus DevSysgenAttach _ARGS_ ((Fs_Device *devicePtr,
    ScsiDevice *devPtr, ScsiTape *tapePtr));

#endif /* _DEVSCSISYSGEN */
