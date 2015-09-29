/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

#ifndef _MMAN
#define _MMAN

/*
 * Protection flags.
 */
#define PROT_READ	0x4	/* Read permissions. */
#define PROT_WRITE	0x2	/* Write permissions. */
#define PROT_EXEC	0x1	/* Exec permissions. */
/*
 * For no good reason, Sun and Sequent have the flags in the reverse order.
 */
#define SUN_PROT_READ	0x1	/* Read permissions. */
#define SUN_PROT_WRITE	0x2	/* Write permissions. */
#define SUN_PROT_EXEC	0x4	/* Exec permissions. */

#define PROT_RDWR	(PROT_READ|PROT_WRITE)
#define PROT_BITS	(PROT_READ|PROT_WRITE|PROT_EXEC)

/*
 * Sharing flags.
 */
#define MAP_SHARED	1	/* Share modifications. */
#define MAP_PRIVATE	2	/* Keep modifications private. */
#define MAP_ZEROFILL	3	/* Zerofill pages. */
#define MAP_TYPE	0xf	/* Mask for type. */

#define MAP_FIXED	0x10	/* Force mapping to user's address. */

#define _MAP_NEW	0x80000000 /* Return address instead of 0. */

#endif _MMAN
