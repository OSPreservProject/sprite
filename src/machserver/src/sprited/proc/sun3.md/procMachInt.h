/*
 * procMachInt.h --
 *
 *	Internal machine-dependent declarations for process management.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/proc/sun3.md/RCS/procMachInt.h,v 1.4 92/05/08 15:10:37 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCMACHINT
#define _PROCMACHINT

#include <sprite.h>

#include <procTypes.h>

extern ReturnStatus	ProcMachGetUserRegs _ARGS_((Proc_LockedPCB *procPtr,
				Address *userPcPtr, Address *userStackPtr));
extern void		ProcMachInit _ARGS_((void));
extern ReturnStatus	ProcMachSetRegisters _ARGS_((
				Proc_ControlBlock *procPtr,
				Address stackPointer, Address startAddr));

#endif /* _PROCMACHINT */
