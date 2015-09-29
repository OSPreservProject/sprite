/* 
 * Fs_IOControl.c --
 *
 *	Source code for the Fs_IOControl library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/syscall/RCS/Fs_IOControl.c,v 1.2 89/09/27 16:52:47 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_IOControl --
 *
 *      The "normal" Fs_IOControl interface for user code.
 *	Retries some operations when GEN_ABORTED_BY_SIGNAL is
 *	returned from the kernel.  The list of operations is not
 *	yet inclusive.  
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_IOControl(streamID, command, inBufSize, inBuffer, outBufSize, outBuffer)
    int 	streamID;	/* User's handle on the stream */
    int 	command;	/* IOControl command */
    int 	inBufSize;	/* Size of inBuffer */
    Address 	inBuffer;	/* Command specific input parameters */
    int 	outBufSize;	/* Size of outBuffer */
    Address 	outBuffer;	/* Command specific output parameters */
{
    ReturnStatus status;
    do {
	status = Fs_RawIOControl(streamID, command, inBufSize, inBuffer,
				 outBufSize, outBuffer);
	if (status == GEN_ABORTED_BY_SIGNAL) {
	    switch(command) {
#ifdef notdef
/*
 * IOC_LOCK can block forever unexpectedly if a signal is suppressed.
 */
		case IOC_LOCK:
#endif
		case IOC_UNLOCK:
		case IOC_NUM_READABLE:
		case IOC_TRUNCATE:
		case IOC_GET_OWNER:
		case IOC_SET_OWNER:
		case IOC_MAP:
		case IOC_REPOSITION:
		case IOC_SET_FLAGS: 
		case IOC_CLEAR_BITS:
		break;

		default:
		return(status);
	    }
	}
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return(status);
}
