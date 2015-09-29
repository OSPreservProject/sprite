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
 * $Header: /sprite/src/lib/include/RCS/bstring.h,v 1.4 90/09/11 14:32:01 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _BSTRING
#define _BSTRING

#include <cfuncproto.h>

extern int	bcmp _ARGS_((char *sourcePtr, char *destPtr, int numBytes));
extern void	bcopy _ARGS_((char *sourcePtr, char *destPtr, int numBytes));
extern void	bzero _ARGS_((char *destPtr, int numBytes));
extern int	ffs _ARGS_((int i));

#endif /* _BSTRING */
