/*
 * fsioStreamInt.h --
 *
 *	Declarations of internal stuff about streams.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSIOSTREAMINT
#define _FSIOSTREAMINT

/* constants */

/* data structures */

/* procedures */
extern ReturnStatus FsioSetupStreamReopen _ARGS_((Fs_HandleHeader *hdrPtr,
	Address paramsPtr));
extern void FsioFinishStreamReopen _ARGS_((Fs_HandleHeader *hdrPtr,
	ReturnStatus status));

#endif /* _FSIOSTREAMINT */
