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

#include "sprite.h"

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

#endif /* _SYSINT */
