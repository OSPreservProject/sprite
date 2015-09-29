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
 * $Header: /sprite/src/lib/include/sun4.md/RCS/varargs.h,v 1.2 89/02/24 22:01:03 mgbaker Exp Locker: rab $ SPRITE (Berkeley)
 */

#ifndef _VARARGS
#define _VARARGS

typedef struct {
    char *vl_current;			/* Pointer to last arg returned from
					 * list. */
    char *vl_next;			/* Pointer to next arg to return. */
} va_list;

/*
 * An argument of list of __builtin_va_alist causes the sun4 compiler
 * to store all the input registers, %i0 to %i5, in the stack frame
 * so the var_arg() macro will be able to reference them in memory.
 */
#define va_alist __builtin_va_alist

#define va_dcl int __builtin_va_alist;

#define va_start(list) \
    (list).vl_current = (list).vl_next = (char *) &__builtin_va_alist;

#define va_arg(list, type)			\
    ((list).vl_current = (list).vl_next,	\
    (list).vl_next += sizeof(type),		\
     *((type *) (list).vl_current))

#define va_end(list)

#endif _VARARGS
