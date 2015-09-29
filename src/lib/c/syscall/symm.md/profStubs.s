/*
 *
 * profStubs.s --
 *
 *     Stubs for the Prof_ system calls.
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
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/profStubs.s,v 1.1 90/01/19 10:19:21 fubar Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"
SYS_CALL(1,	Prof_Dump,	SYS_PROF_DUMP)
SYS_CALL(0,	Prof_End,	SYS_PROF_END)
SYS_CALL(0,	Prof_Start,	SYS_PROF_START)
SYS_CALL(4,	Prof_Profil,	SYS_PROF_PROFIL)
