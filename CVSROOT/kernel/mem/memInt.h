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

extern void Mem_DumpTrace _ARGS_((int blockSize));
extern void MemPanic _ARGS_((char *message));
extern int MemChunkAlloc _ARGS_((int size, Address *addressPtr));
extern void MemPrintInit _ARGS_((void));
extern Address Mem_CallerPC _ARGS_((void));

extern void	(*memPrintProc) _ARGS_(());
extern ClientData	memPrintData;
extern Boolean		memAllowFreeingFree;

#endif /* _MEMINT */
