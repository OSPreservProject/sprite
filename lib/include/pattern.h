/*
 * pattern.h --
 *
 *	Declarations of defines and procedures provided by the pattern-
 *	matcher library.
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
 * $Header: /sprite/src/lib/pattern/RCS/pattern.h,v 1.1 89/01/11 12:05:20 mlgray Exp $ SPRITE (Berkeley)
 */

#ifndef _PATTERN
#define _PATTERN

# define B_PAREN 257
# define E_PAREN 258
# define AND 259
# define OR 260
# define NOT 261
# define PATTERN 262

extern	int	Pattern_Match();

#endif _PATTERN
