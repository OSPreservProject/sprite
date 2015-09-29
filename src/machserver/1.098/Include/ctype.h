/*
 * ctype.h --
 *
 *	Declarations of BSD library procedures for classifying characters
 *	into a number of different types.
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
 * $Header: /sprite/src/lib/include/RCS/ctype.h,v 1.3 90/11/06 17:18:20 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _CTYPE
#define _CTYPE

#include <cfuncproto.h>

#ifndef EOF
#define EOF (-1)
#endif

_EXTERN int	isalnum _ARGS_ ((int c));
_EXTERN int	isalpha _ARGS_ ((int c));
_EXTERN int	isascii _ARGS_ ((int c));
_EXTERN int	iscntrl _ARGS_ ((int c));
_EXTERN int	isdigit _ARGS_ ((int c));
_EXTERN int	isgraph _ARGS_ ((int c));
_EXTERN int	islower _ARGS_ ((int c));
_EXTERN int	isprint _ARGS_ ((int c));
_EXTERN int	ispunct _ARGS_ ((int c));
_EXTERN int	isspace _ARGS_ ((int c));
_EXTERN int	isupper _ARGS_ ((int c));
_EXTERN int	isxdigit _ARGS_ ((int c));
_EXTERN int	tolower _ARGS_ ((int c));
_EXTERN int	toupper _ARGS_ ((int c));


#define isalnum(char) \
    ((_ctype_bits+1)[char] & (CTYPE_UPPER|CTYPE_LOWER|CTYPE_DIGIT))

#define isalpha(char) \
    ((_ctype_bits+1)[char] & (CTYPE_UPPER|CTYPE_LOWER))

#define iscntrl(char) \
    ((_ctype_bits+1)[char] & CTYPE_CONTROL)

#define isdigit(char) \
    ((_ctype_bits+1)[char] & CTYPE_DIGIT)

#define isgraph(char) \
    ((_ctype_bits+1)[char] & (CTYPE_UPPER|CTYPE_LOWER|CTYPE_DIGIT|CTYPE_PUNCT))

#define islower(char) \
    ((_ctype_bits+1)[char] & CTYPE_LOWER)

#define isprint(char) \
    ((_ctype_bits+1)[char] & CTYPE_PRINT)

#define ispunct(char) \
    ((_ctype_bits+1)[char] & CTYPE_PUNCT)

#define isspace(char) \
    ((_ctype_bits+1)[char] & CTYPE_SPACE)

#define isupper(char) \
    ((_ctype_bits+1)[char] & CTYPE_UPPER)

#define isxdigit(char) \
    ((_ctype_bits+1)[char] & (CTYPE_DIGIT|CTYPE_HEX_DIGIT))

#define isascii(i) \
    ((i & ~0x7f) == 0)

#define CTYPE_UPPER	0x01
#define CTYPE_LOWER	0x02
#define CTYPE_DIGIT	0x04
#define CTYPE_SPACE	0x08
#define CTYPE_PUNCT	0x10
#define CTYPE_PRINT	0x20
#define CTYPE_CONTROL	0x40
#define CTYPE_HEX_DIGIT	0x80

extern char _ctype_bits[];
#endif /* _CTYPE */
