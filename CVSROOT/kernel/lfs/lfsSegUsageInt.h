/*
 * lfsSegUsageInt.h --
 *
 *	Declarations of LFS segment ussage routines and data structures
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

#ifndef _LFSSEGUSAGEINT
#define _LFSSEGUSAGEINT

#include "lfsUsageArray.h"

/* constants */

/* data structures */

typedef struct LfsSegUsage {
    LfsStableMem	stableMem;/* Stable memory supporting the map. */
    LfsSegUsageParams	params;	  /* Map parameters taken from super block. */
    LfsSegUsageCheckPoint checkPoint; /* Desc map data written at checkpoint. */
} LfsSegUsage;

/* procedures */

extern void LfsSetSegUsage();

extern ReturnStatus LfsSegUsageAttach();
extern Boolean	LfsSegUsageClean(), LfsSegUsageCheckpoint(), LfsSegUsageLayout();
extern void LfsSegUsageWriteDone();

#endif /* _LFSSEGUSAGEINT */

