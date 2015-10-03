/*
 * machInt.h --
 *
 *	This file defines things that are shared between the "mach" modules
 *	but aren't used by the rest of Sprite.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#ifndef _MACHINT
#define _MACHINT

#include <machMon.h>
#include <mach.h>
#include <machConst.h>
#include <machAddrs.h>

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern void MachFetchArgs _ARGS_((void));
extern void MachFetchArgsEnd _ARGS_((void));

#ifdef notdef
/*
 * Copy of the boot parameter structure.
 */
extern 	MachStringTable	machMonBootParam;
#endif

/*
 * Internal variables.
 */
extern Mach_State *machCurStatePtr;
extern Mach_State *machFPCurStatePtr;

/*
 * Internal functions.
 */
extern Boolean MachUserReturn _ARGS_((register Proc_ControlBlock *procPtr));
/*
 * These sorta take an argument and sorta don't.  The user's stack pointer is
 * in register t2.
 */
extern void 			MachException _ARGS_((void));
extern void			MachEndException _ARGS_((void));
extern ReturnStatus 		MachFetch0Args _ARGS_((void));
extern ReturnStatus 		MachFetch1Arg _ARGS_((void));
extern ReturnStatus 		MachFetch2Args _ARGS_((void));
extern ReturnStatus 		MachFetch3Args _ARGS_((void));
extern ReturnStatus 		MachFetch4Args _ARGS_((void));
extern ReturnStatus 		MachFetch5Args _ARGS_((void));
extern ReturnStatus 		MachFetch6Args _ARGS_((void));

extern void		MachSysCall _ARGS_((void));
extern unsigned *MachEmulateBranch _ARGS_((unsigned *regsPtr, Address instPC, unsigned fpcCSR, Boolean allowNonBranch));


extern void Mach_SendSignal _ARGS_((int sigType));

extern ReturnStatus CvtSpriteToUnixAtts _ARGS_((register Fs_Attributes *spriteAttsPtr, register struct stat *unixAttsPtr));
extern Boolean MachUserExceptionHandler _ARGS_((unsigned statusReg, unsigned causeReg, Address badVaddr, Address pc));
extern int MachKernelExceptionHandler _ARGS_((unsigned statusReg, unsigned causeReg, Address badVaddr, Address pc));
extern void		MachProbeEnd _ARGS_((void));

#endif /* _MACHINT */
