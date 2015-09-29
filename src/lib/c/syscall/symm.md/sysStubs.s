/*
 *
 * sysStubs.s --
 *
 *     Stubs for the Sys_ system calls.
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
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/sysStubs.s,v 1.1 90/01/19 10:19:31 fubar Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"

SYS_CALL(0,	Sys_DoNothing,	 	SYS_SYS_DONOTHING)
SYS_CALL(3,	Sys_GetTimeOfDay, 	SYS_SYS_GETTIMEOFDAY)
SYS_CALL(3,	Sys_SetTimeOfDay, 	SYS_SYS_SETTIMEOFDAY)
/* next syscall is obsolete? */
SYS_CALL(1,	Sys_EnableDisplay, 	SYS_SYS_ENABLEDISPLAY)
SYS_CALL(2,	Sys_Shutdown, 		SYS_SYS_SHUTDOWN)
SYS_CALL(2,	Sys_GetMachineInfo, 	SYS_SYS_GETMACHINEINFO_NEW)
SYS_CALL(4,	Sys_Stats,		SYS_SYS_STATS)

