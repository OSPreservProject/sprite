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

#include "mach.h"
/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern int MachFetchArgs();
extern int MachFetchArgsEnd();
extern Address Mach_ProbeStart;
extern Address Mach_ProbeEnd;

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

extern	void		Mach_FreeState();
extern	ReturnStatus	Mach_SetupNewState();
extern void Mach_MonTrap();

#endif /* _MACHINT */
