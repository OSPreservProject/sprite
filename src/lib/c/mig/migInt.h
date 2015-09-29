/*
 * migInt.h --
 *
 *	Declarations used internally by the mig procedures.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/c/mig/RCS/migInt.h,v 2.1 90/06/22 14:58:25 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _MIGINT
#define _MIGINT

/*
 * Operations we perform on the host cache.
 */
typedef enum {
    MIG_CACHE_ADD,		/* Add entry to cache. */
    MIG_CACHE_REMOVE,		/* Remove entry from cache. */
    MIG_CACHE_REMOVE_ALL,	/* Remove all entries from cache. */
    MIG_CACHE_VERIFY,		/* Verify entry is in cache. */
} MigCacheOp;

extern int MigHostCache();	/* Routine to manage cache. */
extern int MigOpenPdev();	/* Routine to open pdev, sleeping if needed. */
extern int migGetNewHosts;	/* Whether to query server for new hosts. */

extern void (*migCallBackPtr)();/* Procedure to call if idle hosts become
				   available, or NULL. */
extern int MigSetAlarm();	/* For setting timeouts. */
extern int MigClearAlarm();	/* For removing timeouts. */
/*
 * Library routines that aren't automatically declared by include files.
 */
extern int errno;

extern int strlen();
extern char *strcpy();
extern char *strerror();

#endif _MIGINT





