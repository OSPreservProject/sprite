/* 
 * raidExt.h --
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
 * $Header: /sprite/src/kernel/Cvsroot/kernel/raid/raidExt.h,v 1.6 90/10/12 14:01:27 eklee Exp $ SPRITE (Berkeley)
 */

#ifndef _RAIDEXT
#define _RAIDEXT

#include "devBlockDevice.h"

DevBlockDeviceHandle	*DevRaidAttach();
DevBlockDeviceHandle	*DevDebugAttach();

#endif _RAIDEXT
