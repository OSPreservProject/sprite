/*
 * hostInt.h --
 *
 *	Declarations used internally by the "Host_" procedures.
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
 * $Header: hostInt.h,v 1.1 88/06/30 11:07:12 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _HOSTINT
#define _HOSTINT

#ifndef _STDIO
#include <stdio.h>
#endif

/*
 * Information about the current database file:
 */

extern FILE *hostFile;		/* Non-zero?  Then it points to an open file. */
extern char *hostFileName;	/* Name of database file to use. */

#endif _HOSTINT
