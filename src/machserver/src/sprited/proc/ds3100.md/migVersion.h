/*
 * migVersion.h --
 *
 *	Define the migration version, which is machine-dependent.
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
 * $Header: /user5/kupfer/spriteserver/src/sprited/proc/ds3100.md/RCS/migVersion.h,v 1.2 92/04/29 21:53:19 kupfer Exp $ SPRITE (Berkeley)
 */

/*
 * Set the migration version number.  Machines can only migrate to other
 * machines of the same architecture and version number.
 */
#ifndef PROC_MIGRATE_VERSION
#define PROC_MIGRATE_VERSION 1001
#endif /* PROC_MIGRATE_VERSION */

