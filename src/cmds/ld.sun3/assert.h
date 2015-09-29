/*
 * assert.h --
 *
 *  Definition of assert() macro.
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
 * $Header: /sprite/src/lib/include/RCS/assert.h,v 1.5 89/08/18 11:45:04 rab Exp $
 */

#ifdef assert
#undef assert
#endif

#ifdef _assert
#undef _assert
#endif

#ifndef NDEBUG
#ifdef KERNEL
#ifdef __STDC__

#define _assert(ex) { if (!(ex)) { panic(\
    "Assertion failed: (" #ex ") file \"%s\", line %d\n", __FILE__, __LINE__);}}

#else /* __STDC__ */

#define _assert(ex) { if (!(ex)) { panic(\
    "Assertion failed: file \"%s\", line %d\n", __FILE__, __LINE__);}}

#endif /* __STDC__ */


#else /* KERNEL */
#ifdef __STDC__

extern void __eprintf(const char *string, int line, const char *filename);

void
foobar()
{
    return;
}

#define _assert(ex) { if (!(ex)) { __eprintf( \
    "Assertion failed: (" #ex ") line %d of \"%s\"\n", __LINE__, __FILE__);\
    foobar();}}

#else /* __STDC__ */

extern void __eprintf();

#define _assert(ex) { if (!(ex)) { __eprintf( \
    "Assertion failed: line %d of \"%s\"\n", __LINE__, __FILE__);\
    abort();}}

#endif /* __STDC__ */

#endif /* KERNEL */

# define assert(ex)	_assert(ex)
# else  /* !NDEBUG */
# define _assert(ex)
# define assert(ex)
# endif /* !NDEBUG */

