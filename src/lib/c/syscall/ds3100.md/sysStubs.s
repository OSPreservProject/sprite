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
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/sysStubs.s,v 1.3 92/06/16 11:53:47 jhh Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

SYS_CALL(Sys_DoNothing, 	SYS_SYS_DONOTHING)
SYS_CALL(Sys_GetTimeOfDay, 	SYS_SYS_GETTIMEOFDAY)
SYS_CALL(Sys_SetTimeOfDay, 	SYS_SYS_SETTIMEOFDAY)
SYS_CALL(Sys_EnableDisplay, 	SYS_SYS_ENABLEDISPLAY)
SYS_CALL(Sys_Shutdown, 		SYS_SYS_SHUTDOWN)
SYS_CALL(Sys_GetMachineInfo, 	SYS_SYS_GETMACHINEINFO_NEW)
SYS_CALL(Sys_Stats,		SYS_SYS_STATS)
SYS_CALL(Sys_GetHostName,	SYS_SYS_GET_HOSTNAME)
SYS_CALL(Sys_SetHostName,	SYS_SYS_SET_HOSTNAME)
