/* 
 * abort.c --
 *
 *	Source code for the "abort" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/abort.c,v 1.4 90/09/10 17:08:38 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <stdlib.h>
#include <proc.h>
#include <sig.h>

/*
 *----------------------------------------------------------------------
 *
 * abort --
 *
 *	Cause abnormal termination of the process.  For now, this
 *	puts the process into the debugger.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process drops into the debugger.
 *
 *----------------------------------------------------------------------
 */

void
abort()
{
    extern void _cleanup();

    _cleanup();
    Sig_SetHoldMask(0, 0);
    Sig_Send(SIG_ILL_INST, PROC_MY_PID, FALSE);
    _exit(1);				/* Never return to caller, even
					 * if the debugger lets us continue. */
}
