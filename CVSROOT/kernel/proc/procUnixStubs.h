/*
 * procUnixStubs.h --
 *
 *	External declarations for proc module Unix stubs.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * rcsid $Header$ SPRITE (Berkeley)
 */

#ifndef _PROC_UNIX_STUBS
#define _PROC_UNIX_STUBS

#include <sprite.h>
#include <procTypes.h>
#include <user/sys/wait.h>
#include <user/sys/time.h>
#include <user/sys/resource.h>

extern int Proc_ExitStub _ARGS_((int arg0));
extern int Proc_ForkStub _ARGS_((void));
extern int Proc_VforkStub _ARGS_((void));
extern int Proc_ExecveStub _ARGS_((char *name, char **argv, char **envp));
extern int Proc_ExecvStub _ARGS_((char *name, char **argv));
extern int Proc_GetpidStub _ARGS_((void));
extern int Proc_GetuidStub _ARGS_((void));
extern int Proc_PtraceStub _ARGS_((int request, int pid, int *addr, int data));
extern int Proc_GetgidStub _ARGS_((void));
extern int Proc_UmaskStub _ARGS_((unsigned int newPerm));
extern int Proc_GetgroupsStub _ARGS_((int gidsetlen, int *gidset));
extern int Proc_SetgroupsStub _ARGS_((int ngroups, int *gidset));
extern int Proc_GetpgrpStub _ARGS_((Proc_PID pid));
extern int Proc_SetpgrpStub _ARGS_((int pid, int pgrp));
extern int Proc_Wait4Stub _ARGS_((int pid, union wait *statusPtr, int options,
	struct rusage *unixRusagePtr));
extern int Proc_SetpriorityStub _ARGS_((int which, int who, int prio));
extern int Proc_GetpriorityStub _ARGS_((int which, int who, int prio));
extern int Proc_SetreuidStub _ARGS_((int userID, int effUserID));
extern int Proc_SetregidStub _ARGS_((int rgid, int egid));
extern int Proc_GetrlimitStub _ARGS_((void));
extern int Proc_SetrlimitStub _ARGS_((void));
extern int Proc_GetrusageStub _ARGS_((int who, struct rusage *rusage));
extern int Proc_GetitimerStub _ARGS_((int which, struct itimerval *value));
extern int Proc_SetitimerStub _ARGS_((int which, struct itimerval *value,
	struct itimerval *ovalue));
extern int Proc_Wait3Stub _ARGS_((union wait *statusPtr, int options,
	struct rusage *unixRusagePtr));

#if defined(ds3100) || defined(ds5000)
extern int Proc_WaitpidStub _ARGS_((int pid, union wait *statusPtr, int
	options));
#endif

/*
 * These flags indicate the progress of an interrupted system call.
 *
 * PROC_PROGRESS_UNIX		unix call, but no interruption.
 * PROC_PROGRESS_NOT_UNIX	not a unix call.
 * PROC_PROGRESS_RESTART	restart the interrupted call.
 * PROC_PROGRESS_MIG_RESTART	restart call if migrated.
 */
#define PROC_PROGRESS_UNIX	0
#define PROC_PROGRESS_NOT_UNIX	-1
#define PROC_PROGRESS_RESTART	-2
#define PROC_PROGRESS_MIG_RESTART	-3

#endif /* _PROC_UNIX_STUBS */
