/*
 * sysInt.h --
 *
 *     Routines and types used internally by the sys module.
 *
 * Copyright 1990 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _SYSINT
#define _SYSINT

#include <sprite.h>
#include <user/sys/time.h>
#include <user/sys/resource.h>

extern Boolean sysPanicing;

extern ReturnStatus Sys_DoNothing _ARGS_((void));
extern ReturnStatus Sys_Shutdown _ARGS_((int flags, char *rebootString));
extern ReturnStatus Sys_GetMachineInfo _ARGS_((int *archPtr, int *typePtr,
		int *clientIDPtr));
extern ReturnStatus Sys_GetMachineInfoNew _ARGS_((int infoSize,
		Address infoBufPtr));
extern ReturnStatus Sys_StatsStub _ARGS_((int command, int option,
		Address argPtr));
extern void SysInitSysCall _ARGS_((void));
extern ReturnStatus Sys_OutputNumCalls _ARGS_((int numToCopy,
		Address buffer));
extern int Sys_RebootStub _ARGS_((int howto));
extern int Sys_GethostnameStub _ARGS_((char *name, int namelen));
extern int Sys_SethostnameStub _ARGS_((char *name, int namelen));

extern int Sys_GethostidStub _ARGS_((void));
extern int Sys_SethostidStub _ARGS_((int hostid));
extern int Sys_GetdomainnameStub _ARGS_((char *name, int namelen));
extern int Sys_SetdomainnameStub _ARGS_((char *name, int namelen));
extern int Sys_ShutdownStub _ARGS_((void));
extern int Sys_GetpeernameStub _ARGS_((void));
extern int Sys_GetrlimitStub _ARGS_((int resource, struct rlimit *rlp));
extern int Sys_SetrlimitStub _ARGS_((void));
extern int Sys_GetsysinfoStub _ARGS_((unsigned op, char *buffer,
	unsigned nbytes, int *start, char *arg));

#endif /* _SYSINT */
