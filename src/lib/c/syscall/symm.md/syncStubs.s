/*
 *
 * syncStubs.s --
 *
 *     Stubs for the Sync_ system calls.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/syncStubs.s,v 1.1 90/01/19 10:19:28 fubar Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

SYS_CALL(2,	Sync_WaitTime,		SYS_SYNC_WAITTIME)
SYS_CALL(1,	Sync_SlowLockStub,	SYS_SYNC_SLOWLOCK)
SYS_CALL(3,	Sync_SlowWaitStub,	SYS_SYNC_SLOWWAIT)
SYS_CALL(2,	Sync_SlowBroadcast,	SYS_SYNC_SLOWBROADCAST)
