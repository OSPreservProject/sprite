/* 
 * ttyslot.c --
 *
 *	Source code for the ttyslot library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: ttyslot.c,v 1.1 88/07/14 14:08:03 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

/*
 *----------------------------------------------------------------------
 *
 * ttyslot --
 *
 *	This is a Sprite replacement for the UNIX ttyslot library
 *	procedure, which supposedly returns the number of the entry
 *	the ttys file that corresponds to the process' control
 *	terminal.  Sprite doesn't really have the notion of a control
 *	terminal, so this procedure pretends it couldn't find an entry.
 *
 * Results:
 *	0, as if there was no corresponding slot in the ttys file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
ttyslot()
{
    return 0;		/* Pretend we couldn't find it. */
}
