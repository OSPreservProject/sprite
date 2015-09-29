/*
 * memory.h --
 *
 *	Declarations of memory routines.  This file exists only for
 *      backwards compatibility.  New programs should include string.h
 *      to get these definitions.
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
 * $Header: /sprite/src/lib/include/RCS/memory.h,v 1.1 90/10/26 01:05:30 rab Exp $
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include <cfuncproto.h>

extern _VoidPtr	memchr _ARGS_((_CONST char *s, int c, int n));
extern int	memcmp _ARGS_((_CONST char *s1, _CONST char *s2, int n));
extern _VoidPtr	memcpy _ARGS_((char *t, _CONST char *f, int n));
extern _VoidPtr	memmove _ARGS_((char *t, _CONST char *f, int n));
extern _VoidPtr	memset _ARGS_((char *s, int c, int n));

#endif /* _MEMORY_H */

