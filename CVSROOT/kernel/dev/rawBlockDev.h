/*
 * rawBlockDev.h --
 *
 *	Declarations of routines for raw mode access to block devices.
 *	This routines should be accessed thru the file system device
 *	switch.
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

#ifndef _RAWBLOCKDEV
#define _RAWBLOCKDEV

extern ReturnStatus DevRawBlockDevOpen();
extern ReturnStatus DevRawBlockDevReopen();
extern ReturnStatus DevRawBlockDevRead();
extern ReturnStatus DevRawBlockDevWrite();
extern ReturnStatus DevRawBlockDevClose();
extern ReturnStatus DevRawBlockDevIOControl();


#endif /* _RAWBLOCKDEV */

