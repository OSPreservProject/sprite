/*
 * memInt.h --
 *
 *	Internal declarations of procedures for the memory module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MEMINT
#define _MEMINT

#include "sprite.h"

extern void	MemPanic();
extern int	MemChunkAlloc();
extern void	MemAdjustHeap();
extern void	MemPrintInit();

extern void		(*memPrintProc)();
extern ClientData	memPrintData;
extern Boolean		memAllowFreeingFree;

extern int	mem_NumAllocs;
extern int	mem_NumFrees;

#endif /* _MEMINT */
