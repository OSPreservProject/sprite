/*
 * lfsFileLayoutInt.h --
 *
 *	Declarations of data structures describing the layout of 
 *	files and descriptors in LFS segments.
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

#ifndef _LFSFILELAYOUTINT
#define _LFSFILELAYOUTINT

#include <lfsFileLayout.h>

typedef struct LfsFileLayout {
    LfsFileLayoutParams	 params;	/* File layout description. */
} LfsFileLayout;

extern void LfsFileLayoutInit _ARGS_((void));
extern Boolean LfsFileMatch _ARGS_((Fscache_FileInfo *cacheInfoPtr,
			ClientData clientData));


#endif /* _LFSFILELAYOUTINT */
