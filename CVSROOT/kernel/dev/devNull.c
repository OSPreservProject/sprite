/* 
 * devNull.c --
 *
 *	Stubs to implement /dev/null.  (For a while NullProc would do!)
 *	These routines make sure that they drop all data headed
 *	for the void, and that they never return any data from the void.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"


/*
 *----------------------------------------------------------------------
 *
 *  Dev_NullRead --
 *
 *	Return zero bytes read and SUCCESS.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_NullRead(devicePtr, readPtr, replyPtr)
    Fs_Device *devicePtr;
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    replyPtr->length = 0;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 *  Dev_NullWrite --
 *
 *	Claim that the bytes have been written and return SUCCESS.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_NullWrite(devicePtr, writePtr, replyPtr)
    Fs_Device *devicePtr;
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
    replyPtr->length = writePtr->length;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_NullIOControl --
 *
 *	This procedure handles IOControls for /dev/null and other
 *	devices.  It refuses all IOControls except for a few of
 *	the generic ones, for which it does nothing.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Dev_NullIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	        *devicePtr;
    Fs_IOCParam		*ioctlPtr;
    Fs_IOReply		*replyPtr;
{
    if ((ioctlPtr->command == IOC_GET_FLAGS)
	    || (ioctlPtr->command == IOC_SET_FLAGS)
	    || (ioctlPtr->command == IOC_SET_BITS)
	    || (ioctlPtr->command == IOC_CLEAR_BITS)) {
	return SUCCESS;
    }
    return GEN_NOT_IMPLEMENTED;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_NullSelect --
 *
 *	This procedure handles selects for /dev/null and other
 *	devices that are always ready.
 *
 * Results:
 *	The device is indicated to be readable and writable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Dev_NullSelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device	*devicePtr;	/* Ignored. */
    int	*readPtr;		/* Read bit to clear if not readable */
    int	*writePtr;		/* Write bit to clear if not readable */
    int	*exceptPtr;		/* Except bit to clear if not readable */
{
    *exceptPtr = 0;
    return(SUCCESS);
}
