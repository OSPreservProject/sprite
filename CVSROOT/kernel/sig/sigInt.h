/*
 * sigInt.h --
 *
 *     Data structures and procedure headers exported by the
 *     the signal module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SIGINT
#define _SIGINT

#include "sprite.h"
#include "sig.h"

/*
 * Flags for the sigFlags field in the proc table.
 *
 * SIG_PAUSE_IN_PROGRESS	A Sig_Pause is currently being executed.
 *
 */

#define	SIG_PAUSE_IN_PROGRESS	0x01

/*
 * Signals that can be blocked.
 */
extern	int	sigCanHoldMask;

/*
 * Array of bit masks, one for each signal.  The bit mask for a particular
 * signal is equal to 1 << signal.  This is to allow particular bits be
 * extracted and set in the signal masks.
 */

extern	unsigned int	sigBitMasks[];

/*
 * Array of default actions for signals.
 */

extern	int	sigDefActions[];

extern	Sync_Lock	sigLock;
#define LOCKPTR &sigLock

extern	void	SigClearPendingMask();

#endif _SIGINT
