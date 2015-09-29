/* 
 * Sig_Pause.c --
 *
 *	Source code for the Sig_Pause library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/syscall/RCS/Sig_Pause.c,v 1.2 88/06/21 11:14:50 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <sig.h>


/*
 *----------------------------------------------------------------------
 *
 * Sig_Pause --
 *
 *      The "normal" Sig_Pause routine for user code.   This retries
 *	the Sig_Pause in the event the return status is
 *	GEN_INTERRUPTED_BY_SIGNAL since that return code means the process
 * 	was migrated during the Sig_Pause.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	The process is put to sleep awaiting a signal.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sig_Pause(sigHoldMask)
    int	sigHoldMask;	/* The value that the mask of held signals is to be set
			   to while waiting for a signal to arrive. */
{
    ReturnStatus status;

    do {
	status = Sig_RawPause(sigHoldMask);
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return(status);
}
