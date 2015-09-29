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
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/netStubs.s,v 1.2 92/06/16 11:52:57 jhh Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"
SYS_CALL(Net_InstallRoute, SYS_NET_INSTALL_ROUTE)
SYS_CALL(Net_GetRoutes, SYS_NET_GET_ROUTES)
SYS_CALL(Net_DeleteRoute, SYS_NET_DELETE_ROUTE)
