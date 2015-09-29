/*
 * bstring.h --
 *
 *	Declarations of BSD library procedures for byte manipulation.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/bstring.h,v 1.6 92/05/14 16:57:25 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _BSTRING
#define _BSTRING

#include <cfuncproto.h>

/* 
 * Some of the int's below should be size_t.  Unfortunately, forcing 
 * bstring.h to include <sys/types.h> can break old code that does its own 
 * type declarations.
 */

extern int	bcmp _ARGS_((_CONST _VoidPtr sourcePtr,
			     _CONST _VoidPtr destPtr,
			     /* size_t */ int numBytes));
extern void	bcopy _ARGS_((_CONST _VoidPtr sourcePtr, _VoidPtr destPtr,
			      /* size_t */ int numBytes));
extern void	bzero _ARGS_((_VoidPtr destPtr, /* size_t */ int numBytes));
extern int	ffs _ARGS_((int i));

#endif /* _BSTRING */
