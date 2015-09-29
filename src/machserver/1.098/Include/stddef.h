/*
 * stddef.h --
 *
 *	Declarations of standard types.
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
 * $Header: /sprite/src/lib/include/RCS/stddef.h,v 1.4 90/10/19 15:41:25 shirriff Exp $
 */

#ifndef _STDDEF
#define _STDDEF

typedef int ptrdiff_t;
typedef short wchar_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef int size_t;
#endif

#ifndef NULL
#define	NULL		0
#endif

#define offsetof(structtype, field) \
    ((size_t)(char *)&(((structtype *)0)->field))

#endif /* _STDDEF */

