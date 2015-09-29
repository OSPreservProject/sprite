/*
 * sched.h --
 *
 *	Instrumentation about system load, etc.  This header file contains 
 *	the remains of the native Sprite kernel/sched.h.
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
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/sched.h,v 1.1 92/04/29 22:34:20 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SCHED
#define _SCHED

#include <spriteTime.h>

#define MACH_NUM_LOAD_VALUES	3

/* 
 * System instrumentation returned by the Sys_Stats SYS_SCHED_STATS call.
 */

typedef struct {
    double avenrun[MACH_NUM_LOAD_VALUES]; /* load average from Mach */
    Time noUserInput;		/* time since last console input */
} Sched_Instrument;

#endif /* _SCHED */
