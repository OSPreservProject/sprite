/*
 * struct.h --
 *
 *	Declarations of macros for getting infomation about the fields
 *      in a structure.
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
 * $Header: /sprite/src/lib/include/RCS/struct.h,v 1.1 91/11/05 15:44:02 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _STRUCT_H
#define _STRUCT_H

#define fldoff(str, fld)        ((int)&(((struct str *)0)->fld))
#define fldsiz(str, fld)        (sizeof(((struct str *)0)->fld))
#define strbase(str, ptr, fld) \
                            ((struct str *)((char *)(ptr)-fldoff(str, fld)))

#endif /* _STRUCT_H */

