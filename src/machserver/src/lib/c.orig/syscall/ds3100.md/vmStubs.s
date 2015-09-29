/*
 *
 * vmStubs.s --
 *
 *     Stubs for the Vm_ system calls.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * rcs = $Header: /sprite/src/lib/c/syscall/ds3100.md/RCS/vmStubs.s,v 1.4 90/07/26 18:51:55 shirriff Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"

SYS_CALL(Vm_Cmd, SYS_VM_CMD)
SYS_CALL(Vm_GetSegInfo, SYS_VM_GETSEGINFO)
SYS_CALL(Vm_PageSize, SYS_VM_PAGESIZE)
SYS_CALL(Vm_CreateVA, SYS_VM_CREATEVA)
SYS_CALL(Vm_DestroyVA, SYS_VM_DESTROYVA)
SYS_CALL(Vm_MapKernelIntoUser, SYS_VM_MAPKERNELINTOUSER)
SYS_CALL(Vm_Mmap, SYS_VM_MMAP)
SYS_CALL(Vm_Munmap, SYS_VM_MUNMAP)
SYS_CALL(Vm_Msync, SYS_VM_MSYNC)
SYS_CALL(Vm_Mlock, SYS_VM_MLOCK)
SYS_CALL(Vm_Munlock, SYS_VM_MUNLOCK)
SYS_CALL(Vm_Mincore, SYS_VM_MINCORE)
SYS_CALL(Vm_Mprotect, SYS_VM_MPROTECT)
