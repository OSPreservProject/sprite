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

extern void		Dump_Init();
extern void		Dump_ProcessTable();
extern void		Dump_ReadyQueue();
extern void		Dump_TimerQueue();

#endif /* _DUMP */
