/* 
 * HalfwordTest.c --
 *
 *	Routine to test a bitfield one halfword at a time.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <kernel/bf.h>


/*
 *----------------------------------------------------------------------
 *
 * HalfwordTest --
 *
 * 	Wrapper for Bf_Halfwordest.
 *
 * Results:
 *	Result from Bf_Halfwordest.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
HalfwordTest(ptr, offset, size, value)
    unsigned short	*ptr;
    int			offset;
    int			size;
    int			value;
{
    return Bf_HalfwordTest(ptr, offset, size, value);
}

