/*
 * devCounter.h --
 *
 *	Declarations of DevCounter typedef - the type of the value read
 *	from the SPUR's freerunning counter.
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

#ifndef _DEVCOUNTER
#define _DEVCOUNTER

/* data structures */
/*
 * The free running counter on the SPUR is T0, a 64 bit integer.  DevCounter
 * is the lower 32-bits (low) and the upper 32-bits (high) of the counter.
 * Any changes to this structure will require changes to devTimerT0.s
 */
 
typedef struct Spur64bitCounterStruct {
	unsigned	int	low;
	unsigned	int	high;
} DevCounter;


#endif _DEVCOUNTER


