/* 
 * sysStubs.c --
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
#include <user/sys/types.h>
#include <user/sys/wait.h>
#include <user/sys/time.h>
#include <user/sys/resource.h>
#include <mach.h>
#include <proc.h>
#include <vm.h>
#include <fsutil.h>
#include <assert.h>

int debugSysStubs;


/*
 *----------------------------------------------------------------------
 *
 * Sys_RebootStub --
 *
 *      The stub for the "reboot" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Sys_RebootStub(howto)
    int howto;
{
    printf("reboot is not implemented.\n");
    Mach_SetErrno(EINVAL);
    return -1;
}

#define MAX_HOST_NAME_LEN 255

char sysHostName[MAX_HOST_NAME_LEN + 1];
static int sysHostNameLen = 0;
static char sysDomainName[MAX_HOST_NAME_LEN + 1];
static int sysDomainNameLen = 0;
int sysHostID;


/*
 *----------------------------------------------------------------------
 *
 * Sys_GethostnameStub --
 *
 *      The stub for the "gethostname" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_GethostnameStub(name, namelen)
    char *name;
    int namelen;
{
    int copyLen;
    ReturnStatus status;

    if (debugSysStubs) {
	printf("Sys_GethostnameStub\n");
    }
    if (namelen < sysHostNameLen + 1) {
	copyLen = namelen;
    } else {
	copyLen = sysHostNameLen + 1;
    }
    status = Vm_CopyOut(copyLen, sysHostName, name);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_SethostnameStub --
 *
 *      The stub for the "sethostname" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_SethostnameStub(name, namelen)
    char *name;
    int namelen;
{
    Proc_ControlBlock	*procPtr = Proc_GetEffectiveProc();
    ReturnStatus	status;

    if (debugSysStubs) {
	printf("Sys_SethostnameStub\n");
    }
    if (procPtr->userID!=0) {
	Mach_SetErrno(EPERM);
	return -1;
    }
    if (namelen > MAX_HOST_NAME_LEN) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Vm_CopyIn(namelen, name, sysHostName);
    if (status != SUCCESS) {
	sysHostNameLen = 0;
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    sysHostName[namelen] = 0;
    sysHostNameLen = namelen;
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GethostidStub --
 *
 *      The stub for the "gethostid" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_GethostidStub()

{

    if (debugSysStubs) {
	printf("Sys_GethostidStub\n");
    }
    return sysHostID;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_SethostidStub --
 *
 *      The stub for the "sethostid" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_SethostidStub(hostid)
    int hostid;
{

    if (debugSysStubs) {
	printf("Sys_SethostidStub\n");
    }
    sysHostID = hostid;
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetdomainnameStub --
 *
 *      The stub for the "getdomainname" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_GetdomainnameStub(name, namelen)
    char *name;
    int namelen;
{
    ReturnStatus	status;
    int copyLen;

    if (debugSysStubs) {
	printf("Sys_GetdomainnameStub\n");
    }
    if (namelen < sysDomainNameLen + 1) {
	copyLen = namelen;
    } else {
	copyLen = sysDomainNameLen + 1;
    }
    status = Vm_CopyOut(copyLen, sysDomainName, name);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_SetdomainnameStub --
 *
 *      The stub for the "setdomainname" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_SetdomainnameStub(name, namelen)
    char *name;
    int namelen;
{
    Proc_ControlBlock	*procPtr = Proc_GetEffectiveProc();
    ReturnStatus	status;

    if (procPtr->userID!=0) {
	Mach_SetErrno(EPERM);
	return -1;
    }
    if (debugSysStubs) {
	printf("Sys_SetdomainnameStub\n");
    }
    if (namelen > MAX_HOST_NAME_LEN) {
	Mach_SetErrno(EINVAL);
    }
    status = Vm_CopyIn(namelen, name, sysDomainName);
    if (status != SUCCESS) {
	sysDomainNameLen = 0;
	Mach_SetErrno(EFAULT);
	return -1;
    }
    sysDomainName[namelen] = 0;
    sysDomainNameLen = namelen;
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_ShutdownStub --
 *
 *      The stub for the "shutdown" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_ShutdownStub()
{

    printf("Sys_Shutdown is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetpeernameStub --
 *
 *      The stub for the "getpeername" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_GetpeernameStub()
{

    printf("Sys_Getpeername is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_Getrlimit --
 *
 *      The stub for the "getrlimit" Unix system call.
 *	We don't really implement this, so we return INFINITY for
 *	everything except the stack size.  For the stack size, we
 *	return the same as SunOS.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
#define RLIM_STACK_CUR	0x00200000
#define RLIM_STACK_MAX	0x80000000
int
Sys_GetrlimitStub(resource, rlp)
int resource;
struct rlimit *rlp;
{

    struct rlimit rl;
    ReturnStatus status;

    if (debugSysStubs) {
	printf("Sys_GethostnameStub(%d, %x)\n", resource, rlp);
    }
    switch (resource) {
	case RLIMIT_CPU:
	case RLIMIT_FSIZE:
	case RLIMIT_DATA:
	case RLIMIT_CORE:
	case RLIMIT_RSS:
	    rl.rlim_cur = RLIM_INFINITY;
	    rl.rlim_max = RLIM_INFINITY;
	case RLIMIT_STACK:
	    rl.rlim_cur = RLIM_STACK_CUR;
	    rl.rlim_max = RLIM_STACK_MAX;
	    break;
	default:
	    Mach_SetErrno(EINVAL);
	    return -1;
	    break;
    }
    status = Vm_CopyOut(sizeof(struct rlimit), (Address)&rl, (Address)rlp);
    if (status != SUCCESS) {
	Mach_SetErrno(EFAULT);
	return -1;
    }

    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_Setrlimit --
 *
 *      The stub for the "setrlimit" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Sys_SetrlimitStub()
{

    printf("Sys_Setrlimit not implemented\n");
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Sys_GetsysinfoStub --
 *
 *      The stub for the "getsysinfo" Unix system call.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Any side effects associated with the call.
 *
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Sys_GetsysinfoStub(op, buffer, nbytes, start, arg)

    unsigned  op;
    char      *buffer;
    unsigned   nbytes;
    int       *start;
    char       *arg;
{
#ifdef actually_do_it
    switch (op) {
	case GSI_PROG_ENV:
	case GSI_MAX_UPROCS:
	case GSI_TTYP:
	case GSI_NETBLK:
	case GSI_BOOTDEV:
	case GSI_UACSYS:
	case GSI_UACPARNT:
	case GSI_UACPROC:
	default:
    }
#endif

    printf("Sys_getsysinfo not implemented\n");
    /*
     * Just return a 0.  This says that the requested information is
     * not available which is certainly true for the most part.
     */
    return 0;
}

