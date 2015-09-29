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
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/profStubs.s,v 1.1 89/07/11 19:52:14 nelson Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"
SYS_CALL(Prof_Dump, SYS_PROF_DUMP)
SYS_CALL(Prof_End, SYS_PROF_END)
SYS_CALL(Prof_Start, SYS_PROF_START)
SYS_CALL(Prof_Profil, SYS_PROF_PROFIL)
