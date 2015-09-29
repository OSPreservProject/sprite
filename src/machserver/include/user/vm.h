/*
 * vm.h --
 *
 *     User virtual memory structures.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/vm.h,v 1.3 92/04/16 11:11:43 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMUSER
#define _VMUSER

#include <sprite.h>
#include <cfuncproto.h>
#include <mach.h>
#include <sys/types.h>
#ifdef SPRITED
#include <user/vmTypes.h>
#else
#include <vmTypes.h>
#endif

/*
 * System calls (or what used to be system calls).
 */

extern ReturnStatus	Vm_MapFile _ARGS_((char *fileName, Boolean readOnly,
				off_t offset, vm_size_t length,
				Address *startAddrPtr));
extern ReturnStatus	Vm_PageSize _ARGS_((int *pageSizePtr));

#endif /* _VMUSER */
