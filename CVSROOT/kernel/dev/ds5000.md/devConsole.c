/* 
 * devConsole.c --
 *
 *	This module provides a routine to output characters onto the
 *	console.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "dev.h"
#include "tty.h"
#include "graphics.h"
#include "dc7085.h"

Boolean	devDivertXInput = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * DevConsoleRawProc --
 *
 *	This procedure is invoked from the Td module to handle control
 *	requests on the raw side of the console.  This procedure is
 *	special because output to the console is not transmitted on
 *	a serial line:  it gets drawn directly on the screen by calling
 *	a procedure in the boot ROM.
 *
 * Results:
 *	The return value is the number of bytes returned to the caller
 *	at outBuffer.
 *
 * Side effects:
 *	Depends on the control operation.  Most likely effect is to
 *	draw data on the screen.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
int
DevConsoleRawProc(dcPtr, operation, inBufSize, inBuffer, outBufSize, outBuffer)
    register DevDC7085 *dcPtr;	/* Information about keyboard device. */
    int operation;		/* What to do:  TD_RAW_OUTPUT_READY etc. */
    int inBufSize;		/* Size of input buffer for operation. */
    char *inBuffer;		/* Input buffer. */
    int outBufSize;		/* Size of output buffer for operation. */
    char *outBuffer;		/* Output buffer. */
{
    int c;

    if (operation != TD_RAW_OUTPUT_READY) {
	return 0;
    }
    while (TRUE) {
	/*
	 * Note:  must call DevTtyOutputChar directly, rather than calling
	 * indirectly through dcPtr->outputProc:  dcPtr->outputProc must point
	 * to a dummy procedure so the DC7850 interrupt handler won't grab
	 * characters and output them to the keyboard.
	 */
	c = DevTtyOutputChar(dcPtr->ttyPtr);
	if (c == -1) {
	    break;
	}
	Dev_GraphicsPutc(c);
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_ConsoleReset --
 *
 *	Change where keyboard input goes.  TRUE => it will go to the normal
 *	console input routines.  FALSE => means that it will go back to X
 *	if X is using it.
 *
 * Results:
 *	The return value is the number of bytes returned to the caller
 *	at outBuffer.
 *
 * Side effects:
 *	Depends on the control operation.  Most likely effect is to
 *	draw data on the screen.
 *
 *----------------------------------------------------------------------
 */
void
Dev_ConsoleReset(keyboardMode)
    Boolean	keyboardMode;
{
    devDivertXInput = keyboardMode;
}
