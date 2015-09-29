/*
 * stdarg.h --
 *
 *	ANSI C macros for handling variable-length argument lists.
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
 * $Header: /sprite/src/lib/include/sun4.md/RCS/stdarg.h,v 1.2 90/12/07 23:54:59 rab Exp $
 */

#ifndef _STDARG_H
#define _STDARG_H

#ifndef _VA_LIST
#define _VA_LIST
typedef char *va_list;
#endif

#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_start(AP, lastarg) \
 (__builtin_saveregs(), (AP) = ((char *)&lastarg + __va_rounded_size(lastarg)))

#define va_arg(AP, TYPE)						\
 ((AP) += __va_rounded_size (TYPE),					\
  *((TYPE *) ((AP) - __va_rounded_size (TYPE))))

#define va_end(list)

#endif /* _STDARG_H */
