/*
 * sigMach.h --
 *
 *	Declarations of machine-dependent signals routines.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/sig/ds3100.md/RCS/sigMach.h,v 1.2 92/03/12 17:50:10 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SIGMACH
#define _SIGMACH

#include <cfuncproto.h>
#include <sprite.h>
#include <procTypes.h>
#include <sigTypes.h>

extern char *mach_mips_exception_string(); /* XXX */

#define SigMach_ExceptionString(a, b, c) \
    mach_mips_exception_string((a), (b), (c))

extern void SigMach_ExcToSig _ARGS_((int exceptionType, int exceptionCode, 
			int exceptionSubcode, int *sigNumPtr, int *codePtr,
			Address *sigAddrPtr));
extern ReturnStatus SigMach_RestoreState _ARGS_((Proc_LockedPCB *procPtr,
			Sig_Context *sigContextPtr));
extern ReturnStatus SigMach_SetSignalState _ARGS_((Proc_LockedPCB *procPtr,
			Sig_Stack *sigStackPtr, Address pc));

#endif /* _SIGMACH */
