/*
 * fsLock.h --
 *
 *	Declarations for user-level file locking routines.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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

#ifndef _FSLOCK
#define _FSLOCK

/*
 * Cache consistency routines.
 */
extern void		FsLockInit();
extern ReturnStatus	FsIocLock();
extern ReturnStatus	FsLock();
extern ReturnStatus	FsUnlock();
extern void		FsLockClose();
extern void		FsLockClientKill();
#endif _FSLOCK
