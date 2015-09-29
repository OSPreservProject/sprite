/* 
 * zebra.s --
 *
 *	Stubs for Zebra system calls.
 *	
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 * rcs = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.6 92/03/02 15:29:56 bmiller Exp $ SPRITE (Berkeley)";
 *
 */

#include "userSysCallInt.h"

SYS_CALL(Zss_Cmd, SYS_ZSS_CMD)
SYS_CALL(Zebra_Cmd, SYS_ZEBRA_CMD)
