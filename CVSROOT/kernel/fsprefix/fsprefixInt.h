/*
 * prefixInt.h --
 *
 *	Declarations of data structures and variables private to the 
 *	fsprefix module.
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

#ifndef _PREFIX
#define _PREFIX

/* constants */

/* data structures */

/*
 * A list of hosts that can use a domain is kept for exported prefixes.
 */
typedef struct FsprefixExport {
    List_Links	links;
    int		spriteID;
} FsprefixExport;

/* procedures */

extern void FsprefixDone();
extern void FsprefixHandleCloseInt();
extern void FsprefixIterate();

#endif /* _PREFIX */

