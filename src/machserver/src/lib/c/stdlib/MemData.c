/* 
 * MemData.c --
 *
 *	Contains variables that are shared among the various memory
 *	allocation procedures.  They need to be in a separate file
 *	in order to avoid unpleasant interactions with some old-
 *	fashioned UNIX programs that define some or all of the malloc-
 *	related stuff for themselves.  If the variables are tied to
 *	a particular procedure, then the procedure will get linked in
 *	whenever the variables are needed, even in the program has
 *	defined its own version of that procedure.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/stdlib/RCS/MemData.c,v 1.2 91/12/12 21:56:09 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"

/*
 * Info for binned allocation.  See memInt.h for details.
 */

Address		memFreeLists[BIN_BUCKETS];
int		mem_NumBlocks[BIN_BUCKETS];
#ifdef MEM_TRACE
int		mem_NumBinnedAllocs[BIN_BUCKETS];
#endif

/*
 * Info for large-block allocator.  See memInt.h for details.
 */

Address		memFirst, memLast;
Address		memCurrentPtr;
int		memLargestFree = 0;
int		memBytesFreed = 0;
int		mem_NumLargeBytes = 0;
int		mem_NumLargeAllocs = 0;
int		mem_NumLargeLoops = 0;

int		mem_NumAllocs = 0;
int		mem_NumFrees = 0;

int		memInitialized = 0;

#if defined(LIBC_USERLOCK) || defined(SPRITED)
Sync_Lock	memMonitorLock;
#endif
