/*
 * devKeyboardInt.h --
 *
 * 	Internal types and procedure headers for the keyboard driver.
 * 	Corresponds to the file in the sun machine-dependent directory,
 *	but it may be possible to eliminate this file entirely.
 *
 * Copyright 1988 Regents of the University of California
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

#ifndef _DEVKEYBOARDINT
#define _DEVKEYBOARDINT

#include "sync.h"
/*
 * Master lock for keyboard/mouse driver.
 */

extern Sync_Semaphore	devKbdMutex;

#endif _DEVKEYBOARDINT
