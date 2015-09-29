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
 * rcs = $Header: /sprite/src/lib/c/syscall/sun3.md/RCS/procStubs.s,v 1.6 89/07/31 17:40:11 douglis Exp Locker: mottsmth $ SPRITE (Berkeley)
 *
 */

#include "userSysCallInt.h"

SYS_CALL(Proc_Debug,			SYS_PROC_DEBUG)
SYS_CALL(Proc_Detach,			SYS_PROC_DETACH)
SYS_CALL(Proc_RawExec,			SYS_PROC_EXEC)
SYS_CALL(Proc_Fork,			SYS_PROC_FORK)
SYS_CALL(Proc_ExecEnv,			SYS_PROC_EXEC_ENV)
SYS_CALL(Proc_RawRemoteExec,		SYS_PROC_REMOTE_EXEC)
SYS_CALL(Proc_RawExit,			SYS_PROC_EXIT)
SYS_CALL(Proc_GetFamilyID,		SYS_PROC_GETFAMILYID)
SYS_CALL(Proc_GetGroupIDs,		SYS_PROC_GETGROUPIDS)
SYS_CALL(Proc_GetIDs,			SYS_PROC_GETIDS)
SYS_CALL(Proc_GetPCBInfo,		SYS_PROC_GETPCBINFO)
SYS_CALL(Proc_GetPriority,		SYS_PROC_GETPRIORITY)
SYS_CALL(Proc_GetResUsage,		SYS_PROC_GETRESUSAGE)
SYS_CALL(Proc_Profile,			SYS_PROC_PROFILE)
SYS_CALL(Proc_SetFamilyID,		SYS_PROC_SETFAMILYID)
SYS_CALL(Proc_SetGroupIDs,		SYS_PROC_SETGROUPIDS)
SYS_CALL(Proc_SetIDs,			SYS_PROC_SETIDS)
SYS_CALL(Proc_SetPriority,		SYS_PROC_SETPRIORITY)
SYS_CALL(Proc_RawWait,			SYS_PROC_WAIT)
SYS_CALL(Proc_OLDSetEnviron,		SYS_PROC_SET_ENVIRON)
SYS_CALL(Proc_OLDUnsetEnviron,		SYS_PROC_UNSET_ENVIRON)
SYS_CALL(Proc_OLDGetEnvironVar, 	SYS_PROC_GET_ENVIRON_VAR)
SYS_CALL(Proc_OLDGetEnvironRange, 	SYS_PROC_GET_ENVIRON_RANGE)
SYS_CALL(Proc_OLDInstallEnviron, 	SYS_PROC_INSTALL_ENVIRON)
SYS_CALL(Proc_OLDCopyEnviron,		SYS_PROC_COPY_ENVIRON)
SYS_CALL(Proc_RawMigrate,		SYS_PROC_MIGRATE)
SYS_CALL(Proc_GetIntervalTimer,		SYS_PROC_GETINTERVALTIMER)
SYS_CALL(Proc_SetIntervalTimer,		SYS_PROC_SETINTERVALTIMER)
SYS_CALL(Proc_GetHostIDs,		SYS_PROC_GETHOSTIDS)
