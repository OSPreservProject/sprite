/* 
 * Fs_Read.c --
 *
 *	Source code for the Fs_Read library procedure.
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
static char rcsid[] = "$Header: Fs_Read.c,v 1.2 88/06/21 11:14:50 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_Read --
 *
 *      The "normal" Fs_Read routine for user code.  Read from the file
 *      indicated by the stream id into the buffer.  bufSize indicates how
 *      much data to read, and amountReadPtr is an output parameter that
 *      indicates how much data were read.  A length of zero means
 *      end-of-file.
 *
 *      If Fs_RawRead blocks to wait for data on the stream and a signal
 *      is received by the process during the wait, then the system call
 *      is aborted in order to process the signal.  When the signal has
 *      been processed, the system call will return with a
 *      GEN_ABORTED_BY_SIGNAL result, and *readCountPtr will have been
 *      updated to reflect the number of bytes read so far.  This routine
 *      will detect this condition and restart the system call to read the
 *      remainder of the data.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	The read access pointer for the stream is advanced by
 *	the amount of data read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_Read(streamID, amountRead, buffer, amountReadPtr)
    int		streamID;	/* The user's index into its open file list */
    int		amountRead;	/* The amount of bytes to read */
    Address	buffer;		/* The storage place for the read */
    int		*amountReadPtr;	/* The amount of bytes actually read */
{
    int	realAmtRead;
    ReturnStatus status;

    *amountReadPtr = 0;
    do {
	status = Fs_RawRead(streamID, amountRead, buffer, &realAmtRead);
	*amountReadPtr += realAmtRead;
	if (status == GEN_ABORTED_BY_SIGNAL) {
	    amountRead -= realAmtRead;
	    if (amountRead <= 0) {
		return(SUCCESS);
	    }
	    buffer += realAmtRead;
	}
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return(status);
}
