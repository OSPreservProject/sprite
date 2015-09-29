/*
 *
 * netStubs.s --
 *
 *     Stubs for the Net_ system calls.
 *
 * Copyright 1966, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/netStubs.s,v 1.1 90/01/19 10:18:57 fubar Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"
SYS_CALL(6,	Net_InstallRoute, SYS_NET_INSTALL_ROUTE)

