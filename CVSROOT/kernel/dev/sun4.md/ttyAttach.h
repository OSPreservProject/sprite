/*
 * ttyAttach.h --
 *
 *	Declarations for things exported by devTtyAttach.c to the rest
 *	of the device module.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVTTYATTACH
#define _DEVTTYATTACH

#ifndef _DEVTTY
#include "tty.h"
#endif
#ifndef _DEVZ8530
#include "z8530.h"
#endif

extern DevZ8530	*	DevGrabKeyboard();
extern void		DevReleaseKeyboard();
extern DevTty *		DevTtyAttach();
extern void		DevTtyInit();

#endif /* _DEVTTYATTACH */
