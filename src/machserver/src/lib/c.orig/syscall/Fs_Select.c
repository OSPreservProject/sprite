/* 
 * Fs_Select.c --
 *
 *	Source code for the Fs_Select library procedure.
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
static char rcsid[] = "$Header: Fs_Select.c,v 1.2 88/06/21 11:17:08 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include <status.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_Select --
 *
 *      The "normal" Fs_Select interface for user code.  Waits a period of
 *      time  indicated  by  *timeoutPtr for  I/O  to become possible on
 *      any of the streams indicated by *readMaskPtr,  *writeMaskPtr and
 *      *exceptMaskPtr.
 *
 * Results:
 *	The result from Fs_RawSelect.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_Select(numStreams, timeoutPtr, readMaskPtr, writeMaskPtr,
	       exceptMaskPtr, numReadyPtr)

    int		numStreams;	/* # of bits in the read and write masks. */
    Time	*timeoutPtr;	/* Timeout period or NULL. */
    int		*readMaskPtr;	/* A bitmask indicating stream ID's to check
				 * for readability. */
    int		*writeMaskPtr;	/* A bitmask indicating stream ID's to check
				 * for writability. */
    int		*exceptMaskPtr;	/* A bitmask indicating stream ID's to check
				 * for exception conditions. */
    int		*numReadyPtr;	/* On return indicates the number of streams
				 * ready for I/O. */
{
    ReturnStatus	status;

    do {
	status = Fs_RawSelect(numStreams, timeoutPtr, readMaskPtr, writeMaskPtr,
		       exceptMaskPtr, numReadyPtr);
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return(status);
}
