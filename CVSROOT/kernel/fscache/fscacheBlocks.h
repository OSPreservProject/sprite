/*
 * fsBlockCache.h --
 *
 *	Declarations for the file systems block cache.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef _FSBLOCKCACHE
#define _FSBLOCKCACHE

#include "fscache.h"
#include "fsio.h"
#include "fslcl.h"
#include "fsrmt.h"

/*
 * Minimum number of cache blocks required.  The theoretical limit
 * is about 3, enough for indirect blocks and data blocks, but
 * that is a bit extreme.  The maximum number of cache blocks is
 * a function of the physical memory size and is computed at boot time.
 */
#define FSCACHE_MIN_BLOCKS	32

/*
 * Macros to get from the dirtyLinks of a cache block to the cache block itself.
 */
#define DIRTY_LINKS_TO_BLOCK(ptr) \
		((Fscache_Block *) ((int) (ptr) - sizeof(List_Links)))

#define FILE_LINKS_TO_BLOCK(ptr) \
		((Fscache_Block *) ((int) (ptr) - 2 * sizeof(List_Links)))

/*
 * routines.
 */
extern	void		FscacheBlocksUnneeded();
extern  void            Fs_BlockCleaner();

#endif _FSBLOCKCACHE
