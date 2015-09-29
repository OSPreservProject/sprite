/*
 * stdarg.h --
 *
 *	Macros for handling variable-length argument lists for the sun3.
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
 * $Header$
 */

#ifndef _STDARG_H
#define _STDARG_H

#ifndef _VA_LIST
#define _VA_LIST
typedef char *va_list;
#endif

#define va_start(list,lastarg) (list = ((char *) &lastarg + 4))
# define va_end(list)
# define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
#endif /* _STDARG_H */
