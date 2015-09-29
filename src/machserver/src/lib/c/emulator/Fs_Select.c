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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/Fs_Select.c,v 1.3 92/03/12 19:22:27 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include <status.h>
#include <spriteEmuInt.h>


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
 *	The Sprite status code from Fs_SelectStub.
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
    kern_return_t	kernStatus;
    boolean_t		useTimeout = TRUE;
    Time		dummyTime; /* used to keep MIG happy */
    Boolean		sigPending;

    if (timeoutPtr == NULL) {
	useTimeout = FALSE;
	timeoutPtr = &dummyTime;
    }

    do {
	kernStatus = Fs_SelectStub(SpriteEmu_ServerPort(), numStreams,
				   useTimeout, timeoutPtr,
				   (vm_address_t)readMaskPtr,
				   (vm_address_t)writeMaskPtr,
				   (vm_address_t)exceptMaskPtr,
				   &status, numReadyPtr, &sigPending);
	if (kernStatus != KERN_SUCCESS) {
	    status = Utils_MapMachStatus(kernStatus);
	}
	if (sigPending) {
	    SpriteEmu_TakeSignals();
	}
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return(status);
}
