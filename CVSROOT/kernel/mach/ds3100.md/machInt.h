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

#include "machMon.h"
#include "mach.h"

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern Address MachFetchArgs;
extern Address MachFetchArgsEnd;

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
extern void		MachUTLBMiss();
extern void		MachEndUTLBMiss();
extern void 		MachException();
extern void		MachEndException();
extern ReturnStatus 	MachFetch0Args();
extern ReturnStatus 	MachFetch1Arg();
extern ReturnStatus 	MachFetch2Args();
extern ReturnStatus 	MachFetch3Args();
extern ReturnStatus 	MachFetch4Args();
extern ReturnStatus 	MachFetch5Args();
extern ReturnStatus 	MachFetch6Args();
extern void		MachSysCall();
extern void		MachProbeAddrEnd();
extern unsigned *MachEmulateBranch _ARGS_((unsigned *regsPtr, Address instPC, unsigned fpcCSR, Boolean allowNonBranch));


extern void Mach_SendSignal _ARGS_((int sigType));

extern ReturnStatus CvtSpriteToUnixAtts _ARGS_((register Fs_Attributes *spriteAttsPtr, register struct stat *unixAttsPtr));
extern Boolean MachUserExceptionHandler _ARGS_((unsigned statusReg, unsigned causeReg, Address badVaddr, Address pc));
extern int MachKernelExceptionHandler _ARGS_((unsigned statusReg, unsigned causeReg, Address badVaddr, Address pc));


#endif /* _MACHINT */
