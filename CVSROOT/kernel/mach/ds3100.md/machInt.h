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

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern int MachFetchArgs();
extern int MachFetchArgsEnd();

#ifdef notdef
/*
 * Copy of the boot parameter structure.
 */
extern 	MachStringTable	machMonBootParam;
#endif

/*
 * Internal functions.
 */
extern Boolean		MachUserReturn();
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
extern unsigned		*MachEmulateBranch();

#endif /* _MACHINT */
