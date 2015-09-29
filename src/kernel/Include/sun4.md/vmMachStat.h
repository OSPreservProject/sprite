/*
 * vmMachStat.h --
 *
 *	The statistics structure for the Sun vm module.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/kernel/Cvsroot/kernel/vm/sun4.md/vmMachStat.h,v 9.0 89/09/12 15:23:57 douglis Stable $ SPRITE (Berkeley)
 */

#ifndef _VMMACHSTAT
#define _VMMACHSTAT

/*
 * Statistics about Sun virtual memory.
 */

typedef struct {
    int	stealContext;		/* The number of times that have to take
				   a context away from a process. */
    int	stealPmeg;		/* The number of times that have to take a
				   pmeg away from a process. */
} VmMachDepStat;

#endif /* _VMMACHSTAT */
