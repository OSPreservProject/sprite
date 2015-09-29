/* 
 * Fs_Write.c --
 *
 *	Source code for the Fs_Write library procedure.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/Fs_Write.c,v 1.3 92/03/12 19:22:28 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <fs.h>
#include <spriteEmuInt.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_Write --
 *
 *      The "normal" Fs_Write interface for user code.  Writes writeLength
 *      characters from buffer to the file identified by streamID.
 *      *amtWrittenPtr is updated to reflect how much data were actually
 *      written.
 *
 *      For some kinds of devices, Fs_Write causes the calling process to
 *      block until the device is ready to accept the data.  If a signal
 *      is received by the process while it is blocked, then the
 *      Fs_WriteStub call will be aborted in order to process the
 *      signal.  When (if) the signal returns, the system call will return
 *      a GEN_ABORTED_BY_SIGNAL status, and *writeCountPtr will reflect
 *      the number of bytes accepted by the device before the signal
 *      occurred.  This routine will detect this occurrence and
 *      automatically restart the system call to write the remainder of
 *      the data.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	The file offset is incremented to be after the data written.
 *	The amtWrittenPtr is updated to reflect the amount actually written.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_Write(streamID, writeLength, buffer, amtWrittenPtr)
    int		streamID;	/* The user's index into its open file list */
    int		writeLength;	/* The amount of bytes to write */
    Address	buffer;		/* The data to be written */
    int		*amtWrittenPtr;	/* The amount of bytes actually written */
{
    int amtWritten;
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;

    *amtWrittenPtr = 0;
    do {
	kernStatus = Fs_WriteStub(SpriteEmu_ServerPort(), streamID, 
				  writeLength, (vm_address_t)buffer,
				  &status, &amtWritten, &sigPending);
	if (kernStatus != KERN_SUCCESS) {
	    status = Utils_MapMachStatus(kernStatus);
	}
	if (sigPending) {
	    SpriteEmu_TakeSignals();
	}
	*amtWrittenPtr += amtWritten;
	if (status == GEN_ABORTED_BY_SIGNAL) {
	    writeLength -= amtWritten;
	    if (writeLength <= 0) {
		return(SUCCESS);
	    }
	    buffer += amtWritten;
	}
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return(status);
}
