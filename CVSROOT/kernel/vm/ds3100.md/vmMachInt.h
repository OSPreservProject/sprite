/*
 * vmMachInt.h
 *
 *     	Internal machine dependent virtual memory data structures and
 *	procedure headers.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMMACHINT
#define _VMMACHINT

/*
 * Assembly language routines in vmPmaxAsm.s.
 */
extern	int	VmMachDoCopy();
extern	int	VmMachCopyEnd();
extern	int	VmMachWriteTLB();
extern	void	VmMachSetPID();

#endif /* _VMMACHINT */
