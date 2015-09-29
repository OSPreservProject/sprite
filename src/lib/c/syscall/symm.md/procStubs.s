/*
 * proc.s --
 *
 *     Contains stubs for user-level system calls.
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
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/procStubs.s,v 1.1 90/01/19 10:19:03 fubar Exp $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

SYS_CALL(5,	Proc_Debug,			SYS_PROC_DEBUG)
SYS_CALL(1,	Proc_Detach,			SYS_PROC_DETACH)
SYS_CALL(5,	Proc_RawExec,			SYS_PROC_EXEC)
SYS_CALL(2,	Proc_Fork,			SYS_PROC_FORK)
SYS_CALL(4,	Proc_ExecEnv,			SYS_PROC_EXEC_ENV)
SYS_CALL(4,	Proc_RawRemoteExec,		SYS_PROC_REMOTE_EXEC)
SYS_CALL(1,	Proc_RawExit,			SYS_PROC_EXIT)
SYS_CALL(2,	Proc_GetFamilyID,		SYS_PROC_GETFAMILYID)
SYS_CALL(3,	Proc_GetGroupIDs,		SYS_PROC_GETGROUPIDS)
SYS_CALL(4,	Proc_GetIDs,			SYS_PROC_GETIDS)
SYS_CALL(7,	Proc_GetPCBInfo,		SYS_PROC_GETPCBINFO)
SYS_CALL(2,	Proc_GetPriority,		SYS_PROC_GETPRIORITY)
SYS_CALL(2,	Proc_GetResUsage,		SYS_PROC_GETRESUSAGE)
SYS_CALL(6,	Proc_Profile,			SYS_PROC_PROFILE)
SYS_CALL(2,	Proc_SetFamilyID,		SYS_PROC_SETFAMILYID)
SYS_CALL(2,	Proc_SetGroupIDs,		SYS_PROC_SETGROUPIDS)
SYS_CALL(2,	Proc_SetIDs,			SYS_PROC_SETIDS)
SYS_CALL(3,	Proc_SetPriority,		SYS_PROC_SETPRIORITY)
SYS_CALL(8,	Proc_RawWait,			SYS_PROC_WAIT)

/* Next six not found; may be obsolete syscalls... */
SYS_CALL(0,	Proc_OLDSetEnviron,		SYS_PROC_SET_ENVIRON)
SYS_CALL(0,	Proc_OLDUnsetEnviron,		SYS_PROC_UNSET_ENVIRON)
SYS_CALL(0,	Proc_OLDGetEnvironVar, 	SYS_PROC_GET_ENVIRON_VAR)
SYS_CALL(0,	Proc_OLDGetEnvironRange, 	SYS_PROC_GET_ENVIRON_RANGE)
SYS_CALL(0,	Proc_OLDInstallEnviron, 	SYS_PROC_INSTALL_ENVIRON)
SYS_CALL(0,	Proc_OLDCopyEnviron,		SYS_PROC_COPY_ENVIRON)

SYS_CALL(2,	Proc_RawMigrate,		SYS_PROC_MIGRATE)
SYS_CALL(2,	Proc_GetIntervalTimer,		SYS_PROC_GETINTERVALTIMER)
SYS_CALL(3,	Proc_SetIntervalTimer,		SYS_PROC_SETINTERVALTIMER)
SYS_CALL(2,	Proc_GetHostIDs,		SYS_PROC_GETHOSTIDS)
