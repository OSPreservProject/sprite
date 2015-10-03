/*
 * dump.h --
 *
 *	Declarations of external routines for the ``dump'' utility.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DUMP
#define _DUMP

/* 
 * For L1 routines that want to print a header line every screenful, 
 * this #define gives a common definition for how big a screen is.  A 
 * few extra lines are provided so that, e.g., syslog output is less 
 * likely to make the header scroll off the screen.
 */
#define DUMP_LINES_PER_SCREEN	20

extern void	Dump_Init _ARGS_((void));

#endif /* _DUMP */
