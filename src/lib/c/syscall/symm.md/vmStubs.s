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
 * rcs = $Header: /crg2/bruces6/sprite/src/lib/c/syscall/sym.md/RCS/vmStubs.s,v 1.1 90/01/19 10:19:37 fubar Exp $ SPRITE (Berkeley)
 *
 */
#include "userSysCallInt.h"

SYS_CALL(2,	Vm_Cmd,		SYS_VM_CMD)
SYS_CALL(4,	Vm_GetSegInfo,	SYS_VM_GETSEGINFO)
SYS_CALL(1,	Vm_PageSize,	SYS_VM_PAGESIZE)
SYS_CALL(2,	Vm_CreateVA,	SYS_VM_CREATEVA)
SYS_CALL(2,	Vm_DestroyVA,	SYS_VM_DESTROYVA)
SYS_CALL(4,	Vm_MapKernelIntoUser,	SYS_VM_MAPKERNELINTOUSER)
SYS_CALL(7,	Vm_Mmap,	SYS_VM_MMAP)
SYS_CALL(3,	Vm_Munmap,	SYS_VM_MUNMAP)
