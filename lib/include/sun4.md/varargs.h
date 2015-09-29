/*
 * varargs.h --
 *
 *	Macros for handling variable-length argument lists.
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
 * $Header: /sprite/src/lib/include/sun4.md/RCS/varargs.h,v 1.4 91/02/01 16:24:28 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _VARARGS
#define _VARARGS

#ifndef _VA_LIST
#define _VA_LIST
typedef char *va_list;
#endif

/*
 * An argument of list of __builtin_va_alist causes the sun4 compiler
 * to store all the input registers, %i0 to %i5, in the stack frame
 * so the var_arg() macro will be able to reference them in memory.
 */
#define va_alist __builtin_va_alist

#define va_dcl int __builtin_va_alist;

#define va_start(AP) (__builtin_saveregs(), (AP) = (char *)&__builtin_va_alist)

#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_arg(AP, TYPE)						\
 ((AP) += __va_rounded_size (TYPE),					\
  *((TYPE *) ((AP) - __va_rounded_size (TYPE))))


/*  #define va_arg(list, type)  ((type *)(list += sizeof(type)))[-1]  */

#define va_end(list)

#endif /* _VARARGS */
