/* 
 *  devDC7085.c --
 *
 *     	This file contains machine-dependent routines that handle the
 *	output queue for the serial lines.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "machMon.h"
#include "mach.h"
#include "dev.h"
#include "fs.h"
#include "sys.h"
#include "sync.h"
#include "timer.h"
#include "dbg.h"
#include "dc7085.h"
#include "graphics.h"
#include "machAddrs.h"
#include <sgtty.h>

static Sync_Semaphore dc7085Mutex = Sync_SemInitStatic("Dev:dc7085Mutex");

/*
 * Define the six registers.
 */
#define REG_ADDR(offset) (unsigned short *)(MACH_SERIAL_INTERFACE_ADDR + (offset))
static volatile unsigned short *csrPtr =  REG_ADDR(0x00);
static volatile unsigned short *rBufPtr = REG_ADDR(0x08);
static volatile unsigned short *lprPtr =  REG_ADDR(0x08);
static volatile unsigned short *tcrPtr =  REG_ADDR(0x10);
static volatile unsigned short *msrPtr =  REG_ADDR(0x18);
static volatile unsigned short *tdrPtr =  REG_ADDR(0x18);

/*
 * The current status of the break bits.
 */
unsigned breakVal = 0;

/*
 * Tables mapping sgttyb baud-rate values to actual integers and lpr register
 * values.
 */
static struct {
    int sgttybVal;			/* Baud value from sgtyb. */
    int regVal;				/* Value to set in lpr register. */
    int baud;				/* Integer baud rate. */
} baudMap[] = {
    {0, 0, 0},
    {LPR_B50, B50, 50},
    {LPR_B75, B75, 75},
    {LPR_B110, B110, 110},
    {LPR_B134, B134, 134},
    {LPR_B150, B150, 150},
    {-1, B200, 200},
    {LPR_B300, B300, 300},
    {LPR_B600, B600, 600},
    {LPR_B1200, B1200, 1200},
    {LPR_B2400, B2400, 2400},
    {LPR_B4800, B4800, 4800},
    {LPR_B9600, B9600, 9600}, 
    {-1, -1, -1}
};

/*
 *  Raw key code for a console command.
 */
#define KEY_CONSOLE_CMD		KEY_F13

/*
 * Ascii values of command keys.
 */
#define KBD_TAB		'\t'
#define KBD_DEL		127
#define KBD_RET		'\r'

/*
 *  Define "hardware-independent" codes for the control, shift, meta and 
 *  function keys.  Codes start after the last 7-bit ASCII code (127)
 *  and are assigned in an arbitrary order.
 */
#define KBD_NOKEY	128
#define KBD_UNKNOWN	129

#define KBD_F1		201
#define KBD_F2		202
#define KBD_F3		203
#define KBD_F4		204
#define KBD_F5		205
#define KBD_F6		206
#define KBD_F7		207
#define KBD_F8		208
#define KBD_F9		209
#define KBD_F10		210
#define KBD_F11		211
#define KBD_F12		212
#define KBD_F13		213
#define KBD_F14		214
#define KBD_HELP	215
#define KBD_DO		216
#define KBD_F17		217
#define KBD_F18		218
#define KBD_F19		219
#define KBD_F20		220

#define KBD_FIND	221
#define KBD_INSERT	222
#define KBD_REMOVE	223
#define KBD_SELECT	224
#define KBD_PREVIOUS	225
#define KBD_NEXT	226

#define KBD_KP_ENTER	227
#define KBD_KP_F1	228
#define KBD_KP_F2	229
#define KBD_KP_F3	230
#define KBD_KP_F4	231
#define KBD_LEFT	232
#define KBD_RIGHT	233
#define KBD_DOWN	234
#define KBD_UP		235

#define KBD_CONTROL	236
#define KBD_SHIFT	237
#define KBD_CAPSLOCK	238
#define KBD_ALTERNATE	239

#define KBD_MAX_VALUE	KBD_ALTERNATE

/*
 * Keyboard to Ascii, unshifted. 
 */
static unsigned char unshiftedAscii[] = {
/*  0 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/*  4 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/*  8 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/*  c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 10 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 14 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 18 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 1c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 20 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 24 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 28 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 2c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 30 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 34 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 38 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 3c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 40 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 44 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 48 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 4c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 50 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 54 */ KBD_NOKEY,	KBD_NOKEY,	KBD_F1,		KBD_F2,
/* 58 */ KBD_F3,	KBD_F4,		KBD_F5,		KBD_NOKEY,
/* 5c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 60 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 64 */ KBD_F6,	KBD_F7,		KBD_F8,		KBD_F9,
/* 68 */ KBD_F10,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 6c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 70 */ KBD_NOKEY,	KBD_F11,	KBD_F12,	KBD_F13,
/* 74 */ KBD_F14,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 78 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 7c */ KBD_HELP,	KBD_DO,		KBD_NOKEY,	KBD_NOKEY,
/* 80 */ KBD_F17,	KBD_F18,	KBD_F19,	KBD_F20,
/* 84 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 88 */ KBD_NOKEY,	KBD_NOKEY,	KBD_FIND,	KBD_INSERT,
/* 8c */ KBD_REMOVE,	KBD_SELECT,	KBD_PREVIOUS,	KBD_NEXT,
/* 90 */ KBD_NOKEY,	KBD_NOKEY,	'0',		KBD_NOKEY,
/* 94 */ '.',		KBD_KP_ENTER,	'1',		'2',
/* 98 */ '3',		'4',		'5',		'6',
/* 9c */ ',',		'7',		'8',		'9',
/* a0 */ '-',		KBD_KP_F1,	KBD_KP_F2,	KBD_KP_F3,
/* a4 */ KBD_KP_F4,	KBD_NOKEY,	KBD_NOKEY,	KBD_LEFT,
/* a8 */ KBD_RIGHT,	KBD_DOWN, 	KBD_UP,		KBD_NOKEY,
/* ac */ KBD_NOKEY,	KBD_NOKEY,	KBD_SHIFT,	KBD_CONTROL,
/* b0 */ KBD_CAPSLOCK,	KBD_ALTERNATE,	KBD_NOKEY,	KBD_NOKEY,
/* b4 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* b8 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* bc */ KBD_DEL,	KBD_RET,	KBD_TAB,	'`',
/* c0 */ '1',		'q',		'a',		'z',
/* c4 */ KBD_NOKEY,	'2',		'w',		's',
/* c8 */ 'x',		'<',		KBD_NOKEY,	'3',
/* cc */ 'e',		'd',		'c',		KBD_NOKEY,
/* d0 */ '4',		'r',		'f',		'v',
/* d4 */ ' ',		KBD_NOKEY,	'5',		't',
/* d8 */ 'g',		'b',		KBD_NOKEY,	'6',
/* dc */ 'y',		'h',		'n',		KBD_NOKEY,
/* e0 */ '7',		'u',		'j',		'm',
/* e4 */ KBD_NOKEY,	'8',		'i',		'k',
/* e8 */ ',',		KBD_NOKEY,	'9',		'o',
/* ec */ 'l',		'.',		KBD_NOKEY,	'0',
/* f0 */ 'p',		KBD_NOKEY,	';',		'/',
/* f4 */ KBD_NOKEY,	'=',		']',		'\\',
/* f8 */ KBD_NOKEY,	'-',		'[',		'\'',
/* fc */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
};

/*
 * Keyboard to Ascii, shifted.
 */
static unsigned char shiftedAscii[] = {
/*  0 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/*  4 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/*  8 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/*  c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 10 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 14 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 18 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 1c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 20 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 24 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 28 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 2c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 30 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 34 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 38 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 3c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 40 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 44 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 48 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 4c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 50 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 54 */ KBD_NOKEY,	KBD_NOKEY,	KBD_F1,		KBD_F2,
/* 58 */ KBD_F3,	KBD_F4,		KBD_F5,		KBD_NOKEY,
/* 5c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 60 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 64 */ KBD_F6,	KBD_F7,		KBD_F8,		KBD_F9,
/* 68 */ KBD_F10,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 6c */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 70 */ KBD_NOKEY,	KBD_F11,	KBD_F12,	KBD_F13,
/* 74 */ KBD_F14,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 78 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 7c */ KBD_HELP,	KBD_DO,		KBD_NOKEY,	KBD_NOKEY,
/* 80 */ KBD_F17,	KBD_F18,	KBD_F19,	KBD_F20,
/* 84 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* 88 */ KBD_NOKEY,	KBD_NOKEY,	KBD_FIND,	KBD_INSERT,
/* 8c */ KBD_REMOVE,	KBD_SELECT,	KBD_PREVIOUS,	KBD_NEXT,
/* 90 */ KBD_NOKEY,	KBD_NOKEY,	'0',		KBD_NOKEY,
/* 94 */ '.',		KBD_KP_ENTER,	'1',		'2',
/* 98 */ '3',		'4',		'5',		'6',
/* 9c */ ',',		'7',		'8',		'9',
/* a0 */ '-',		KBD_KP_F1,	KBD_KP_F2,	KBD_KP_F3,
/* a4 */ KBD_KP_F4,	KBD_NOKEY,	KBD_NOKEY,	KBD_LEFT,
/* a8 */ KBD_RIGHT,	KBD_DOWN, 	KBD_UP,		KBD_NOKEY,
/* ac */ KBD_NOKEY,	KBD_NOKEY,	KBD_SHIFT,	KBD_CONTROL,
/* b0 */ KBD_CAPSLOCK,	KBD_ALTERNATE,	KBD_NOKEY,	KBD_NOKEY,
/* b4 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* b8 */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
/* bc */ KBD_DEL,	KBD_RET,	KBD_TAB,	'~',
/* c0 */ '!',		'q',		'a',		'z',
/* c4 */ KBD_NOKEY,	'@',		'w',		's',
/* c8 */ 'x',		'>',		KBD_NOKEY,	'#',
/* cc */ 'e',		'd',		'c',		KBD_NOKEY,
/* d0 */ '$',		'r',		'f',		'v',
/* d4 */ ' ',		KBD_NOKEY,	'%',		't',
/* d8 */ 'g',		'b',		KBD_NOKEY,	'^',
/* dc */ 'y',		'h',		'n',		KBD_NOKEY,
/* e0 */ '&',		'u',		'j',		'm',
/* e4 */ KBD_NOKEY,	'*',		'i',		'k',
/* e8 */ ',',		KBD_NOKEY,	'(',		'o',
/* ec */ 'l',		'.',		KBD_NOKEY,	')',
/* f0 */ 'p',		KBD_NOKEY,	':',		'?',
/* f4 */ KBD_NOKEY,	'+',		'}',		'|',
/* f8 */ KBD_NOKEY,	'_',		'{',		'"',
/* fc */ KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,	KBD_NOKEY,
};


/*
 * ----------------------------------------------------------------------------
 *
 * DevDC7085Reset --
 *
 *	Reset the chip.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
void
DevDC7085Reset()
{
    *csrPtr = CSR_CLR;
    while ((*csrPtr & CSR_CLR) != 0) {
    }
    *csrPtr = CSR_MSE | CSR_TIE | CSR_RIE;
}

static void RecvIntr(), XmitIntr();

/*
 * ----------------------------------------------------------------------------
 *
 * Dev_DC7085Interrupt --
 *
 *	Service an interrupt from the uart.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds serial, keyboard or mouse events to the queue.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
Dev_DC7085Interrupt()
{
    MASTER_LOCK(&dc7085Mutex);

    if (*csrPtr & CSR_RDONE) {
	RecvIntr();
    }

    if (*csrPtr & CSR_TRDY) {
	XmitIntr();
    }

    MASTER_UNLOCK(&dc7085Mutex);
}

static Boolean	shiftDown = FALSE;
static Boolean	ctrlDown = FALSE;
static Boolean	consoleCmd = FALSE;
static unsigned char lastChar = 0;


/*
 *----------------------------------------------------------------------
 *
 * RecvIntr --
 *
 *	Process a received character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Events added to the queue.
 *
 *----------------------------------------------------------------------
 */
ENTRY static void
RecvIntr()
{
    unsigned short	recvBuf;
    unsigned char	ch;
    unsigned char	asciiChar;

    while (*csrPtr & CSR_RDONE) {
	recvBuf = *rBufPtr;
	ch = recvBuf & 0xFF;

	switch ((recvBuf & RBUF_LINE_NUM) >> RBUF_LINE_NUM_SHIFT) {
	    case KBD_PORT: {
		if (devGraphicsOpen) {
		    MASTER_UNLOCK(&dc7085Mutex);
		    DevGraphicsKbdIntr(ch);
		    MASTER_LOCK(&dc7085Mutex);
		    break;
		}
		if (ch != KEY_REPEAT) {
		    if (ch == KEY_UP) {
			shiftDown = FALSE;
			ctrlDown = FALSE;
			break;
		    } else {
			if (ch == KEY_SHIFT) {
			    shiftDown = TRUE;
			    break;
			} else if (ch == KEY_CONTROL) {
			    ctrlDown = TRUE;
			    break;
			} else if (ch == KEY_CONSOLE_CMD) {
			    consoleCmd = TRUE;
			    break;
			}
		    }
		    lastChar = ch;
		} else {
		    ch = lastChar;
		}

		if (shiftDown) {
		    asciiChar = shiftedAscii[ch];
		} else {
		    asciiChar = unshiftedAscii[ch];
		}
		if (asciiChar >= KBD_NOKEY) {
		    /*
		     * A function key was typed - ignore it.
		     */
		    break;
		} else if (asciiChar > KBD_MAX_VALUE) {
		    printf("RecvIntr: Bad key code raw: %d, mapped: %d\n",
				ch, asciiChar);
		    break;
		} else {
		    if (asciiChar >= 'a' && asciiChar <= 'z') {
			if (ctrlDown) {
			    asciiChar = asciiChar - 'a' + '';
			} else if (shiftDown) {
			    asciiChar = asciiChar - 'a' + 'A';
			}
		    } else if (ctrlDown) {
			if (asciiChar >= '[' && asciiChar <= '_') {
			    asciiChar = asciiChar - '@';
			} else if (asciiChar == ' ' || asciiChar == '@') {
			    asciiChar = '\0'; 
			}
		    }
		}
		if (consoleCmd) {
		    consoleCmd = FALSE;
		    Dev_InvokeConsoleCmd(asciiChar);
		} else {
		    (*devKeyboard.inputProc)(devKeyboard.inputData, asciiChar);
		}
		break;
	    }
	    case MOUSE_PORT:
		if (devGraphicsOpen) {
		    MASTER_UNLOCK(&dc7085Mutex);
		    DevGraphicsMouseIntr(ch);
		    MASTER_LOCK(&dc7085Mutex);
		}
		break;
	    case MODEM_PORT:
		(*devSerialA.inputProc)(devSerialA.inputData, ch);
		break;
	    case PRINTER_PORT:
		(*devSerialB.inputProc)(devSerialB.inputData, ch);
		break;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * XmitIntr --
 *
 *	Handle a transmission interrupt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
XmitIntr()
{
    char c;
    int lineNum;

    if (!(*csrPtr & CSR_TRDY)) {
	printf("XmitIntr: Spurious interrupt\n");
	return;
    }
    lineNum = *csrPtr >> 8;
    switch (lineNum) {
	case KBD_PORT:
	    break;
	case MODEM_PORT:
	    c = (*devSerialA.outputProc)(devSerialA.outputData);
	    if (c == -1) {
		*tcrPtr &= ~(1 << MODEM_PORT);
	    } else {
		*tdrPtr = breakVal | c;
	    }
	    break;
	case PRINTER_PORT:
	    c = (*devSerialB.outputProc)(devSerialB.outputData);
	    if (c == -1) {
		*tcrPtr &= ~(1 << PRINTER_PORT);
	    } else {
		*tdrPtr = breakVal | c;
	    }
	    break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * DevDC7085Activate --
 *
 *	This procedure is invoked in order to "activate" one half of a
 *	DC7085 chip.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The channel is re-initialized and the receiver is started.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
DevDC7085Activate(dcPtr)
    register DevDC7085 *dcPtr;		/* Information about the device. */
{
    MASTER_LOCK(&dc7085Mutex);

    switch (dcPtr->port) {
	case KBD_PORT:
	    if (!(dcPtr->flags & LINE_ACTIVE)) {
		*lprPtr = LPR_RXENAB | LPR_B4800 | LPR_8_BIT_CHAR | KBD_PORT;
		dcPtr->flags |= LINE_ACTIVE;
	    }
	    break;
	case MODEM_PORT:
	case PRINTER_PORT:
	    if (!(dcPtr->flags & LINE_ACTIVE)) {
		*lprPtr = LPR_RXENAB | BaudToReg(dcPtr->baud) | 
			     LPR_8_BIT_CHAR | (1 << dcPtr->port);
		dcPtr->flags |= LINE_ACTIVE;
	    }
	    break;
    }

    MASTER_UNLOCK(&dc7085Mutex);
}


/*
 *----------------------------------------------------------------------
 *
 * DevDC7085RawProc --
 *
 *	This procedure is called back from the Td module as a raw
 *	control procedure.
 *
 * Results:
 *	The return value is the number of bytes returned to the caller
 *	at outBuffer.
 *
 * Side effects:
 *	Depends on the control operation.  Most likely effect is to
 *	start transferring output data.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY int
DevDC7085RawProc(dcPtr, operation, inBufSize, inBuffer, outBufSize, outBuffer)
    register DevDC7085 *dcPtr;	/* Our information about device. */
    int operation;		/* What to do:  TD_RAW_OUTPUT_READY etc. */
    int inBufSize;		/* Size of input buffer for operation. */
    char *inBuffer;		/* Input buffer. */
    int outBufSize;		/* Size of output buffer for operation. */
    char *outBuffer;		/* Output buffer. */
{
    int result = 0;

    MASTER_LOCK(&dc7085Mutex);

    switch (operation) {
	case TD_RAW_START_BREAK:
	    breakVal |= 1 << (dcPtr->port + 8);
	    *tdrPtr = breakVal;
	    break;

	case TD_RAW_STOP_BREAK:
	    breakVal &= ~(1 << (dcPtr->port + 8));
	    *tdrPtr = breakVal;
	    break;

	case TD_RAW_SET_DTR:
	    if (dcPtr->port == MODEM_PORT) {
		*tcrPtr |= TCR_DTR2;
	    }
	    break;

	case TD_RAW_CLEAR_DTR:
	    if (dcPtr->port == MODEM_PORT) {
		*tcrPtr &= ~TCR_DTR2;
	    }
	    break;

	case TD_RAW_SHUTDOWN:
	    if (dcPtr->flags & LINE_ACTIVE) {
		*lprPtr = dcPtr->port;
		dcPtr->flags &= ~LINE_ACTIVE;
		*tcrPtr &= ~(1 << dcPtr->port);
	    }
	    break;

	case TD_RAW_OUTPUT_READY:
	    if (!(dcPtr->flags & XMIT_ENABLED)) {
		*tcrPtr |= 1 << dcPtr->port;
		dcPtr->flags |= XMIT_ENABLED;
	    }
	    break;

	case TD_RAW_FLUSH_OUTPUT:
	    while ((*dcPtr->outputProc)(dcPtr->outputData) != -1) {
		/* do nothing */
	    }
	    break;

	case TD_RAW_FLOW_CHARS:
	    /* Ignore flow-control chars. */
	    break;

	case TD_RAW_SET_BAUD_RATE: {
	    Td_BaudRate *brPtr;
	    int		i;

	    /*
	     * Map the baud rate from an sgttyb constant to an actual
	     * number.  Return the value we actually set things to.
	     */

	    brPtr = (Td_BaudRate *) inBuffer;
	    for (i = 0; baudMap[i].baud != -1; i++) {
		if (baudMap[i].sgttybVal == brPtr->ospeed) {
		    dcPtr->baud = baudMap[i].baud;
		    break;
		}
	    }
	    switch (dcPtr->port) {
		case MODEM_PORT:
		    *lprPtr = LPR_RXENAB | BaudToReg(dcPtr->baud) | 
				 LPR_8_BIT_CHAR | MODEM_PORT;
		    break;
		case PRINTER_PORT:
		    *lprPtr = LPR_RXENAB | BaudToReg(dcPtr->baud) |
				 LPR_8_BIT_CHAR | PRINTER_PORT;
		    break;
	    }

	    /*
	     * Fall through to next arm of case to return current
	     * settings.
	     */
	}

	case TD_RAW_GET_BAUD_RATE: {
	    int i;
	    Td_BaudRate *brPtr;

	    brPtr = (Td_BaudRate *) outBuffer;
	    if (outBufSize >= sizeof(Td_BaudRate)) {
		for (i = 0; baudMap[i].baud != -1; i++) {
		    if (baudMap[i].baud == dcPtr->baud) {
			brPtr->ispeed = brPtr->ospeed = baudMap[i].sgttybVal;
			result = sizeof(Td_BaudRate);
		    }
		}
	    }
	    break;
	}
    }
    MASTER_UNLOCK(&dc7085Mutex);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * BaudToReg --
 *
 *	Map from a raw baud rate to the value to shove into the register.
 *
 * Results:
 *	The baud value to shove into the register.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
BaudToReg(baud)
    int	baud;
{
    int	i = 0;

    while (baudMap[i].baud != baud && baudMap[i].baud != -1) {
	i++;
    }
    return(baudMap[i].regVal);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DevDC7085MouseInit --
 *
 *	Initialize the mouse.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mouse is initialized.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
DevDC7085MouseInit()
{
    MASTER_LOCK(&dc7085Mutex);

    *lprPtr = LPR_RXENAB | LPR_B4800 | LPR_OPAR | LPR_PARENB |
			   LPR_8_BIT_CHAR | MOUSE_PORT;

    MASTER_UNLOCK(&dc7085Mutex);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DevDC7085MousePutCh --
 *
 *	Write a character to the mouse.  This is only called at initialization
 *	time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A character is written to the mouse.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
DevDC7085MousePutCh(c)
    int	c;
{
    register	int	timeout;
    register	int	reg;

    MASTER_LOCK(&dc7085Mutex);

    reg = *tcrPtr;
    *tcrPtr = 0x2;
    timeout = 60000;

    for (timeout = 60000;
     (!(*csrPtr & CSR_TRDY) || (*csrPtr & CSR_TX_LINE_NUM) != 0x100) &&
	     timeout > 0;
	 timeout--) {
    }
    *tdrPtr = c & 0xff;
    MACH_DELAY(50000);
    *tcrPtr = reg;

    MASTER_UNLOCK(&dc7085Mutex);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DevDC7085MouseGetCh --
 *
 *	Read a character from the mouse.  This is only called at
 *	initialization time.
 *
 * Results:
 *	A character read from the mouse, -1 if we timed out waiting.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY int
DevDC7085MouseGetCh()
{
    register int		timeout;
    register unsigned short	c;

    MASTER_LOCK(&dc7085Mutex);

    for (timeout = 1000000; timeout > 0; timeout--) {
	if (*csrPtr & CSR_RDONE) {
	    c = *rBufPtr;
	    MACH_DELAY(50000);
	    if (((c >> 8) & 03) != 1) {
		continue;
	    }
	    MASTER_UNLOCK(&dc7085Mutex);
	    return(c & 0xff);
	}
    }
    MASTER_UNLOCK(&dc7085Mutex);

    return(-1);
}


/*
 * ----------------------------------------------------------------------------
 *
 * DevDC7085KBDPutc --
 *
 *	Put a character out to the keyboard.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A character is written to the keyboard.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
DevDC7085KBDPutc(c)
    register int c;
{
    register unsigned	short	tcr;
    register int	timeout;
    int			line;

    MASTER_LOCK(&dc7085Mutex);

    tcr = *tcrPtr & 1;
    *tcrPtr |= 1;
    while (1) {
        timeout = 1000000;
        while (!(*csrPtr & CSR_TRDY) && timeout > 0) {
            timeout--;
        }
        if (timeout == 0) {
            break;
        }
        line = (*csrPtr >> 8) & 3;
        if (line != 0) {
            tcr |= 1 << line;
            *tcrPtr &= ~(1 << line);
            continue;
        }
        *tdrPtr = breakVal | (c & 0xff);
        MACH_DELAY(5);
        while (1) {
            while (!(*csrPtr & CSR_TRDY)) {
            }
            line = (*csrPtr >> 8) & 3;
            if (line != 0) {
                tcr |= 1 << line;
                *tcrPtr &= ~(1 << line);
                continue;
            }
            break;
        }
        break;
    }
    *tcrPtr &= ~1;
    if (tcr != 0) {
	*tcrPtr |= tcr;
    }

    MASTER_UNLOCK(&dc7085Mutex);
}

