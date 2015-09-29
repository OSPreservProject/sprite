/*
 * getMachineInfo.h --
 *
 *	Declarations of the interface to the GetMachineInfo routine.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/admin/mklfs/RCS/getMachineInfo.h,v 1.1 91/05/31 11:09:55 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _GETMACHINEINFO
#define _GETMACHINEINFO

#include <cfuncproto.h>


extern void GetMachineInfo _ARGS_((int *serverIDPtr, int *maxNumCacheBlocksPtr));

#endif /* _GETMACHINEINFO */

