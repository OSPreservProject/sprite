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
 *	None.
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
 *	None.
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
