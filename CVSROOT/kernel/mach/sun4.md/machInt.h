/*
 * machInt.h --
 *
 *	This file defines things that are shared between the "mach" modules
 *	but aren't used by the rest of Sprite.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHINT
#define _MACHINT

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern int MachFetchArgs();
extern int MachFetchArgsEnd();
extern Address Mach_ProbeStart;
extern Address Mach_ProbeEnd;

#endif /* _MACHINT */
