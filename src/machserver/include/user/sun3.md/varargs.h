/*
 * varargs.h --
 *
 *	Macros for handling variable-length argument lists for the sun3.
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
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.5 90/01/12 12:03:25 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _VARARGS_H
#define _VARARGS_H

#ifndef _VA_LIST
#define _VA_LIST
typedef char *va_list;
#endif

# define va_dcl int va_alist;
# define va_start(list) list = (char *) &va_alist
# define va_end(list)
# define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
#endif /* _VARARGS_H */
