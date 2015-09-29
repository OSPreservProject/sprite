/*
 * spriteString.h --
 *
 *	Declarations for Sprite string-handling routines.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/spriteString.h,v 1.1 91/10/03 22:55:15 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SPRITESTRING
#define _SPRITESTRING

#include <cfuncproto.h>

extern char **String_SaveArray _ARGS_ ((char **origPtr));
extern char **String_FreeArray _ARGS_ ((char **stringsPtr));

#endif /* _SPRITESTRING */
