/* 
 * procStubs.c --
 *
 *	Stubs for Unix compatible system calls.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#define MACH_UNIX_COMPAT

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <status.h>
#include <errno.h>
#include <procUnixStubs.h>
#include <user/sys/types.h>
#include <user/sys/wait.h>
#include <user/sys/time.h>
#include <user/sys/resource.h>
#include <mach.h>
#include <proc.h>
#include <procInt.h>
#include <vm.h>
#include <fsutil.h>
#include <assert.h>

int debugProcStubs;

#if defined(ds3100) || defined(ds5000)
extern Mach_State *machCurStatePtr;
#endif

/*
 *----------------------------------------------------------------------
 *
 * copyin --
 *
 *	Copys a string from user space.
 *
 * Results:
 *	Returns string.
 *
 * Side effects:
 *	Copies the string.
 *	 
 *
 *----------------------------------------------------------------------
 */
static char *
copyin(string)
    char *string;
{
    static char buf[FS_MAX_PATH_NAME_LENGTH + 1];
    int x;

    assert(debugProcStubs);
    Fsutil_StringNCopy(FS_MAX_PATH_NAME_LENGTH, string, buf, &x);
    return buf;
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_ExitStub --
 *
 *	The stub for the "exit" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_ExitStub(arg0)
    int arg0;
{

    if (debugProcStubs) {
	printf("Proc_ExitStub(0x%x)\n", arg0);
    }
    Proc_Exit(arg0);
    return -1;
}
/*
 *----------------------------------------------------------------------
 *
 * Proc_ForkStub --
 *
 *	The stub for the "fork" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_ForkStub()
{
    ReturnStatus	status;
    Proc_PID newPid;

    if (debugProcStubs) {
	printf("Proc_ForkStub\n");
    }
#if defined(ds3100) || defined(ds5000)
    /*
     * Put the right values in V1 and A3 for the child because the process
     * jumps directly to user space after it is created.  A 1 in v1 
     * is what the system call stub in the user process expects for the
     * child.
     */
    machCurStatePtr->userState.regState.regs[V1] = 1;
    machCurStatePtr->userState.regState.regs[A3] = 0;
#else
    Mach_Return2(1);
#endif
    status = Proc_NewProc((Address) 0, PROC_USER, FALSE, &newPid, (char *) NIL);
    if (status == PROC_CHILD_PROC) {
	panic("Proc_ForkStub: Child came alive here?\n");
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
#if defined(ds3100) || defined(ds5000)
    machCurStatePtr->userState.regState.regs[V1] = 0;
#else
    Mach_Return2(0);
#endif
    return (int) newPid;
}

int
Proc_VforkStub()
{
    ReturnStatus	status;
    Proc_PID newPid;

    if (debugProcStubs) {
	printf("Proc_VForkStub\n");
    }
#if defined(ds3100) || defined(ds5000)
    /*
     * Put the right values in V1 and A3 for the child because the process
     * jumps directly to user space after it is created.  A 1 in v1 
     * is what the system call stub in the user process expects for the
     * child.
     */
    machCurStatePtr->userState.regState.regs[V1] = 1;
    machCurStatePtr->userState.regState.regs[A3] = 0;
#else
    Mach_Return2(1);
#endif
    status = Proc_NewProc(0, PROC_USER, TRUE, &newPid, (char *) NIL);
    if (status == PROC_CHILD_PROC) {
	panic("Proc_VforkStub: Child came alive here?\n");
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
#if defined(ds3100) || defined(ds5000)
    machCurStatePtr->userState.regState.regs[V1] = 0;
#else
    Mach_Return2(0);
#endif
    return (int) newPid;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ExecveStub --
 *
 *	The stub for the "execve" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_ExecveStub(name, argv, envp)
    char *name;			/* name of file to exec */
    char **argv;		/* array of arguments */
    char **envp;		/* array of environment pointers */
{
    ReturnStatus status;

    if (debugProcStubs) {
	printf("Proc_ExecveStub(%s)\n", copyin(name));
    }
    status = Proc_ExecEnv(name, argv, envp, FALSE);
    Mach_SetErrno(Compat_MapCode(status));
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ExecvStub --
 *
 *	The stub for the "execv" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_ExecvStub(name, argv)
    char *name;			/* Name of file containing program to exec. */
    char **argv;		/* Array of arguments to pass to program. */
{
    ReturnStatus status;

    if (debugProcStubs) {
	printf("Proc_ExecvStub(%s)\n", copyin(name));
    }
    status = Proc_Exec(name, argv, USER_NIL, FALSE, 0);
    Mach_SetErrno(Compat_MapCode(status));
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetpidStub --
 *
 *	The stub for the "getpid" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetpidStub()
{

    if (debugProcStubs) {
	printf("Proc_GetpidStub: %x\n", Proc_GetEffectiveProc()->processID);
    }
    Mach_Return2(Proc_GetEffectiveProc()->parentID);
    return Proc_GetEffectiveProc()->processID;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetuidStub --
 *
 *	The stub for the "getuid" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetuidStub()
{
    Proc_ControlBlock *procPtr = Proc_GetEffectiveProc();
    int uid,euid;

    if (debugProcStubs) {
	printf("Proc_GetuidStub\n");
    }
    uid = procPtr->userID;
    euid = procPtr->effectiveUserID;
    /*
     * We have to return the effective user id via Mach_Return2.
     */
    Mach_Return2(euid);
    return uid;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_PtraceStub --
 *
 *	The stub for the "ptrace" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Proc_PtraceStub(request, pid, addr, data)
    int request, pid, *addr, data;
{

    printf("ptrace is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetgidStub --
 *
 *	The stub for the "getgid" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetgidStub()
{

    /*
     * The Sprite group id for Sprite at Berkeley.  Should do a better job
     * of this.
     */
    if (debugProcStubs) {
	printf("Proc_GetgidStub\n");
    }
    /*
     * We have to return the effective group id via Mach_Return2.
     */
    Mach_Return2(155);
    return 155;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_UmaskStub --
 *
 *	The stub for the "umask" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_UmaskStub(newPerm)
    unsigned int newPerm;
{
    unsigned int        oldPerm;
    Proc_ControlBlock	*procPtr;

    if (debugProcStubs) {
	printf("Proc_UmaskStub(0x%x)\n", newPerm);
    }
    procPtr = Proc_GetEffectiveProc();
    oldPerm = procPtr->fsPtr->filePermissions;
    procPtr->fsPtr->filePermissions = ~newPerm & 0777;
    return ~oldPerm & 0x777;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_GetgroupsStub --
 *
 *	The stub for the "getgroups" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetgroupsStub(gidsetlen, gidset)
    int gidsetlen;
    int *gidset;
{
    int trueGidlen;
    register	Fs_ProcessState 	*fsPtr;

    if (debugProcStubs) {
	printf("Proc_GetgroupsStub\n");
    }
    fsPtr = (Proc_GetEffectiveProc())->fsPtr;

    if (gidsetlen < 0) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    trueGidlen = gidsetlen < fsPtr->numGroupIDs ? gidsetlen:fsPtr->numGroupIDs;
    if (trueGidlen > 0 && gidset != USER_NIL) {
	if (Proc_ByteCopy(FALSE, trueGidlen * sizeof(int),
	                  (Address) fsPtr->groupIDs,
			  (Address) gidset) != SUCCESS) {
            Mach_SetErrno(EFAULT);
	    return -1;
	}
    }
    return trueGidlen;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetgroupsStub --
 *
 *	The stub for the "setgroups" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_SetgroupsStub(ngroups, gidset)
    int ngroups;
    int *gidset;
{
    ReturnStatus status;

    if (debugProcStubs) {
	printf("Proc_SetgroupsStub\n");
    }
    status = Proc_SetGroupIDs(ngroups, gidset);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetpgrpStub --
 *
 *	The stub for the "getpgrp" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetpgrpStub(pid)
    Proc_PID	pid;
{
    ReturnStatus        status;
    Proc_PID	        familyID;
    Proc_ControlBlock 	*procPtr;

    if (debugProcStubs) {
	printf("Proc_GetpgrpStub\n");
    }
    if (pid == 0 || pid == PROC_MY_PID) {
	procPtr = Proc_GetEffectiveProc();
	Proc_Lock(procPtr);
    } else {
	/*
	 *   Get the PCB entry for the given process.
	 */
	procPtr = Proc_LockPID(pid);
	status = SUCCESS;
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    status = PROC_INVALID_PID;
	} else if (!Proc_HasPermission(procPtr->effectiveUserID)) {
	    Proc_Unlock(procPtr);
	    status = PROC_UID_MISMATCH;
	}
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    familyID = procPtr->familyID;
    Proc_Unlock(procPtr);
    return familyID;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetpgrpStub --
 *
 *	The stub for the "setpgrp" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_SetpgrpStub(pid, pgrp)
    int pid;
    int pgrp;
{
    ReturnStatus status;

    if (debugProcStubs) {
	printf("Proc_SetpgrpStub\n");
    }
    if (pid == 0) {
	pid = PROC_MY_PID;
    }
    status = Proc_SetFamilyID(pid, pgrp);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Wait4Stub --
 *
 *	The stub for the "wait4" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_Wait4Stub(pid, statusPtr, options, unixRusagePtr)
    int			pid;
    union	wait	*statusPtr;
    int			options;
    struct	rusage	*unixRusagePtr;
{
    ReturnStatus        status;
    int	                flags;
    union wait          waitStatus;
    struct	rusage	unixRusage;
    Proc_ControlBlock	*curProcPtr;
    ProcChildInfo	childInfo;
    Proc_ResUsage 	resUsage;
    extern ReturnStatus DoWait();
    extern ReturnStatus Compat_SpriteSignalToUnix();
    int			numPids = 0;
    Proc_PID		*pidArray = 0;

    if (debugProcStubs) {
	printf("Proc_Wait4Stub(%x, %x, %x, %x)\n", pid, statusPtr, options,
		unixRusagePtr);
    }

    if (pid<0) {
	printf("Proc_Wait4Stub: wait on pgrp not implemented\n");
	Mach_SetErrno(EINVAL);
	return -1;
    } else if (pid != 0) {
	pidArray = (Proc_PID *)&pid;
	numPids = 1;
    }

    flags = 0;
    if (!(options & WNOHANG)) {
	flags |= PROC_WAIT_BLOCK;
    }
    if (options & WUNTRACED) {
	flags |= PROC_WAIT_FOR_SUSPEND;
    }
    curProcPtr = Proc_GetCurrentProc();
    if (curProcPtr->genFlags & PROC_FOREIGN) {
	status = ProcRemoteWait(curProcPtr, flags, numPids,
	                        pidArray, &childInfo);
    } else {
	status = DoWait(curProcPtr, flags, numPids, pidArray, &childInfo);
    }

    if (debugProcStubs) {
	printf("Proc_Wait4Stub: status = %x, child pid = %x, status = %x, code = %x\n",
		status, childInfo.processID,  childInfo.termStatus,
		childInfo.termCode);
    }

    if (status == GEN_ABORTED_BY_SIGNAL) {
	if (debugProcStubs) {
	    printf("Wait interrupted by signal\n");
	}
	curProcPtr->unixProgress = PROC_PROGRESS_RESTART;
	return 0;
    } else if (status != SUCCESS) {
	if (status == PROC_NO_EXITS && (options & WNOHANG)) {
	    if (debugProcStubs) {
		printf("Proc_Wait4Stub: exiting\n");
	    }
	    return 0;
	}
	Mach_SetErrno(ECHILD);
	return -1;
    }
    if (statusPtr != NULL)  {
	int	unixSignal;

	waitStatus.w_status = 0;
	if (childInfo.termReason == PROC_TERM_SUSPENDED) {
	    (void)Compat_SpriteSignalToUnix(childInfo.termStatus, &unixSignal);
	    waitStatus.w_stopval = WSTOPPED;
	    waitStatus.w_stopsig = unixSignal;
	} else if (childInfo.termReason == PROC_TERM_SIGNALED ||
		   childInfo.termReason == PROC_TERM_RESUMED) {
	    (void)Compat_SpriteSignalToUnix(childInfo.termStatus, &unixSignal);
	    waitStatus.w_termsig = unixSignal;
	    /* NEED TO HANDLE coredump FIELD */
	} else {
	    waitStatus.w_retcode = childInfo.termStatus;
	}
	status = Vm_CopyOut(sizeof(waitStatus), (Address)&waitStatus,
			    (Address)statusPtr);
	if (status != SUCCESS) {
	    Mach_SetErrno(EFAULT);
	    return -1;
	}
    }
    if (unixRusagePtr != NULL) {
	Time totalKTime;
	Time totalUTime;

	/*
	 * Convert the usages from the internal Timer_Ticks format
	 * into the external Time format.
	 */
	Timer_TicksToTime(childInfo.kernelCpuUsage, &resUsage.kernelCpuUsage);
	Timer_TicksToTime(childInfo.userCpuUsage, &resUsage.userCpuUsage);
	Timer_TicksToTime(childInfo.childKernelCpuUsage, 
	                  &resUsage.childKernelCpuUsage);
        Timer_TicksToTime(childInfo.childUserCpuUsage,
			      &resUsage.childUserCpuUsage);
	resUsage.numQuantumEnds = childInfo.numQuantumEnds;
	resUsage.numWaitEvents = childInfo.numWaitEvents;
	/*
	 * Return the total time used by the process and all its children.
	 */
	bzero((char *) &unixRusage, sizeof(*unixRusagePtr));
	Time_Add(resUsage.userCpuUsage, resUsage.childUserCpuUsage,
					    &totalUTime);
	Time_Add(resUsage.kernelCpuUsage,
		 resUsage.childKernelCpuUsage, &totalKTime);
	unixRusage.ru_utime.tv_sec = totalUTime.seconds;
	unixRusage.ru_utime.tv_usec = totalUTime.microseconds;
	unixRusage.ru_stime.tv_sec = totalKTime.seconds;
	unixRusage.ru_stime.tv_usec = totalKTime.microseconds;
	unixRusage.ru_nvcsw = resUsage.numWaitEvents;
	unixRusage.ru_nivcsw = resUsage.numQuantumEnds;
	status = Vm_CopyOut(sizeof(unixRusage), (Address)&unixRusage,
			    (Address)unixRusagePtr);
	if (status != SUCCESS) {
	    Mach_SetErrno(EFAULT);
	    return -1;
	}
    }
    if (debugProcStubs) {
	printf("Proc_Wait4Stub: returning %x\n", childInfo.processID);
    }
    Mach_Return2(*(int *)&waitStatus);
    return childInfo.processID;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_SetpriorityStub --
 *
 *	The stub for the "setpriority" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Proc_SetpriorityStub(which, who, prio)
    int which, who, prio;
{

    printf("Proc_Setpriority not implemented\n");
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetpriorityStub --
 *
 *	The stub for the "getpriority" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Proc_GetpriorityStub(which, who, prio)
    int which, who, prio;
{

    printf("Proc_GetpriorityStub not implemented\n");
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_SetreuidStub --
 *
 *	The stub for the "setreuid" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_SetreuidStub(userID, effUserID)
    int 	userID;
    int 	effUserID;
{
    ReturnStatus	status;
    Proc_ControlBlock 	*procPtr;

    if (debugProcStubs) {
	printf("Proc_SetreuidStub\n");
    }
    procPtr = Proc_GetEffectiveProc();
    if (userID == -1 || userID == PROC_NO_ID) {
	userID = procPtr->userID;;
    }
    if (effUserID == -1 || effUserID == PROC_NO_ID) {
	effUserID = procPtr->effectiveUserID;
    }
    if (userID != procPtr->userID && userID != procPtr->effectiveUserID &&
	procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
	Mach_SetErrno(Compat_MapCode(PROC_UID_MISMATCH));
	return -1;
    }
    if (effUserID != procPtr->userID && effUserID != procPtr->effectiveUserID &&
	procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
	Mach_SetErrno(Compat_MapCode(PROC_UID_MISMATCH));
	return -1;
    }
    procPtr->userID = userID;
    procPtr->effectiveUserID = effUserID;
    if (procPtr->state == PROC_MIGRATED) {
	status = Proc_MigUpdateInfo(procPtr);
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
	}
    }
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_SetregidStub --
 *
 *	The stub for the "setregid" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_SetregidStub(rgid, egid)
    int	rgid, egid;
{
    int		array[2];
    int		num = 0;
    Proc_ControlBlock 	*procPtr;
    Fs_ProcessState 	*fsPtr;
    int i;

    if (debugProcStubs) {
	printf("Proc_SetregidStub\n");
    }
    if (rgid != -1) {
	array[0] = rgid;
	num = 1;
	if (egid != rgid && egid != -1) {
	    array[1] = egid;
	    num++;
	}
    } else if (egid != -1) {
	array[0] = egid;
	num++;
    }
    if (num > 0) {
	/*
	 * Need to protect against abritrary group setting.
	 */
	procPtr = Proc_GetEffectiveProc();
	if (procPtr->effectiveUserID != 0) {
	    Mach_SetErrno(EPERM);
	    return -1;
	}

	/*
	 *  If the current group ID table is too small, allocate space
	 *	for a larger one.
	 */
	fsPtr = procPtr->fsPtr;
	if (fsPtr->numGroupIDs < num) {
	    free((Address) fsPtr->groupIDs);
	    fsPtr->groupIDs = (int *) malloc(num * sizeof(int));
	}
	for (i = 0; i < num; i++) {
	    fsPtr->groupIDs[i] = array[i];
	}  
	fsPtr->numGroupIDs = num;
    }
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_GetrlimitStub --
 *
 *	The stub for the "getrlimit" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetrlimitStub()
{

    printf("Proc_Getrlimit not implemented\n");
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetrlimitStub --
 *
 *	The stub for the "setrlimit" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_SetrlimitStub()
{

    printf("Proc_Setrlimit not implemented\n");
    return 0;
}


#define COPYTIME(TO,FROM) { \
	    (TO).tv_sec = (FROM).seconds; \
	    (TO).tv_usec = (FROM).microseconds; \
	  }


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetrusageStub --
 *
 *	The stub for the "getrusage" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetrusageStub(who, rusage)
    int who;
    struct rusage *rusage;
{
    Proc_ResUsage spriteUsage; 	    /* sprite resource usage buffer */
    struct rusage unixUsage;
    register Proc_ControlBlock 	*procPtr;
    ReturnStatus status;

    if (debugProcStubs) {
	printf("Proc_GetrusageStub\n");
    }

    procPtr = Proc_GetEffectiveProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_GetResUsage: procPtr == NIL\n");
    } 
    Proc_Lock(procPtr);
    Timer_TicksToTime(procPtr->kernelCpuUsage.ticks,
	&spriteUsage.kernelCpuUsage);
    Timer_TicksToTime(procPtr->userCpuUsage.ticks, &spriteUsage.userCpuUsage);
    Timer_TicksToTime(procPtr->childKernelCpuUsage.ticks,
	&spriteUsage.childKernelCpuUsage);
    Timer_TicksToTime(procPtr->childUserCpuUsage.ticks,
	&spriteUsage.childUserCpuUsage);
    spriteUsage.numQuantumEnds = procPtr->numQuantumEnds;
    spriteUsage.numWaitEvents 	= procPtr->numWaitEvents;
    Proc_Unlock(procPtr);
    if (who == RUSAGE_SELF) {
	COPYTIME(unixUsage.ru_utime, spriteUsage.userCpuUsage);
	COPYTIME(unixUsage.ru_stime, spriteUsage.kernelCpuUsage);
    } else {
	COPYTIME(unixUsage.ru_utime, spriteUsage.childUserCpuUsage);
	COPYTIME(unixUsage.ru_stime, spriteUsage.childKernelCpuUsage);
    }
    unixUsage.ru_maxrss = 0;
    unixUsage.ru_ixrss = 0;	/* integral shared memory size */
    unixUsage.ru_idrss = 0;	/* integral unshared data size */
    unixUsage.ru_isrss = 0;	/* integral unshared stack size */
    unixUsage.ru_minflt = 0;	/* page reclaims */
    unixUsage.ru_majflt = 0;	/* page faults */
    unixUsage.ru_nswap = 0;	/* swaps */
    unixUsage.ru_inblock = 0;	/* block input operations */
    unixUsage.ru_oublock = 0;	/* block output operations */
    unixUsage.ru_msgsnd = 0;	/* messages sent */
    unixUsage.ru_msgrcv = 0;	/* messages received */
    unixUsage.ru_nsignals = 0;	/* signals received */
    unixUsage.ru_nvcsw =
        spriteUsage.numWaitEvents;  /* voluntary context switches */
    unixUsage.ru_nivcsw =
        spriteUsage.numQuantumEnds;  /* involuntary context switches */
    status = Vm_CopyOut(sizeof(unixUsage), (Address)&unixUsage, 
			  (Address)rusage);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetitimerStub --
 *
 *	The stub for the "getitimer" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_GetitimerStub(which, value)
    int which;
    struct itimerval *value;
{
    ReturnStatus	status;

    if (debugProcStubs) {
	printf("Proc_GetitimerStub\n");
    }
    status = Proc_GetIntervalTimer(which, (Proc_TimerInterval *) value);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetitimerStub --
 *
 *	The stub for the "setitimer" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_SetitimerStub(which, value, ovalue)

    int which;
    struct itimerval *value;
    struct itimerval *ovalue;
{
    ReturnStatus	status;

    if (debugProcStubs) {
	printf("Proc_SetitimerStub\n");
    }
    status = Proc_SetIntervalTimer(which, (Proc_TimerInterval *) value,
				 (Proc_TimerInterval *) ovalue);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_Wait3Stub --
 *
 *	The stub for the "wait3" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_Wait3Stub(statusPtr, options, unixRusagePtr)
    union	wait	*statusPtr;
    int			options;
    struct	rusage	*unixRusagePtr;
{
    if (debugProcStubs) {
	printf("Proc_Wait4Stub(%x, %x, %x)\n", statusPtr, options,
		unixRusagePtr);
    }
    return Proc_Wait4Stub(0, statusPtr, options, unixRusagePtr);
}

#if defined(ds3100) || defined(ds5000)
/*
 *----------------------------------------------------------------------
 *
 * Proc_WaitpidStub --
 *
 *	The stub for the "waitpid" Unix system call.
 *
 * Results:
 *	Returns -1 on failure.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *	 
 *
 *----------------------------------------------------------------------
 */
int
Proc_WaitpidStub(pid, statusPtr, options)
    int			pid;
    union	wait	*statusPtr;
    int			options;
{
    if (pid==-1) {
	pid = 0;
    } else if (pid==0) {
	pid = -Proc_GetpgrpStub(pid);
    }
    return Proc_Wait4Stub(pid, statusPtr, options, 0);
}
#endif
