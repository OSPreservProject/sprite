/*
 * vm.h --
 *
 *     User virtual memory structures.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/lib/include/RCS/vm.h,v 1.8 91/03/01 22:11:43 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMUSER
#define _VMUSER

#include <sprite.h>
#include <vmStat.h>
#ifdef KERNEL
#include <user/vmTypes.h>
#include <user/proc.h>
#else
#include <vmTypes.h>
#include <proc.h>
#endif

/*
 * System calls.
 */
extern	ReturnStatus	Vm_PageSize _ARGS_((int *pageSizePtr));
extern	ReturnStatus	Vm_CreateVA _ARGS_((Address address, int size));
extern	ReturnStatus	Vm_DestroyVA _ARGS_((Address address, int size));
extern	ReturnStatus	Vm_Cmd _ARGS_((int command, int arg));
extern	ReturnStatus	Vm_GetSegInfo _ARGS_((Proc_PCBInfo *infoPtr,
	Vm_SegmentID segID, int infoSize, Address segBufPtr));

#endif /* _VMUSER */
