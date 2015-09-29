/*
 *
 * sigStubs.s --
 *
 *     Stubs for the Sig_ system calls.
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
 * rcs = $Header: /sprite/src/lib/c/syscall/sun3.md/RCS/sigStubs.s,v 1.3 89/06/15 22:44:03 douglis Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"

SYS_CALL(Sig_RawPause, 		SYS_SIG_PAUSE)
SYS_CALL(Sig_Send, 		SYS_SIG_SEND)
SYS_CALL(Sig_SetAction, 	SYS_SIG_SETACTION)
SYS_CALL(Sig_SetHoldMask, 	SYS_SIG_SETHOLDMASK)
