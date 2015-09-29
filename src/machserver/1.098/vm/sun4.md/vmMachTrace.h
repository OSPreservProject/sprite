/*
 * vmMachTrace.h --
 *
 *     Virtual memory tracing structures.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/kernel/vm/sun3.md/RCS/vmMachTrace.h,v 9.0 89/09/12 15:24:00 douglis Stable $ SPRITE (Berkeley)
 */

#ifndef _VMMACHTRACE
#define _VMMACHTRACE

/*
 * Trace stats.
 */
typedef struct {
    int		pmegsChecked;
} VmMach_TraceStats;

#endif /* _VMMACHTRACE */
