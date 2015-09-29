/* 
 * fsMach.c --
 *
 *	Machine-dependent file system code.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/fs/ds3100.md/RCS/fsMach.c,v 1.1 92/07/14 17:44:31 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <user/fmt.h>
#include <fsMach.h>

/*
 * The byte ordering/alignment type used with Fmt_Convert and I/O control data
 */
Fmt_Format fsMach_Format = FMT_MIPS_FORMAT;
