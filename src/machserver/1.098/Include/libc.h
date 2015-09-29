/*
 * libc.h --
 *
 *	Declarations for libc functions that don't already have a 
 *	header file assigned to them.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/libc.h,v 1.2 91/06/03 21:40:09 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _LIBC
#define _LIBC

#include <cfuncproto.h>

_EXTERN int Misc_InvokeEditor _ARGS_((char *file));
_EXTERN int alarm _ARGS_ ((unsigned seconds));
_EXTERN int gethostname _ARGS_ ((char *name, int namelen));
_EXTERN int sethostname _ARGS_ ((char *name, int namelen));
_EXTERN int sleep _ARGS_ ((unsigned seconds));

#endif /* _LIBC */
