/* 
 * vmStubs.c --
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
#include <status.h>
#include <errno.h>
#include <user/sys/types.h>
#include <mach.h>
#include <proc.h>
#include <timer.h>
#include <vm.h>

#ifndef Mach_SetErrno
#define Mach_SetErrno(err)
#endif

int debugVmStubs;

int
Vm_SbrkStub(addr)
    Address	addr;
{
    Vm_Segment	        *segPtr;
    Address	        lastAddr;
    Proc_ControlBlock	*procPtr;

    if (debugVmStubs) {
	printf("Vm_SbrkStub(0x%x)\n", addr);
    }

    /*
     * The UNIX brk and sbrk call stubs figure where the end of the
     * heap is and they always call us with the new end of data segment.
     */
    procPtr = Proc_GetCurrentProc();
    segPtr = procPtr->vmPtr->segPtrArray[VM_HEAP];
    if (segPtr != (Vm_Segment *)NIL) {
	lastAddr =
		(Address) ((segPtr->offset + segPtr->numPages) * vm_PageSize);
	if (Vm_CreateVA(lastAddr, addr - lastAddr) == SUCCESS) {

	    if (debugVmStubs) {
		printf("Vm_SbrkStub addr = %x, lastAddr = %x, newAddr = %x\n",
		    addr, lastAddr,
		    (segPtr->offset + segPtr->numPages) * vm_PageSize);
	    }

	    return 0;
	}
    }

    if (debugVmStubs) {
	printf("Vm_SbrkStub Failed\n", addr);
    }

    Mach_SetErrno(ENOMEM);
    return -1;
}

int
Vm_GetpagesizeStub()
{

    if (debugVmStubs) {
	printf("Vm_GetpagesizeStub\n");
    }
    return vm_PageSize;
}

int
Vm_SemctlStub(semid, semnum, cmd, arg)

    int semid, semnum, cmd;
    union semun arg;
{
#if 0    
    ReturnStatus	status;

    status = Sync_SemctlStub(semid, semnum, cmd, arg, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->unixRetVal);
    }
    return(status);
#else
    printf("sysUnixSemctl is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
#endif    
}

int
Vm_SemopStub(semid, sops, nsops)
    int semid, nsops;
    struct sembuf *sops;
{
#if 0    
    ReturnStatus	status;

    status = Sync_SemopStub(semid, sops, nsops, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->unixRetVal);
    }
    return(status);
#else
    printf("Vm_SemopStub is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
#endif    
}

int
Vm_SemgetStub(key, nsems, semflg)
    key_t key;
    int nsems, semflg;
{
#if 0    
    ReturnStatus	status;

    status = Sync_SemgetStub(key, nsems, semflg, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->unixRetVal);
    }
    return(status);
#else
    printf("Vm_SemgetStub is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
#endif    
}

int
Vm_MmapStub(addr, len, prot, share, fd, pos)
    caddr_t	addr;
    int	len, prot, share, fd;
    off_t	pos;
{
#if 0    
    ReturnStatus	status;

    status = Vm_Mmap(addr, len, prot, share, fd, pos, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->unixRetVal);
    }
    return(status);
#else
    printf("Vm_MmapStub is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
#endif    
}

int
Vm_MunmapStub(addr, len)
    caddr_t	addr;
    int	len;
{
#if 0    
    ReturnStatus	status;

    status = Vm_Munmap(addr, len, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->unixRetVal);
    }
    return(status);
#else
    printf("Vm_MunmapStub is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
#endif    

}

int
Vm_MincoreStub(addr, len, vec)
    caddr_t	addr;
    int len;
    char vec[];
{
#if 0
    ReturnStatus	status;

    status = Vm_Mincore(addr, len, vec, usp);
    if (status == SUCCESS) {
        (void)Vm_CopyIn(sizeof(int), usp,
                        (Address)&machCurStatePtr->unixRetVal);
    }
    return(status);
#else
    printf("Vm_MincoreStub is not implemented\n");
    Mach_SetErrno(EINVAL);
    return -1;
#endif
}

