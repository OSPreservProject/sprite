/* 
 * devConsole.c --
 *
 *	This module provides special mechanisms to make a Sun
 *	console keyboard+display usable as a Sprite terminal.
 *	For example, it maps keystroke events to ASCII characters
 *	and draws output characters on the screen.
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
#include "z8530.h"
#include "machMon.h"
/*
 * Two tables are used for mapping keystrokes to ASCII characters.  One
 * identifies the ASCII character associated with an unshifted key, and
 * the other identifies the ASCII character associated with a shifted
 * key.  A few special values are used for keys like SHIFT and CONTROL.
 * The function keys are ignored here (KDB_NO_KEY);  since this keyboard
 * code is only used between when the machine is booted and when the
 * window system is started, the function keys shouldn't be needed.
 */

#define KBD_NO_KEY	200
#define KBD_SHIFT	201
#define KBD_CONTROL	202
#define KBD_CAPS_LOCK	203
#define KBD_CONSOLE_CMD	204
#define KBD_ALL_KEYS_UP	205

static unsigned char unshifted[] = {
/*   0 */	KBD_NO_KEY,	KBD_CONSOLE_CMD,KBD_NO_KEY,  	KBD_NO_KEY,
/*   4 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*   8 */	KBD_NO_KEY, 	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  12 */ 	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  16 */	KBD_NO_KEY, 	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  20 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  24 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  28 */	KBD_NO_KEY,	'\033',		'1',		'2',
/*  32 */	'3',		'4',		'5',		'6', 
/*  36 */	'7',		'8',		'9',		'0',
/*  40 */	'-',		'=',		'`',		'\b',
/*  44 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  48 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  52 */	KBD_NO_KEY,	'\t',		'q',		'w',
/*  56 */	'e',		'r',		't',		'y', 
/*  60 */	'u',		'i',		'o',		'p',
/*  64 */	'[',		']',		'\177',		KBD_NO_KEY, 
/*  68 */	KBD_NO_KEY,	KBD_NO_KEY, 	KBD_NO_KEY,	KBD_NO_KEY,
/*  72 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  76 */	KBD_CONTROL,	'a', 		's',		'd',
/*  80 */	'f',		'g',		'h',		'j', 
/*  84 */	'k',		'l',		';',		'\'',
/*  88 */	'\\',		'\r',		KBD_NO_KEY,	KBD_NO_KEY, 
/*  92 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  96 */	KBD_NO_KEY,	KBD_CAPS_LOCK,	KBD_NO_KEY,	KBD_SHIFT,
/* 100 */	'z',		'x',		'c',		'v',
/* 104 */	'b',		'n',		'm',		',', 
/* 108 */	'.',		'/',		KBD_SHIFT,	'\n',
/* 112 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/* 116 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_CAPS_LOCK,
/* 120 */	KBD_NO_KEY, 	' ',		KBD_NO_KEY, 	KBD_NO_KEY, 
/* 124 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_ALL_KEYS_UP
};

static unsigned char shifted[] = {
/*   0 */	KBD_NO_KEY,	KBD_CONSOLE_CMD,KBD_NO_KEY,  	KBD_NO_KEY,
/*   4 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*   8 */	KBD_NO_KEY, 	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  12 */ 	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  16 */	KBD_NO_KEY, 	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  20 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  24 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  28 */	KBD_NO_KEY,	'\033',		'!',		'@',
/*  32 */	'#',		'$',		'%',		'^', 
/*  36 */	'&',		'*',		'(',		')',
/*  40 */	'_',		'+',		'~',		'\b',
/*  44 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  48 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  52 */	KBD_NO_KEY,	'\t',		'Q',		'W',
/*  56 */	'E',		'R',		'T',		'Y', 
/*  60 */	'U',		'I',		'O',		'P',
/*  64 */	'{',		'}',		'\377',		KBD_NO_KEY, 
/*  68 */	KBD_NO_KEY,	KBD_NO_KEY, 	KBD_NO_KEY,	KBD_NO_KEY,
/*  72 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/*  76 */	KBD_CONTROL, 	'A', 		'S',		'D',
/*  80 */	'F',		'G',		'H',		'J', 
/*  84 */	'K',		'L',		':',		'"',
/*  88 */	'|',		'\r',		KBD_NO_KEY,	KBD_NO_KEY, 
/*  92 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,
/*  96 */	KBD_NO_KEY,	KBD_CAPS_LOCK,	KBD_NO_KEY,	KBD_SHIFT,
/* 100 */	'Z',		'X',		'C',		'V',
/* 104 */	'B',		'N',		'M',		'<', 
/* 108 */	'>',		'?',		KBD_SHIFT,	'\n',
/* 112 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY, 
/* 116 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_CAPS_LOCK,
/* 120 */	KBD_NO_KEY, 	' ',		KBD_NO_KEY, 	KBD_NO_KEY, 
/* 124 */	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY,	KBD_NO_KEY
};

/*
 * Last known state of the keyboard:
 */

static Boolean	controlDown	= FALSE;
static Boolean	shiftDown	= FALSE;
static Boolean	capsLock	= FALSE;
static Boolean	consoleCmdDown	= FALSE;

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
DevConsoleRawProc(ptr, operation, inBufSize, inBuffer, outBufSize, outBuffer)
    Address ptr;
    int operation;		/* What to do:  TD_RAW_OUTPUT_READY etc. */
    int inBufSize;		/* Size of input buffer for operation. */
    char *inBuffer;		/* Input buffer. */
    int outBufSize;		/* Size of output buffer for operation. */
    char *outBuffer;		/* Output buffer. */
{
    register DevZ8530 *zPtr;
    char buf[TTY_OUT_BUF_SIZE];
    int c, i;

    zPtr = (DevZ8530 *) ptr; /* Information about keyboard device. */
    if (operation != TD_RAW_OUTPUT_READY) {
	return 0;
    }
    for (i = 0; i < sizeof buf; ++i) {

	/*
	 * Note:  must call DevTtyOutputChar directly, rather than calling
	 * indirectly through zPtr->outputProc:  zPtr->outputProc must point
	 * to a dummy procedure so the Z8530 interrupt handler won't grab
	 * characters and output them to the keyboard.
	 */

	c = DevTtyOutputChar(zPtr->ttyPtr);
	if (c == -1) {
	    break;
	}
	buf[i] = c & 0x7f;
    }
    if (i > 0) {
	(*romVectorPtr->fbWriteStr)(buf, i);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DevConsoleConvertKeystroke --
 *
 *	Given a keystroke consisting of key number and up/down indication,
 *	convert it to the ASCII character that was typed.
 *
 * Results:
 *	The return value is the ASCII character corresponding to the
 *	keystroke, or -1 if the keystroke didn't correspond to an
 *	ASCII character (e.g. it was an upstroke, or a downstroke on
 *	the SHIFT key), or -2 if the keystroke formed part of an L1-
 *	console command, which means higher-level software should
 *	ignore it.
 *
 * Side effects:
 *	Internal state about the keyboard (such as whether a shift key is
 *	down) is updated.  L1- strokes are turned into console commands
 *	and executed.
 *
 *----------------------------------------------------------------------
 */

int
DevConsoleConvertKeystroke(value)
    int value;			/* Character that arrived from UART:
				 * identifies which key, and whether up
				 * or down transition. */
{
    Boolean down;
    int keyIndex;
    int asciiChar;

    /*
     * Figure out what key number changed, and whether it went up
     * or down.
     */

    keyIndex = value & 0x7f;
    if (value & 0x80) {
	down = FALSE;
    } else {
	down = TRUE;
    }

    /*
     * See if the state of the shift, control, or caps-lock keys changed.
     */

    asciiChar = unshifted[keyIndex];
    switch (asciiChar) {
	case KBD_CONTROL:
	    controlDown = down;
	    break;
	case KBD_SHIFT:
	    shiftDown = down;
	    break;
	case KBD_CAPS_LOCK:
	    if (down) {
		capsLock = !capsLock;
	    }
	    break;
	case KBD_CONSOLE_CMD:
	    consoleCmdDown = down;
	    return -2;
	    break;
	case KBD_ALL_KEYS_UP:
	    controlDown = shiftDown = capsLock = consoleCmdDown = FALSE;
	    break;
    }

    /*
     * From here on, up-transitions are not important, nor are transitions
     * on keys other than ASCII.
     */

    if (!down || (asciiChar >= KBD_NO_KEY)) {
	return -1;
    }

    if (shiftDown || capsLock) {
	asciiChar = shifted[keyIndex];
    }
    if (controlDown) {
	asciiChar &= 0x1f;
    }

    if (consoleCmdDown) {
	consoleCmdDown = 0;
	Dev_InvokeConsoleCmd(asciiChar);
	return -2;
    }
    return asciiChar;
}

/*
 *----------------------------------------------------------------------
 *
 * DevConsoleInputProc --
 *
 *	This procedure is invoked at interrupt level to handle input
 *	characters from the keyboard UART.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The input character (which identifies a key transition) is
 *	converted to an ASCII character, which is then added to the
 *	input buffer for the console terminal device.  Or, if this
 *	is a console command then the console command is executed
 *	and the keystroke is ignored.
 *
 *----------------------------------------------------------------------
 */

void
DevConsoleInputProc(ttyPtr, value)
    DevTty *ttyPtr;		/* Higher-level information about the
				 * terminal. */
    int value;			/* Character that arrived from UART:
				 * identifies which key, and whether up
				 * or down transition. */
{
    int asciiChar;

    /*
     * Ignore special characters such as break, then convert a keystroke
     * into an ASCII character.
     */

    if (value & ~0xff) {
	return;
    }
    asciiChar = DevConsoleConvertKeystroke(value);

    /*
     * If the normal (ASCII stream) handler is in place for the console,
     * then input the ASCII character.  Otherwise the console is being
     * used in "mouse" mode:  input the raw keystroke.
     */

    if (asciiChar != -2) {
	if (ttyPtr->inputProc != (void (*)()) NIL) {
	    DevTtyInputChar(ttyPtr, value);
	} else if (asciiChar >= 0) {
	    DevTtyInputChar(ttyPtr, asciiChar);
	}
    }
}
