/*
 * util.h --
 *
 *	Declarations of routines in the util lib.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: util.h,v 1.1 88/10/03 13:15:24 mlgray Exp $ SPRITE (Berkeley)
 */

#ifndef _UTIL
#define _UTIL

/* procedures */
extern	char	*Util_CanonicalDir();
extern	int	Util_StringToColor();
extern	char	*Util_Strcpy();
extern	char	*Util_Strncpy();

extern	void	free_scandir();

#endif /* _UTIL */
