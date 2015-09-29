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
 * $Header: /r3/kupfer/spriteserver/include/user/RCS/bstring.h,v 1.3 91/11/11 23:07:09 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _BSTRING
#define _BSTRING

#include <cfuncproto.h>
#include <sys/types.h>

extern int	bcmp _ARGS_((_CONST _VoidPtr sourcePtr,
			     _CONST _VoidPtr destPtr,
			     size_t numBytes));
extern void	bcopy _ARGS_((_CONST _VoidPtr sourcePtr, _VoidPtr destPtr,
			      size_t numBytes));
extern void	bzero _ARGS_((_VoidPtr destPtr, size_t numBytes));
extern int	ffs _ARGS_((int i));

#endif /* _BSTRING */
