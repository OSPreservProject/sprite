/*
 * sys.h --
 *
 *     Routines and types for the sys module.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _SYS
#define _SYS

#include "user/sys.h"
#include "sprite.h"
#include "status.h"

/*
 * Stuff for system calls.
 *
 * SYS_ARG_OFFSET	Where the system calls arguments begin.
 * SYS_MAX_ARGS		The maximum number of arguments possible to be passed
 *			    to a system call.
 * SYS_ARG_SIZE		The size in bytes of each argument.
 */

#define	SYS_ARG_OFFSET	8
#define	SYS_MAX_ARGS	10
#define	SYS_ARG_SIZE	4

extern	Boolean	sys_ShuttingDown;
extern	Boolean	sys_ErrorShutdown;
extern	int	sys_NumCalls[];

extern	void	Sys_Init();
extern	void	Sys_Printf();
extern	void	Sys_SafePrintf();
extern	void	Sys_UnSafePrintf();
extern	void	Sys_HostPrint();
extern	void	Sys_Panic();

/*
 *  Declarations of system calls.
 */

extern	ReturnStatus	Sys_GetTimeOfDay();
extern	ReturnStatus	Sys_SetTimeOfDay();
extern	ReturnStatus	Sys_DoNothing();
extern	ReturnStatus	Sys_Shutdown();
extern	ReturnStatus	Sys_GetMachineInfo();

extern  ReturnStatus	Sys_OutputNumCalls();

#endif _SYS
