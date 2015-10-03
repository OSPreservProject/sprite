/*
 * vmUnixStubs.h --
 *
 *     Virtual memory Unix compatibility stub declarations.
 *
 * Copyright 1991 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */


#ifndef _VM_UNIX_STUBS
#define _VM_UNIX_STUBS

#include <user/sys/types.h>

extern int Vm_SbrkStub _ARGS_((Address addr));
extern int Vm_GetpagesizeStub _ARGS_((void));
extern int Vm_MmapStub _ARGS_((caddr_t addr, int len, int prot, int share, int fd, off_t pos));
extern int Vm_MunmapStub _ARGS_((caddr_t addr, int len));
extern int Vm_MincoreStub _ARGS_((caddr_t addr, int len, char vec[]));
extern int Vm_MprotectStub _ARGS_((caddr_t addr, int len, int prot));

#endif /* _VM_UNIX_STUBS */
