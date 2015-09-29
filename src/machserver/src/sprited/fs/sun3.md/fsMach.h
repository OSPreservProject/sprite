/*
 * fsMach.h --
 *
 *	Machine-dependent file system declarations.
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
 * $Header: /user5/kupfer/spriteserver/src/sprited/fs/sun3.md/RCS/fsMach.h,v 1.1 92/07/14 17:45:18 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _FSMACH
#define _FSMACH

#include <user/fmt.h>

/*
 * fsMach_Format defines a byte ordering/structure alignment type
 * used when servicing IOControls.  The input and output buffers for
 * IOControls have to be made right by the server.
 */
extern	Fmt_Format	fsMach_Format;

#endif /* _FSMACH */
