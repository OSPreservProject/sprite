/*
 * lfsDescMapInt.h --
 *
 *	Declarations of LFS descriptor map routines and data structures
 *	private to the Lfs module.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _LFSDESCMAPINT
#define _LFSDESCMAPINT

#include "lfsDescMap.h"

/* constants */

/* data structures */

typedef struct LfsDescMap {
    LfsStableMem	stableMem;/* Stable memory supporting the map. */
    LfsDescMapParams	params;	  /* Map parameters taken from super block. */
    LfsDescMapCheckPoint checkPoint; /* Desc map data written at checkpoint. */
} LfsDescMap;

/* procedures */

#endif /* _LFSDESCMAPINT */

