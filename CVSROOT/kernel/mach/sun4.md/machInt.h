/*
 * machInt.h --
 *
 *	This file defines things that are shared between the "mach" modules
 *	but aren't used by the rest of Sprite.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHINT
#define _MACHINT

#include "sys/types.h"
#ifdef KERNEL
#include <mach.h>
#else
#include <kernel/mach.h>
#endif

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern void MachFetchArgs _ARGS_((void));
extern void MachFetchArgsEnd _ARGS_((void));
extern void MachProbeStart _ARGS_((void));
extern void MachProbeEnd _ARGS_((void));

extern void MachPageFault _ARGS_((unsigned int busErrorReg, Address addrErrorReg, unsigned int trapPsr, Address pcValue));
extern int MachUserAction _ARGS_((void));
extern void MachHandleTrap _ARGS_((int trapType, Address pcValue, unsigned int trapPsr));
extern void MachUserDebug _ARGS_((void));

/*
 * Asm routines.
 */
extern void MachRunUserProc _ARGS_((void));
extern void MachHandleBadQuickCopy _ARGS_((void));
extern void MachHandleBadProbe _ARGS_((void));
extern void MachHandleBadArgs _ARGS_((void));
extern void MachHandleBadArgs _ARGS_((void));
/*
 * FPU routines.
 */
extern void MachFPU_Emulate _ARGS_((int processID, Address instAddr, 
		Mach_RegState *userRegsPtr, Mach_RegWindow *curWinPtr));
extern void MachFPUDumpState _ARGS_((Mach_RegState *regStatePtr));
extern void MachFPULoadState _ARGS_((Mach_RegState *regStatePtr));

extern void MachCallSigReturn _ARGS_((void));
extern int fuword _ARGS_((caddr_t address));
extern int fubyte _ARGS_((caddr_t address));
extern int suword _ARGS_((caddr_t address, int value));

#endif /* _MACHINT */
