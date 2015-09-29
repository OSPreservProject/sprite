/*
 * gateInt.h --
 *
 *	Declarations used internally by the "Gate_" procedures.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/c/gate/RCS/gateInt.h,v 1.1 92/06/05 12:49:19 jhh Exp $ SPRITE (Berkeley)
 */

#ifndef _GATEINT
#define _GATEINT

#ifndef _STDIO
#include <stdio.h>
#endif

/*
 * Information about the current database file:
 */

extern FILE *gateFile;		/* Non-zero? Then it points to an open file. */
extern char *gateFileName;	/* Name of database file to use. */

#endif _HOSTINT
