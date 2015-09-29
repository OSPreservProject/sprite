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
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/syncStubs.s,v 1.2 90/04/27 12:18:00 shirriff Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

SYS_CALL(Sync_WaitTime, SYS_SYNC_WAITTIME)
SYS_CALL(Sync_SlowLockStub, SYS_SYNC_SLOWLOCK)
SYS_CALL(Sync_SlowWaitStub, SYS_SYNC_SLOWWAIT)
SYS_CALL(Sync_SlowBroadcast, SYS_SYNC_SLOWBROADCAST)
SYS_CALL(Sync_Semctl, SYS_SYNC_SEMCTL)
SYS_CALL(Sync_Semget, SYS_SYNC_SEMGET)
SYS_CALL(Sync_Semop, SYS_SYNC_SEMOP)
