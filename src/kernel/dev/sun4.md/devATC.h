/*
 * devATC.h --
 *
 *	Internal declarations for the ATC disk controller. 
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
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun4.md/devATC.h,v 9.1 92/10/23 16:28:53 elm Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVATC
#define _DEVATC

#include "devBlockDevice.h"

extern DevBlockDeviceHandle	*DevATCDiskAttach();


#endif /* _DEVATC */



