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

#ifndef _ASM
#include "user/sys.h"
#include "sprite.h"
#include "status.h"
#endif /* _ASM */

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

#ifndef _ASM
#ifdef KERNEL

extern	Boolean	sys_ShuttingDown;	/* Set when halting */
extern	Boolean	sys_ErrorShutdown;	/* Set after a bad trap or error */
extern	Boolean	sys_ErrorSync;		/* Set while syncing disks */
extern	int	sys_NumCalls[];

extern	void	Sys_Init();
extern	void	printf();
extern	void	panic();
extern	int	vprintf();

extern  ReturnStatus	Sys_OutputNumCalls();
extern	void		Sys_HostPrint();

#endif /* KERNEL */

/*
 *  Declarations of system call stubs, which happen to have the
 *  same name as the user-visible routines.
 */

extern	ReturnStatus	Sys_GetTimeOfDay();
extern	ReturnStatus	Sys_SetTimeOfDay();
extern	ReturnStatus	Sys_DoNothing();
extern	ReturnStatus	Sys_Shutdown();
extern	ReturnStatus	Sys_GetMachineInfo();
extern	ReturnStatus	Sys_GetMachineInfoNew();

#endif /* _ASM */
#endif /* _SYS */
