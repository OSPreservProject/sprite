/*
 * vmMachStat.h --
 *
 *	The statistics structure for the Sun vm module.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header: /sprite/src/kernel/vm/ds3100.md/RCS/vmMachStat.h,v 1.2 89/08/15 19:26:21 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _VMMACHSTAT
#define _VMMACHSTAT

/*
 * Statistics about Sun virtual memory.
 */

typedef struct {
    int	stealTLB;	/* Steal a tlb entry from another process. */
    int stealPID;	/* Steal a PID from a another process. */
} VmMachDepStat;

#endif /* _VMMACHSTAT */
