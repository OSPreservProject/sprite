/*
 * sys.h --
 *
 *     Routines and types for the sys module.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/sys/sys.h,v 9.12 92/08/03 17:41:49 mgbaker Exp $ SPRITE (Berkeley)
 *
 */

#ifndef _SYS
#define _SYS

#ifndef _ASM

#ifdef KERNEL
#include <user/sys.h>
#include <sprite.h>
#include <status.h>
#include <spriteTime.h>
#else /* KERNEL */
#include <sys.h>
#include <sprite.h>
#include <status.h>
#include <spriteTime.h>
#endif /* KERNEL */

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
extern	Boolean sys_CallProfiling;	/* Set if timing system calls */
extern	int	sys_NumCalls[];
extern	char	sys_HostName[];		/* The name of this host. */
extern	Boolean	sys_DontPrint;		/* Turn off printing to console. */

extern void	Sys_Init _ARGS_((void));
extern void	Sys_SyncDisks _ARGS_((int trapType));
extern int	Sys_GetHostId _ARGS_((void));
extern void	Sys_HostPrint _ARGS_((int spriteID, char *string));
extern ReturnStatus Sys_GetTimeOfDay _ARGS_((Time *timePtr,
		    int *localOffsetPtr, Boolean *DSTPtr));
extern ReturnStatus Sys_SetTimeOfDay _ARGS_((Time *timePtr, int localOffset,
		    Boolean DST));
extern void	Sys_RecordCallStart _ARGS_((void));
extern void	Sys_RecordCallFinish _ARGS_((int callNum));
extern ReturnStatus Sys_GetHostName _ARGS_((char *name));
extern ReturnStatus Sys_SetHostName _ARGS_((char *name));

extern int	vprintf _ARGS_(());
extern void	panic _ARGS_(());

/* Temporary declaration until prototyping is done */
extern ReturnStatus Proc_RemoteDummy();

typedef struct unixSyscallEntry {
    int (*func)();
    int numArgs;
} unixSyscallEntry;

extern unixSyscallEntry sysUnixSysCallTable[];

#else

/*
 *  Declarations of system call stubs, which happen to have the
 *  same name as the user-visible routines.
 */

extern ReturnStatus Sys_GetTimeOfDay();
extern ReturnStatus Sys_SetTimeOfDay();
extern ReturnStatus Sys_DoNothing();
extern ReturnStatus Sys_Shutdown();
extern ReturnStatus Sys_GetMachineInfo();
extern ReturnStatus Sys_GetMachineInfoNew();
extern ReturnStatus Sys_GetHostName();
extern ReturnStatus Sys_SetHostName();

#endif /* KERNEL */
#endif /* _ASM */
#endif /* _SYS */
