/*
 * vmMachStat.h --
 *
 *	The statistics structure for the Sun vm module.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMMACHSTAT
#define _VMMACHSTAT

/*
 * Statistics about Sun virtual memory.
 */

typedef struct {
    int	refBitFaults;	/* Number of reference bit faults. */
    int	dirtyBitFaults;	/* Number of dirty bit faults. */
} VmMachDepStat;

#endif _VMMACHSTAT
