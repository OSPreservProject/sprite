/* 
 * ttyDriver.c --
 *
 *	The routines in this module implement an emulator for the
 *	UNIX 4.2 BSD tty driver.  The emulation is done in a way
 *	that is independent of the specific environme (kernel, user,
 *	etc.) by using a set of callback procedures to interface to
 *	a raw device on one side and a client on the "cooked" side..
 *
 * Copyright 1987, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/ttyDriver.c,v 1.20 92/03/18 16:33:08 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <dev/tty.h>
#include <fs.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fmt.h>
#include "td.h"

/*
 * The structure below corresponds to one terminal (one call to Td_Create).
 */

typedef struct Terminal {
    /*
     * Controlling parameters for terminal.  These all have exactly the
     * same meanings as in 4.3 BSD;  see the 4.3 BSD terminal driver
     * documentation for details.
     */

    struct sgttyb sgttyb;
    struct tchars tchars;
    struct ltchars ltchars;
    int localMode;
    struct winsize winsize;

    /*
     * State of the terminal.
     */

    int column;			/* Column where the next character will be
				 * echoed (i.e., cursor position).  Needed
				 * in order to handle tabs correctly. */
    int owner;			/* Identifier of controlling process or process
				 * group (signals are sent here). */
    int numOpens;		/* Number of opens that have been completed
				 * successfully but not yet closed. */
    int flags;			/* See below for definitions. */

    /*
     * Input buffer, indexed circularly.  The buffer is reallocated
     * with a larger size if it ever fills up.
     */

    char *inputBuffer;		/* Input buffer area (malloc'ed). */
    int inBufSize;		/* Number of bytes in inputBuffer. */
    int lastAddedIn;		/* Index of last character added to buffer. */
    int lastRemovedIn;		/* Index of last place from which character
				 * was removed from buffer.  If lastAddedIn =
				 * lastRemovedIn, the buffer is empty. */
    int lastBreak;		/* Index of last break character (newline,
				 * etc.) or lastRemovedIn if none in buffer. */
    int lastHidden;		/* Value of lastAddedIn at the time the last
				 * output to the terminal was done.  Characters
				 * before this can't be smoothly backspaced
				 * over, because there's output in front of
				 * them on the screen.  -1 means there are no
				 * editable characters in the buffer that are
				 * hidden. */

    /*
     * In order to backspace correctly over weird things like tabs, the
     * following two variables keep track of an index position in the input
     * buffer, and the column just to the right of where that character was
     * echoed on the display.  KeyIndex is either the same as lastBreak or
     * lastHidden or lastRemovedIn, and has two properties:  a) it will
     * never be backspaced over in CRT mode;  and b) no characters beyond
     * this one will be returned to the application until keyIndex is first
     * advanced.
     */

    int keyIndex;
    int keyColumn;		/* Column just after keyIndex character. */

    /*
     * Output buffer, indexed circularly.  This buffer also grows
     * dynamically if necessary (this is necessary so that new room can
     * be made for characters being echoed, even if the buffer was
     * previously "full").  However, the cooked side of the terminal
     * is marked "not ready for output" whenever there are more than
     * cookedOutputLimit characters in the buffer (but any call to
     * Td_PutCooked will always complete;  it is up to the cooked-side
     * callbacks to stop calling Td_PutCooked).  The goal here is to
     * keep the application from getting too many characters ahead of
     * the actual device.
     */

    char *outputBuffer;		/* Output buffer area (malloc'ed). */
    int outBufSize;		/* Number of bytes in outputBuffer. */
    int lastAddedOut;		/* Index of last character added to
				 * outputBuffer. */
    int lastRemovedOut;		/* Index of last character removed from
				 * outputBufer. */
    int outCharsBuffered;	/* Number of characters bufferred in
				 * outputBuffer. */
    int cookedOutputLimit;	/* Mark cooked side not ready for output
				 * whenever outCharsBuffered is greater
				 * than this. */

    /*
     * Callback procedures and data provided by the client:
     */

    int (*cookedProc)_ARGS_((ClientData, int operation, int inBufSize, 
			     char *inBuffer, int outBufSize,
			     char *outBuffer)); 
				/* Procedure to call to register change
				 * in state of cooked-side interface. */
    ClientData cookedData;	/* Value to pass to cookedProc. */
    int (*rawProc)_ARGS_((ClientData, int operation, int inBufSize,
			  char *inBuffer, int outBufSize,
			  char *outBuffer));
				/* Procedure to call to register change
				 * in state of raw-side interface. */
    ClientData rawData;		/* Value to pass to rawProc. */
} Terminal;

/* Flag values:
 *
 * EXCLUSIVE:		No more opens should be allowed until terminal
 *			has been completely closed.
 * BS_IN_PROGRESS:	A printing backspace sequence (delimited by
 *			"\" and "/") is in progress, and will eventually
 *			need the closing "/".
 * LITERAL_NEXT:	The next character should be taken literally,
 *			and should be put into the input buffer with
 *			no special interpretation.
 * OWNER_FAMILY:	1 means the owner is a process family.  0 means
 *			it's a single process.
 * OUTPUT_OFF:		1 means output to the raw device has been stopped,
 *			for example because ^S was typed.
 */

#define EXCLUSIVE		0x1
#define BS_IN_PROGRESS		0x2
#define LITERAL_NEXT		0x4
#define OWNER_FAMILY		0x8
#define OUTPUT_OFF		0x10

/*
 * Default values for tty parameters:
 */

struct sgttyb sgttybDefault = {
    B9600, B9600, 010, 025, EVENP|ODDP|CRMOD|ECHO
};
struct tchars tcharsDefault = {
    03, 034, 021, 023, 04, -1
};
struct ltchars ltcharsDefault = {
    032, 031, 022, 017, 027, 026
};
int localModeDefault = LCRTBS|LCRTERA|LCRTKIL|LCTLECH;
struct winsize winsizeDefault = {
    0, 0, 0, 0
};

/*
 * Macros for moving buffer pointers forward and backward circularly.
 */

#define NEXT(src, size, dst) 		\
    (dst) = (src)+1;			\
    if ((dst) >= (size)) {		\
	(dst) = 0;			\
    }

#define PREV(src, size, dst)		\
    (dst) = (src)-1;			\
    if ((dst) < 0) {			\
	(dst) = (size)-1;			\
    }

int td_Debug = 0;

/*
 * Forward declarations for procedures defined later in this file:
 */

static void	TdBackspace _ARGS_((Terminal *tPtr));
static void	TdEcho _ARGS_((Terminal *tPtr, int c));
static void	TdFlushInput _ARGS_((Terminal *tPtr));
static void	TdFlushOutput _ARGS_((Terminal *tPtr));
static void	TdPutChar _ARGS_((Terminal *tPtr, int c));
static void	TdRetypeInput _ARGS_((Terminal *tPtr, int start));
static int	FormatInput _ARGS_((int command, Fmt_Format format,
				    int inputSize, Address input,
				    int *newInputSizePtr, Address newInput));
static int	FormatOutput _ARGS_((int command, Fmt_Format format,
				     int outputSize, Address output,
				     int *newOutputSizePtr,
				     Address newOutput));


/*
 *----------------------------------------------------------------------
 *
 * Td_Create --
 *
 *	This procedure creates and initializes a new terminal
 *	driver.
 *
 * Results:
 *	The return value is a handle that is used to refer to the
 *	driver when calling other Td_ procedures, such as Td_Delete.
 *
 * Side effects:
 *	The procedures cookedProc and rawProc may be invoked by
 *	other procedures in this module at later times.  See the
 *	man page for details.
 *
 *----------------------------------------------------------------------
 */

Td_Terminal
Td_Create(bufferSize, cookedProc, cookedData, rawProc, rawData)
    int bufferSize;		/* How much buffer space to allow on
				 * output. */
    int (*cookedProc)_ARGS_((ClientData, int operation, int inBufSize, 
			     char *inBuffer, int outBufSize,
			     char *outBuffer)); 
				/* Procedure to call for control operations
				 * on cooked side of driver. */
    ClientData cookedData;	/* Arbitrary value, provided by caller,
				 * which will be passed to cookedProc
				 * whenever it is invoked. */
    int (*rawProc)_ARGS_((ClientData, int operation, int inBufSize,
			  char *inBuffer, int outBufSize,
			  char *outBuffer));
				/* Procedure to call for control operations
				 * on raw side of driver. */
    ClientData rawData;		/* Arbitrary value, provided by caller,
				 * which will be passed to rawProc whenever
				 * it is invoked. */
{
    register Terminal *tPtr;
    Td_BaudRate baud;

    tPtr = (Terminal *) malloc(sizeof(Terminal));
    tPtr->sgttyb = sgttybDefault;
    tPtr->tchars = tcharsDefault;
    tPtr->ltchars = ltcharsDefault;
    tPtr->localMode = localModeDefault;
    tPtr->winsize = winsizeDefault;
    tPtr->column = 0;
    tPtr->owner = -1;
    tPtr->flags = 0;
    tPtr->inputBuffer = (char *) malloc(100);
    tPtr->inBufSize = 100;
    tPtr->lastAddedIn = 0;
    tPtr->lastRemovedIn = 0;
    tPtr->lastBreak = 0;
    tPtr->lastHidden = -1;
    tPtr->keyIndex = 0;
    tPtr->keyColumn = 0;
    tPtr->outputBuffer = (char *) malloc(1000);
    tPtr->outBufSize = 1000;
    tPtr->lastAddedOut = 0;
    tPtr->lastRemovedOut = 0;
    tPtr->outCharsBuffered = 0;
    tPtr->cookedOutputLimit = bufferSize;
    tPtr->cookedProc = cookedProc;
    tPtr->cookedData = cookedData;
    tPtr->rawProc = rawProc;
    tPtr->rawData = rawData;

    /*
     * Fetch the actual baud rate from the raw device manager.
     */

    if ((*rawProc)(rawData, TD_RAW_GET_BAUD_RATE, 0, (char *) NULL,
	    sizeof(baud), (char *) &baud) == sizeof(baud)) {
	tPtr->sgttyb.sg_ispeed = baud.ispeed;
	tPtr->sgttyb.sg_ospeed = baud.ospeed;
    }

    return (Td_Terminal) tPtr;
}

/*
 *----------------------------------------------------------------------
 *
 *  Td_Delete --
 *
 *	Close down a terminal driver, destroying all of the state
 *	associated with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A hangup is simulated on the cooked side of the driver, and
 *	memory is released.  The caller should never again use
 *	terminal.
 *
 *----------------------------------------------------------------------
 */

void
Td_Delete(terminal)
    Td_Terminal terminal;	/* Token identifying terminal (returned
				 * by previous call to Td_Create). */
{
    register Terminal *tPtr = (Terminal *) terminal;

    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_SHUTDOWN, 0, (char *) NULL,
	    0, (char *) NULL);
    free((char *) tPtr->inputBuffer);
    free((char *) tPtr->outputBuffer);
    free((char *) tPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Td_Open --
 *
 *	This procedure should be called before accepting a new
 *	"open" for a terminal.  It indicates whether new opens
 *	are permitted.
 *
 * Results:
 *	The return value is zero if the open is to be permitted,
 *	and a non-zero errno value if it is to be denied.  If the
 *	open is successful, *selectBitsPtr is filled in with the
 *	initial select state for the terminal.
 *
 * Side effects:
 *	Information counting open streams on the terminal gets
 *	updated.
 *
 *----------------------------------------------------------------------
 */

int
Td_Open(terminal, selectBitsPtr)
    Td_Terminal terminal;		/* Token for the terminal to be
					 * checked. */
    int *selectBitsPtr;			/* Put initial select state here. */
{
    register Terminal *tPtr = (Terminal *) terminal;

    if (tPtr->flags & EXCLUSIVE) {
	return EBUSY;
    }
    tPtr->numOpens++;
    *selectBitsPtr = 0;
    if ((tPtr->lastBreak != tPtr->lastRemovedIn)
	    || (tPtr->localMode & LPENDIN)) {
	*selectBitsPtr |= FS_READABLE;
    }
    if (tPtr->outCharsBuffered < tPtr->cookedOutputLimit) {
	*selectBitsPtr |= FS_WRITABLE;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Td_Close --
 *
 *	This procedure should be called whenever all of the I/O streams
 *	associated with a single open on a terminal have been closed.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	State in the terminal is updated to reflect the close.
 *
 *----------------------------------------------------------------------
 */

void
Td_Close(terminal)
    Td_Terminal terminal;		/* Token for the terminal to be
					 * checked. */
{
    register Terminal *tPtr = (Terminal *) terminal;

    tPtr->numOpens--;
    if (tPtr->numOpens == 0) {
	tPtr->flags &= ~EXCLUSIVE;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Td_GetCooked --
 *
 *	Retrieve characters that are ready to be read from the cooked
 *	side of a terminal driver.
 *
 * Results:
 *	The return value is normally 0.  If non-zero, it indicates
 *	that an error occurred and holds a UNIX errno.  Most often,
 *	this is EWOULDBLOCK, meaning that no characters were
 *	available.  The value at numCharsPtr is overwritten with the
 *	actual number of characters returned at buffer, and will be
 *	0 if an error or end-of-file occurred.  *SigNumPtr is overwritten
 *	with a signal to send to the invoking process, or 0.  *SelectBitsPtr
 *	is updated to reflect the readability of the terminal.
 *
 * Side effects:
 *	Characters are removed from the terminal's input buffer.
 *
 *----------------------------------------------------------------------
 */

int
Td_GetCooked(terminal, pID, familyID, numCharsPtr, buffer, 
	sigNumPtr, selectBitsPtr)
    Td_Terminal terminal;	/* Token identifying terminal. */
    int pID;			/* Process invoking operation. */
    int familyID;		/* Family of pID. */
    int *numCharsPtr;		/* Points to maximum number of characters to
				 * read from terminal. Overwritten by number
				 * of chars. actually returned. */
    char *buffer;		/* Where to place characters that are read. */
    int *sigNumPtr;		/* Overwrite this with the number of a signal
				 * to generate for the calling process.  0
				 * means no signal. */
    int *selectBitsPtr;		/* The FS_READABLE bit in this word gets
				 * updated to reflect whether or not there
				 * are still more readable characters after
				 * the ones returned. */
{
    register Terminal *tPtr = (Terminal *) terminal;
    register char *dest;
    int count, result;

    *sigNumPtr = 0;

    /*
     * See if this process owns the terminal.  If not, then signal it
     * and don't give it any input.
     */

    if (!(tPtr->flags & OWNER_FAMILY)) {
	if ((pID != tPtr->owner) && (tPtr->owner != -1)) {
	    notOwner:
	    *sigNumPtr = SIGTTIN;
	    *numCharsPtr = 0;
	    result = EINTR;
	    goto done;
	}
    } else if ((familyID != tPtr->owner) && (tPtr->owner != -1)) {
	goto notOwner;
    }

    /*
     * Re-echo buffered characters, if so requested.
     */

    if (tPtr->localMode & LPENDIN) {
	int oldCharsBuffered;

	oldCharsBuffered = tPtr->outCharsBuffered;
	tPtr->localMode &= ~LPENDIN;
	TdRetypeInput(tPtr, tPtr->lastRemovedIn);
	if ((oldCharsBuffered == 0) && (tPtr->outCharsBuffered != 0) &&
		!(tPtr->flags & OUTPUT_OFF)) {
	    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_OUTPUT_READY,
		    0, (char *) NULL, 0, (char *) NULL);
	}
    }

    /*
     * Make sure there's information ready for the terminal.  If not,
     * then block the process.
     */

    if (tPtr->lastBreak == tPtr->lastRemovedIn) {
	*numCharsPtr = 0;
	*selectBitsPtr &= ~FS_READABLE;
	return EWOULDBLOCK;
    }

    /*
     * Copy bytes from the input buffer to the caller's buffer,
     * and update the terminal's input buffer pointer.
     */

    count = 0;
    dest = buffer;
    while ((tPtr->lastRemovedIn != tPtr->lastBreak) &&
	    (count < *numCharsPtr)) {
	register char c;

	NEXT(tPtr->lastRemovedIn, tPtr->inBufSize, tPtr->lastRemovedIn);
	count++;
	c = tPtr->inputBuffer[tPtr->lastRemovedIn];
	*dest = c;
	dest++;
	if (!(tPtr->sgttyb.sg_flags & (RAW|CBREAK))) {
	    if (c == tPtr->tchars.t_eofc) {
		count--;		/* Don't return end-of-file chars. */
		break;
	    } else if ((c == '\n') || (c == tPtr->tchars.t_brkc)) {
		break;
	    }
	}
    }
    *numCharsPtr = count;
    result = 0;

    done:
    if (tPtr->lastBreak == tPtr->lastRemovedIn) {
	*selectBitsPtr &= ~FS_READABLE;
    } else {
	*selectBitsPtr |= FS_READABLE;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Td_PutCooked --
 *
 *	Add characters to the output buffer for a terminal.
 *
 * Results:
 *	The return value is always 0 (the output is grown enough to
 *	hold all the output characters).  The value at *numBytesPtr
 *	is left unchanged to indicate that all the characters were
 *	accepted, *sigNumPtr is overwritten with a signal to send
 *	to the invoking process (or 0), and *selectBitsPtr is updated
 *	to reflect whether the terminal's output buffer is now full.
 *
 * Side effects:
 *	Output processing is performed on the characters in buffer,
 *	and they are queued for output on the terminal's raw side.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Td_PutCooked(terminal, numBytesPtr, buffer, sigNumPtr, selectBitsPtr)
    Td_Terminal terminal;	/* Token identifying terminal. */
    int *numBytesPtr;		/* Points to maximum number of characters
				 * to write to terminal.  Not modified
				 * by this procedure. */
    register char *buffer;	/* Characters to write. */
    int *sigNumPtr;		/* Overwrite this with the number of a signal
				 * to generate for the calling process.  0
				 * means no signal. */
    int *selectBitsPtr;		/* The FS_WRITABLE bit in this word gets
				 * updated to reflect whether or not there
				 * are still more space available in the
				 * terminal's output buffer. */
{
    register Terminal *tPtr = (Terminal *) terminal;
    register char c;
    int i, oldCharsBuffered;

    *sigNumPtr = 0;

    oldCharsBuffered = tPtr->outCharsBuffered;
    for (i = 0; i < *numBytesPtr; i++, buffer++) {
	c = *buffer;
	if ((tPtr->sgttyb.sg_flags & RAW) || (tPtr->localMode & LLITOUT)) {
	    TdPutChar(tPtr, c);
	    continue;
	}
	c &= 0177;
	if (c == 04) {	/* End of file (^D) ignored */
	    continue;
	} else if ((c == '\n') && (tPtr->sgttyb.sg_flags & CRMOD)) {
	    TdPutChar(tPtr, '\r');
	    TdPutChar(tPtr, '\n');
	} else {
	    TdPutChar(tPtr, c);
	}
    }
    tPtr->keyIndex = tPtr->lastAddedIn;
    tPtr->keyColumn = tPtr->column;
    if (tPtr->lastAddedIn != tPtr->lastRemovedIn) {
	tPtr->lastHidden = tPtr->lastAddedIn;
    }
    if ((oldCharsBuffered == 0) && (tPtr->outCharsBuffered != 0) &&
	    !(tPtr->flags & OUTPUT_OFF)) {
	(*tPtr->rawProc)(tPtr->rawData, TD_RAW_OUTPUT_READY,
		0, (char *) NULL, 0, (char *) NULL);
    }
    if (tPtr->outCharsBuffered >= tPtr->cookedOutputLimit) {
	*selectBitsPtr &= ~FS_WRITABLE;
    } else {
	*selectBitsPtr |= FS_WRITABLE;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Td_ControlCooked --
 *
 *	This procedure is used to invoke iocontrol operations on the
 *	cooked side of a terminal driver.
 *
 * Results:
 *	If the operation completed successfully then the return value
 *	is zero.  If the operation failed, then the return value is a
 *	UNIX errno indicating what went wrong.  *SigNumPtr gets
 *	overwritten with the number of a signal to send to the invoking
 *	process (or 0), and *selectBitsPtr gets filled in with information
 *	about whether or not the terminal is now readable or writable.
 *
 * Side effects:
 *	Depends on the iocontrol;  see the tty(4) man page for details.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Td_ControlCooked(terminal, command, format, inputSize, input, outputSizePtr,
	output, sigNumPtr, selectBitsPtr)
    Td_Terminal terminal;		/* Token for terminal. */
    int command;			/* Iocontrol operation to perform
					 * (	e.g. TIOCGETP). */
    Fmt_Format format;			/* Byte-order/alignment format */
    int inputSize;			/* Size of input, in bytes. */
    char *input;			/* Input buffer:  contains information
					 * provided by client as input to
					 * operation. */
    int *outputSizePtr;			/* Largest amount of output that
					 * client is prepared to receive.  This
					 * value is overwritten with the count
					 * of actual bytes returned. */
    char *output;			/* Place to store output bytes;
					 * provided by caller. */
    int *sigNumPtr;			/* Overwrite this with the number of
					 * a signal to generate for the
					 * calling process.  0 means no
					 * signal. */
    int *selectBitsPtr;			/* Store new select state of terminal
					 * here. */
{
    register Terminal *tPtr = (Terminal *) terminal;
    int count, result;
    char *out = (char *) NIL;
    int i;
    Ioc_Owner owner;
    int oldIspeed, oldOspeed, oldFlags, oldStopc, oldStartc;

    /*
     * The union below describes all of the possible formats in which
     * the input area may appear.
     */

    typedef union {
	int		i;
	char		chars[20];
	struct sgttyb	sgttyb;
	struct tchars	tchars;
	struct ltchars	ltchars;
	Ioc_Owner	owner;
	struct winsize	winsize;
    } InBuf;
    InBuf newInputBuf;
    register InBuf *in = (InBuf *) input;

    if (td_Debug) {
	printf("Td_ControlCooked: command %d\n", command);
    }

    if (format != FMT_MY_FORMAT) {
	/*
	 * Fix up the formatting of the input buffer.
	 */
	int newSize = sizeof(newInputBuf);
	if (FormatInput(command, format, inputSize, input,
			    &newSize, (Address) &newInputBuf) != FMT_OK) {
	    goto invalid;
	}
	in = &newInputBuf;
	inputSize = newSize;
    }
    /*
     * Save certain pieces of information about the terminal so that
     * if they change we can call the raw control procedure.
     */

    oldIspeed = tPtr->sgttyb.sg_ispeed;
    oldOspeed = tPtr->sgttyb.sg_ospeed;
    oldFlags = tPtr->sgttyb.sg_flags;
    oldStopc = tPtr->tchars.t_stopc;
    oldStartc = tPtr->tchars.t_startc;

    *sigNumPtr = 0;
    count = 0;
    switch (command) {

	case IOC_TTY_SET_DISCIPLINE:
	    if ((inputSize != sizeof(int))
		    || (in->i != NTTYDISC)) {
		goto invalid;
	    }
	    break;

	case IOC_TTY_GET_DISCIPLINE:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    i = NTTYDISC;
	    out = (char *) &i;
	    count = sizeof(int);
	    break;

	case IOC_TTY_GETP:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    out = (char *) &tPtr->sgttyb;
	    count = sizeof(struct sgttyb);
	    break;

	case IOC_TTY_SETP:
	    /*
	     * Technically, this code should delay until all characters
	     * currently buffered for output have been printed, but
	     * there's no easy way to do that here:  ioctls must complete
	     * immediately.
	     */
	    TdFlushInput(tPtr);
	case IOC_TTY_SETN:
	    if (inputSize != sizeof(struct sgttyb)) {
		goto invalid;
	    }
	    if ((tPtr->sgttyb.sg_flags ^ in->sgttyb.sg_flags) & RAW) {
		/*
		 * Going into or out of raw mode;  always flush input
		 * buffer.
		 */

		TdFlushInput(tPtr);
	    }
	    tPtr->sgttyb = in->sgttyb;
	    break;

	case IOC_TTY_EXCL:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    tPtr->flags |= EXCLUSIVE;
	    break;

	case IOC_TTY_NXCL:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    tPtr->flags &= ~EXCLUSIVE;
	    break;

	case IOC_TTY_HUP_ON_CLOSE:		/* Not implemented. */
	    goto invalid;

	case IOC_TTY_FLUSH: {
	    /*
	     * For compatibility with TIOCFLUSH, we accept one
	     * integer argument which has the FREAD and FWRITE bits in it.
	     */
	    int flags;
	    if (inputSize == 0) {
		flags = 0;
	    } else {
		flags = in->i;
	    }
#ifndef FREAD
#define FREAD	0x1
#define FWRITE	0x2
#endif
	    if (flags == 0) {
		flags = FREAD|FWRITE;
	    }
	    if (flags & FREAD) {
		TdFlushInput(tPtr);
	    }
	    if (flags & FWRITE) {
		TdFlushOutput(tPtr);
	    }
	    break;
	}
	case IOC_TTY_INSERT_CHAR:
	    if (inputSize != 1) {
		goto invalid;
	    }
	    Td_PutRaw((Td_Terminal) tPtr, 1, &in->chars[0]);
	    break;

	case IOC_TTY_SET_BREAK:
	    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_START_BREAK,
		    0, (char *) NULL, 0, (char *) NULL);
	    break;

	case IOC_TTY_CLEAR_BREAK:
	    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_START_BREAK,
		    0, (char *) NULL, 0, (char *) NULL);
	    break;

	case IOC_TTY_SET_DTR:
	    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_SET_DTR,
		    0, (char *) NULL, 0, (char *) NULL);
	    break;

	case IOC_TTY_CLEAR_DTR:	
	    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_CLEAR_DTR,
		    0, (char *) NULL, 0, (char *) NULL);
	    break;

	case IOC_GET_OWNER:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    owner.id = tPtr->owner;
	    if (tPtr->flags & OWNER_FAMILY) {
		owner.procOrFamily = IOC_OWNER_FAMILY;
	    } else {
		owner.procOrFamily = IOC_OWNER_PROC;
	    }
	    out = (char *) &owner;
	    count = sizeof(owner);
	    break;

	case IOC_SET_OWNER:
	    if (inputSize != sizeof(Ioc_Owner)) {
		goto invalid;
	    }
	    tPtr->owner = in->owner.id;
	    if (in->owner.procOrFamily == IOC_OWNER_FAMILY) {
		tPtr->flags |= OWNER_FAMILY;
	    } else {
		tPtr->flags &= ~OWNER_FAMILY;
	    }
	    break;

	case IOC_NUM_READABLE:
	    i = tPtr->lastBreak - tPtr->lastRemovedIn;
	    if (i < 0) {
		i += tPtr->inBufSize;
	    }
	    out = (char *) &i;
	    count = sizeof(int);
	    break;

	case IOC_TTY_GET_TCHARS:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    out = (char *) &tPtr->tchars;
	    count = sizeof(struct tchars);
	    break;

	case IOC_TTY_SET_TCHARS:
	    if (inputSize != sizeof(struct tchars)) {
		goto invalid;
	    }
	    tPtr->tchars = in->tchars;
	    break;

	case IOC_TTY_BIS_LM:
	    if (inputSize != sizeof(int)) {
		goto invalid;
	    }
	    tPtr->localMode |= in->i;
	    break;

	case IOC_TTY_BIC_LM:
	    if (inputSize != sizeof(int)) {
		goto invalid;
	    }
	    tPtr->localMode &= ~in->i;
	    break;

	case IOC_TTY_SET_LM:
	    if (inputSize != sizeof(int)) {
		goto invalid;
	    }
	    tPtr->localMode = in->i;
	    break;

	case IOC_TTY_GET_LM:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    out = (char *) &tPtr->localMode;
	    count = sizeof(int);
	    break;

	case IOC_TTY_SET_LTCHARS:
	    if (inputSize != sizeof(struct ltchars)) {
		goto invalid;
	    }
	    tPtr->ltchars = in->ltchars;
	    break;

	case IOC_TTY_GET_LTCHARS:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    out = (char *) &tPtr->ltchars;
	    count = sizeof(struct ltchars);
	    break;

	case IOC_GET_FLAGS:
	    i = 0;
	    out = (char *) &i;
	    count = sizeof(int);
	    break;
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	case IOC_TTY_NOT_CONTROL_TTY:
	    break;

	case IOC_TTY_GET_WINDOW_SIZE:
	    if (inputSize != 0) {
		goto invalid;
	    }
	    out = (char *) &tPtr->winsize;
	    count = sizeof(struct winsize);
	    break;

	case IOC_TTY_SET_WINDOW_SIZE: {
	    Td_Signal signalInfo;
	    if (inputSize != sizeof(struct winsize)) {
		goto invalid;
	    }
	    tPtr->winsize = in->winsize;
	    signalInfo.sigNum = SIGWINCH;
	    signalInfo.groupID = tPtr->owner;
	    (*tPtr->cookedProc)(tPtr->cookedData, TD_COOKED_SIGNAL,
		    sizeof(signalInfo), (char *) &signalInfo,
		    0, (char *) NULL);
	    break;
	}

	default:
	    goto invalid;
    }
    /*
     * Fix up output buffer for the client.  At this point
     * count = size of output data
     * out = pointer to output data
     * Here we rely on the Fmt_ library doing essentially a bcopy
     * if our format and the client's format are the same.
     */
    if (count != 0) {
	result = FormatOutput(command, format, count, out,
			      outputSizePtr, output);
	if (result != FMT_OK) {
	    result = EINVAL;
	    if (td_Debug) {
		printf("Td_ControlCooked: command %d invalid output\n", command);
	    }
	}
    } else {
	*outputSizePtr = 0;
	result = 0;
    }

    /*
     * Call the raw control procedure if anything changed that it needs
     * to know about.
     */

    if ((oldIspeed != tPtr->sgttyb.sg_ispeed)
	    || (oldOspeed != tPtr->sgttyb.sg_ospeed)) {
	Td_BaudRate baud, baud2;

	baud.ispeed = tPtr->sgttyb.sg_ispeed;
	baud.ospeed = tPtr->sgttyb.sg_ospeed;
	if ((*tPtr->rawProc)(tPtr->rawData, TD_RAW_SET_BAUD_RATE,
		sizeof(baud), (char *) &baud,
		sizeof(baud2), (char *) &baud2) == sizeof(baud2)) {

	    /*
	     * Device has overridden the baud-rate change;  take its advice.
	     */

	    tPtr->sgttyb.sg_ispeed = baud2.ispeed;
	    tPtr->sgttyb.sg_ospeed = baud2.ospeed;
	}
    }
    if (((oldFlags & RAW) != (tPtr->sgttyb.sg_flags & RAW))
	    || (oldStopc != tPtr->tchars.t_stopc)
	    || (oldStartc != tPtr->tchars.t_startc)) {
	Td_FlowChars flow;

	if (tPtr->sgttyb.sg_flags & RAW) {
	    flow.stop = flow.start = -1;
	} else {
	    flow.stop = tPtr->tchars.t_stopc;
	    flow.start = tPtr->tchars.t_startc;
	}
	(*tPtr->rawProc)(tPtr->rawData, TD_RAW_FLOW_CHARS,
		sizeof(flow), (char *) &flow, 0, (char *) NULL);
    }

    /*
     * Setting the select bits is a bit tricky:  if LPENDIN is set, then
     * we need to find out when the next read is done.  So, make the device
     * appear to be readable even if it isn't.  Otherwise, we won't be told
     * when the device is read.
     */

    done:
    *selectBitsPtr = 0;
    if ((tPtr->lastBreak != tPtr->lastRemovedIn)
	    || (tPtr->localMode & LPENDIN)) {
	*selectBitsPtr |= FS_READABLE;
    }
    if (tPtr->outCharsBuffered < tPtr->cookedOutputLimit) {
	*selectBitsPtr |= FS_WRITABLE;
    }
    return result;

    invalid:
    if (td_Debug) {
	printf("Td_ControlCooked: command %d invalid input\n", command);
    }
    *outputSizePtr = 0;
    result = EINVAL;
    goto done;
}

/*
 *----------------------------------------------------------------------
 *
 * FormatInput --
 *
 *	Re-format the input buffer of an I/O control.  This is required
 *	if the client is on a host with a different byte order/alignment.
 *	This uses the Fmt_Convert library routine.
 *
 * Results:
 *	This returns zero if all goes well.
 *	Otherwise a FMT_ error code is returned.
 *
 * Side effects:
 *	The reformatted input is put into newBuffer.  The true size of
 *	the data in this buffer is returned in *newInputSizePtr.
 *
 *----------------------------------------------------------------------
 */

static int
FormatInput(command, format, inputSize, input, newInputSizePtr, newInput)
    int command;		/* I/O Control command */
    Fmt_Format format;		/* Format of client host */
    int inputSize;		/* Size of input buffer */
    Address input;		/* Input buffer in client format */
    int *newInputSizePtr;	/* In/Out - Size of new input buffer */
    Address newInput;		/* Out - Input buffer in our format */
{
    int status = FMT_OK;
    char *fmtString = "";

    switch (command) {

	case IOC_TTY_GET_DISCIPLINE:
	case IOC_TTY_GETP:
	case IOC_TTY_EXCL:
	case IOC_TTY_NXCL:
	case IOC_GET_OWNER:
	case IOC_TTY_GET_TCHARS:
	case IOC_TTY_GET_LM:
	case IOC_TTY_GET_LTCHARS:
	case IOC_TTY_GET_WINDOW_SIZE:
	case IOC_TTY_NOT_CONTROL_TTY:
	default:
	    *newInputSizePtr = 0;
	    goto noconversion;

	case IOC_TTY_FLUSH:
	    /*
	     * Optional int
	     */
	    if (inputSize == 0) {
		*newInputSizePtr = 0;
		goto noconversion;
	    } else {
		fmtString = "w";
	    }
	    break;

	case IOC_TTY_SET_DISCIPLINE:
	case IOC_TTY_BIS_LM:
	case IOC_TTY_BIC_LM:
	case IOC_TTY_SET_LM:
	    /*
	     * One int
	     */
	    fmtString = "w";
	    break;

	case IOC_TTY_SETP:
	case IOC_TTY_SETN:
	    /*
	     * struct sgttyb
	     */
	    fmtString = "{b4h}";
	    break;

	case IOC_TTY_INSERT_CHAR:
	    /*
	     * One char
	     */
	    fmtString = "b";
	    break;

	case IOC_SET_OWNER:
	    /*
	     * Ioc_Owner
	     */
	    fmtString = "{w2}";
	    break;

	case IOC_TTY_SET_TCHARS:
	    /*
	     * struct tchars
	     */
	    fmtString = "{b6}";
	    break;

	case IOC_TTY_SET_LTCHARS:
	    /*
	     * struct ltchars
	     */
	    fmtString = "{b6}";
	    break;


	case IOC_TTY_SET_WINDOW_SIZE: {
	    /*
	     * struct winsize
	     */
	    fmtString = "{h4}";
	    break;
	}

    }
    status = Fmt_Convert(fmtString, format, &inputSize, input, FMT_MY_FORMAT,
	    newInputSizePtr, newInput);
noconversion:
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FormatOutput --
 *
 *	Re-format the output buffer of an I/O control.
 *	This uses the Fmt_Convert library routine.
 *
 * Results:
 *	This returns zero if all goes well.
 *	Otherwise a FMT_ error code is returned.
 *
 * Side effects:
 *	The reformatted output is put into newOutput.  The true size of
 *	the data in this buffer is returned in *newOutputSizePtr.
 *
 *----------------------------------------------------------------------
 */

static int
FormatOutput(command, format, outputSize, output, newOutputSizePtr, newOutput)
    int command;		/* I/O Control command */
    Fmt_Format format;		/* Format of client host */
    int outputSize;		/* Size of input buffer (in our format) */
    Address output;		/* Output buffer in our format */
    int *newOutputSizePtr;	/* In/Out - Size of new output buffer */
    Address newOutput;		/* Out - Output buffer in the client's format */
{
    int status = FMT_OK;
    char *fmtString = "";

    switch (command) {

	case IOC_TTY_SET_DISCIPLINE:
	case IOC_TTY_SETP:
	case IOC_TTY_SETN:
	case IOC_TTY_EXCL:
	case IOC_TTY_NXCL:
	case IOC_TTY_FLUSH:
	case IOC_TTY_INSERT_CHAR:
	case IOC_SET_OWNER:
	case IOC_TTY_SET_TCHARS:
	case IOC_TTY_BIS_LM:
	case IOC_TTY_BIC_LM:
	case IOC_TTY_SET_LM:
	case IOC_TTY_SET_LTCHARS:
	case IOC_TTY_NOT_CONTROL_TTY:
	default:
	    *newOutputSizePtr = 0;
	    goto noconversion;

	case IOC_TTY_GET_DISCIPLINE:
	case IOC_NUM_READABLE:
	case IOC_TTY_GET_LM:
	case IOC_GET_FLAGS:
	    /*
	     * One int
	     */
	    fmtString = "w";
	    break;

	case IOC_TTY_GETP:
	    /*
	     * struct sgttyb
	     */
	    fmtString = "{b4h}";
	    break;

	case IOC_GET_OWNER:
	    /*
	     * Ioc_Owner
	     */
	    fmtString = "{w2}";
	    break;

	case IOC_TTY_GET_TCHARS:
	    /*
	     * struct tchars
	     */
	    fmtString = "{b6}";
	    break;

	case IOC_TTY_GET_LTCHARS:
	    /*
	     * struct ltchars
	     */
	    fmtString = "{b6}";
	    break;

	case IOC_TTY_GET_WINDOW_SIZE:
	    /*
	     * struct winsize
	     */
	    fmtString = "{h4}";
	    break;
    }
    status = Fmt_Convert(fmtString, FMT_MY_FORMAT, &outputSize, output, format,
	    newOutputSizePtr, newOutput);
noconversion:
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Td_GetRaw --
 *
 *	Retrieve characters that are ready to be output from the
 *	terminal driver to the raw device.
 *
 * Results:
 *	The return value is a count of the number of characters
 *	actually returned at buffer.  This will be less than or
 *	equal to numChars.  A return value of 0 indicates that
 *	there are no characters in terminal's output buffer or
 *	that output has been disabled.
 *
 * Side effects:
 *	Characters are removed from the terminal's output buffer,
 *	and the cooked side may be notified that the terminal is
 *	writable again.
 *
 *----------------------------------------------------------------------
 */

int
Td_GetRaw(terminal, numChars, buffer)
    Td_Terminal terminal;	/* Token identifying terminal. */
    int numChars;		/* Maximum number of characters to read
				 * from terminal's output buffer. */
    register char *buffer;	/* Where to place characters that are read. */
{
    register Terminal *tPtr = (Terminal *) terminal;
    int count;

    if (tPtr->flags & OUTPUT_OFF) {
	return 0;
    }
    for (count = 0; count < numChars; count++, buffer++) {
	if (tPtr->lastRemovedOut == tPtr->lastAddedOut) {
	    break;
	}
	NEXT(tPtr->lastRemovedOut, tPtr->outBufSize, tPtr->lastRemovedOut);
	*buffer = tPtr->outputBuffer[tPtr->lastRemovedOut];
    }
    tPtr->outCharsBuffered -= count;
    if (tPtr->outCharsBuffered < tPtr->cookedOutputLimit) {
	(*tPtr->cookedProc)(tPtr->cookedData, TD_COOKED_WRITES_OK,
		0, (char *) NULL, 0, (char *) NULL);
    }
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * Td_PutRaw --
 *
 *	This procedure is invoked when characters arrive from the
 *	raw device associated with the terminal (e.g., from the
 *	keyboard).  It adds them to the input buffer of the terminal
 *	and does appropriate line editing etc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are added to the input buffer, and may be made
 *	available on the cooked side of the terminal.  Echoed
 *	characters get added to the output buffer, which could result
 *	in a call to the raw control procedure.
 *
 *----------------------------------------------------------------------
 */

void
Td_PutRaw(terminal, numChars, buffer)
    Td_Terminal terminal;	/* Token identifying terminal. */
    int numChars;		/* Number of characters to process. */
    char *buffer;		/* Characters that were ostensibly typed
				 * on the raw device's keyboard. */
{
    register Terminal *tPtr = (Terminal *) terminal;
    int next, oldCharsBuffered;
    register char c = '\0';	/* dummy initial value */
    Td_Signal signalInfo;

    oldCharsBuffered = tPtr->outCharsBuffered;
    tPtr->localMode &= ~LFLUSHO;

    /*
     * According to the 4.3 BSD manual page, we should re-echo everything
     * in the input buffer if LPENDIN is set here.  But this appears to
     * produce the wrong results and I suspect that it isn't even
     * implemented in BSD.  So it's not implemented here either.
     */

    for ( ; numChars > 0; numChars--, buffer++) {
	c = *buffer;

	/*
	 * Skip all further processing if in raw mode.
	 */
	
	if (tPtr->sgttyb.sg_flags & RAW) {
	    goto addToBuffer;
	}
	c &= 0x7f;

	/*
	 * Handle flow-control characters.
	 */

	
	if (c == tPtr->tchars.t_stopc) {
	    if (tPtr->flags & OUTPUT_OFF) {
		if (c == tPtr->tchars.t_startc) {
		    goto restartOutput;
		}
	    } else {
		tPtr->flags |= OUTPUT_OFF;
	    }
	    continue;
	} else if (c == tPtr->tchars.t_startc) {
	    restartOutput:
	    tPtr->flags &= ~OUTPUT_OFF;
	    if (tPtr->outCharsBuffered != 0) {
		(*tPtr->rawProc)(tPtr->rawData, TD_RAW_OUTPUT_READY,
			0, (char *) NULL, 0, (char *) NULL);
	    }
	    continue;
	} else if ((tPtr->flags & OUTPUT_OFF) && !(tPtr->localMode & LDECCTQ)) {
	    tPtr->flags &= ~OUTPUT_OFF;
	    if (tPtr->outCharsBuffered != 0) {
		(*tPtr->rawProc)(tPtr->rawData, TD_RAW_OUTPUT_READY,
			0, (char *) NULL, 0, (char *) NULL);
	    }
	}

	/*
	 * If the last character typed was a "quote" character, then
	 * just add the new character to the input buffer without
	 * additional processing.
	 */

	if (tPtr->flags & LITERAL_NEXT) {
	    tPtr->flags &= ~LITERAL_NEXT;
	    goto addToBuffer;
	}

	/*
	 * Handle output-flushing character.  Clearing oldCharsBuffered
	 * is essential, otherwise, the raw client won't be notified if
	 * characters are added to the output buffer in this procedure.
	 */

	if (c == tPtr->ltchars.t_flushc) {
	    if (tPtr->localMode & LFLUSHO) {
		tPtr->localMode &= ~LFLUSHO;
	    } else {
		TdFlushOutput(tPtr);
		oldCharsBuffered = 0;
		TdEcho(tPtr, c);
		TdRetypeInput(tPtr, tPtr->lastRemovedIn);
		tPtr->localMode |= LFLUSHO;
	    }
	    continue;
	}
    
	/*
	 * Handle line-editing characters such as erase and kill.
	 */
	
	if ((c == '\r') && (tPtr->sgttyb.sg_flags & CRMOD)) {
	    c = '\n';
	}
	if (!(tPtr->sgttyb.sg_flags & CBREAK)) {
	    if (c == tPtr->sgttyb.sg_erase) {		/* Backspace. */
		if (tPtr->lastAddedIn != tPtr->lastBreak) {
		    TdBackspace(tPtr);
		}
		continue;
	    } else if (c == tPtr->ltchars.t_werasc) {	/* Delete word. */
		int gotNonSpace = 0;
    
		while (tPtr->lastAddedIn != tPtr->lastBreak) {
		    if (isspace(tPtr->inputBuffer[tPtr->lastAddedIn])) {
			if (gotNonSpace) {
			    break;
			}
		    } else {
			gotNonSpace = 1;
		    }
		    TdBackspace(tPtr);
		}
		continue;
	    } else if (c == tPtr->sgttyb.sg_kill) {	/* Delete line. */
		if ((tPtr->lastHidden != -1) || !(tPtr->localMode & LCRTKIL)) {
		    TdEcho(tPtr, c);
		    TdEcho(tPtr, '\n');
		    tPtr->lastAddedIn = tPtr->lastBreak;
		    tPtr->lastHidden = -1;
		} else {
		    while (tPtr->lastAddedIn != tPtr->lastBreak) {
			TdBackspace(tPtr);
		    }
		}
		continue;
	    } else if (c == tPtr->ltchars.t_rprntc) {	/* Re-echo all. */
		TdEcho(tPtr, c);
		TdEcho(tPtr, '\n');
		TdRetypeInput(tPtr, tPtr->lastRemovedIn);
		continue;
	    }
	}

	if (c == tPtr->ltchars.t_lnextc) {
	    tPtr->flags |= LITERAL_NEXT;
	    continue;
	}
    
	/*
	 * Generate signals in response to certain input characters.  If this
	 * isn't a signal character, then officially add it to the input
	 * buffer.
	 */
    
	if (c == tPtr->tchars.t_intrc) {
	    signalInfo.sigNum = SIGINT;
	    sendSignal:
	    signalInfo.groupID = tPtr->owner;
	    (*tPtr->cookedProc)(tPtr->cookedData, TD_COOKED_SIGNAL,
		    sizeof(signalInfo), (char *) &signalInfo,
		    0, (char *) NULL);
	    TdFlushInput(tPtr);
	    TdFlushOutput(tPtr);
	    oldCharsBuffered = 0;
	    goto echo;
	} else if (c == tPtr->tchars.t_quitc) {
	    signalInfo.sigNum = SIGQUIT;
	    goto sendSignal;
	} else if (c == tPtr->ltchars.t_suspc) {
	    signalInfo.sigNum = SIGTSTP;
	    goto sendSignal;
	}
    
	/*
	 * If the buffer is full, then reallocate it with a size twice as
	 * large.  Then add the character to the buffer.
	 */
    
	addToBuffer:
	NEXT(tPtr->lastAddedIn, tPtr->inBufSize, next);
	if (next == tPtr->lastRemovedIn) {
	    char *newBuffer;
	    int src, dst;
    
	    newBuffer = malloc((unsigned) 2*tPtr->inBufSize);
	    for (src = tPtr->lastRemovedIn, dst = 0; src != tPtr->lastAddedIn; ) {
		NEXT(src, tPtr->inBufSize, src);
		dst += 1;
		newBuffer[dst] = tPtr->inputBuffer[src];
	    }
	    tPtr->lastBreak -= tPtr->lastRemovedIn;
	    if (tPtr->lastBreak < 0) {
		tPtr->lastBreak += tPtr->inBufSize;
	    }
	    if (tPtr->lastHidden != -1) {
		tPtr->lastHidden -= tPtr->lastRemovedIn;
		if (tPtr->lastHidden < 0) {
		    tPtr->lastHidden += tPtr->inBufSize;
		}
	    }
	    tPtr->inputBuffer = newBuffer;
	    tPtr->inBufSize *= 2;
	    tPtr->lastRemovedIn = 0;
	    tPtr->lastAddedIn = dst;
	    NEXT(dst, tPtr->inBufSize, next);
	}
	tPtr->inputBuffer[next] = c;
	tPtr->lastAddedIn = next;
    
	/*
	 * Echo.
	 */
    
	echo:
	if ((tPtr->sgttyb.sg_flags & ECHO) && !(tPtr->sgttyb.sg_flags & RAW)) {
	    if (tPtr->flags & BS_IN_PROGRESS) {
		TdPutChar(tPtr, '/');
		tPtr->flags &= ~BS_IN_PROGRESS;
	    }
	    TdEcho(tPtr, c);
	}
    }

    /*
     * Are there any characters that are ready for reading?  If so,
     * change the terminal's state to be readable and notify the
     * cooked side.
     */

    if ((tPtr->sgttyb.sg_flags & (RAW|CBREAK)) || (c == tPtr->tchars.t_eofc) ||
	    (c == tPtr->tchars.t_brkc) || (c == '\n')) {
	tPtr->lastBreak = tPtr->lastAddedIn;
	tPtr->lastHidden = -1;
	tPtr->keyIndex = tPtr->lastAddedIn;
	tPtr->keyColumn = tPtr->column;
	(*tPtr->cookedProc)(tPtr->cookedData, TD_COOKED_READS_OK,
		0, (char *) NULL, 0, (char *) NULL);
    }

    /*
     * If the output buffer just became non-empty, then notify the
     * raw control procedure.
     */

    if ((oldCharsBuffered == 0) && (tPtr->outCharsBuffered != 0) &&
	    !(tPtr->flags & OUTPUT_OFF)) {
	(*tPtr->rawProc)(tPtr->rawData, TD_RAW_OUTPUT_READY,
		0, (char *) NULL, 0, (char *) NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Td_ControlRaw --
 *
 *	This procedure is used to tell the terminal driver that
 *	certain special things happened on the raw side of the
 *	terminal, such as a hangup or break.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the operation;  see the man page for details
 *	on what commands may be invoked.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
void
Td_ControlRaw(terminal, operation)
    Td_Terminal terminal;		/* Token for terminal. */
    int operation;			/* What just happened: TD_BREAK etc. */
{
    register Terminal *tPtr = (Terminal *) terminal;

    switch (operation) {
	case TD_BREAK: {

	    /*
	     * Reset some of the terminal state, such as flow control,
	     * then pretend an interrupt character was typed.
	     */

	    tPtr->flags &= ~(OUTPUT_OFF | BS_IN_PROGRESS | LITERAL_NEXT);
	    if (tPtr->sgttyb.sg_flags & RAW) {
		char c = 0;
		Td_PutRaw(terminal, 1, &c);
	    } else if ((int) tPtr->tchars.t_intrc != -1) {
		Td_PutRaw(terminal, 1, &tPtr->tchars.t_intrc);
	    }
	    if (tPtr->outCharsBuffered != 0) {
		(*tPtr->rawProc)(tPtr->rawData, TD_RAW_OUTPUT_READY,
			0, (char *) NULL, 0, (char *) NULL);
	    }
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TdPutChar --
 *
 *	Add a character to the output buffer associated with a
 *	terminal, and keep track of the current column while outputting
 *	the character.  This routine also substitutes spaces for tabs,
 *	if that's the mode the terminal is in.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TPtr->column gets updated and stuff gets added to the terminal's
 *	output buffer.  The output buffer will get grown if necessary.
 *
 *----------------------------------------------------------------------
 */

static void
TdPutChar(tPtr, c)
    register Terminal *tPtr;	/* Terminal on which to output. */
    char c;			/* Character to output. */
{
    /*
     * Ignore the character if output is being flushed.
     */

    if (tPtr->localMode & LFLUSHO) {
	return;
    }

    /*
     * Grow the output buffer if there isn't enough space for the
     * largest amount of information this procedure might want to
     * add to it.
     */

    if ((tPtr->outCharsBuffered + 8) >= tPtr->outBufSize) {
	char *newBuffer;
	int dst;

	newBuffer = malloc((unsigned) (2*tPtr->outBufSize));
	for (dst = 0; tPtr->lastRemovedOut != tPtr->lastAddedOut; ) {
	    dst += 1;
	    NEXT(tPtr->lastRemovedOut, tPtr->outBufSize, tPtr->lastRemovedOut);
	    newBuffer[dst] = tPtr->outputBuffer[tPtr->lastRemovedOut];
	}
	tPtr->outputBuffer = newBuffer;
	tPtr->outBufSize *= 2;
	tPtr->lastRemovedOut = 0;
	tPtr->lastAddedOut = dst;
    }

    /*
     * Update column position, and add character(s) to the buffer.
     */

    if (isprint(c)) {
	tPtr->column += 1;
    } else if (c == '\r') {
	tPtr->column = 0;
    } else if (c == '\t') {
	int count = 8 - (tPtr->column & 07);

	tPtr->column += count;
	if ((tPtr->sgttyb.sg_flags & TBDELAY) == XTABS) {
	    for ( ; count > 0; count--) {
		NEXT(tPtr->lastAddedOut, tPtr->outBufSize, tPtr->lastAddedOut);
		tPtr->outputBuffer[tPtr->lastAddedOut] = ' ';
		tPtr->outCharsBuffered++;
	    }
	    return;
	}
    } else if (c == '\b') {
	tPtr->column -= 1;
    }
    NEXT(tPtr->lastAddedOut, tPtr->outBufSize, tPtr->lastAddedOut);
    tPtr->outputBuffer[tPtr->lastAddedOut] = c;
    tPtr->outCharsBuffered++;
}

/*
 *----------------------------------------------------------------------
 *
 * TdEcho --
 *
 *	Echo a character on a terminal, if echoing is enabled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The appropriate echo sequence for c gets added to the 
 *	terminal's output buffer.
 *
 *----------------------------------------------------------------------
 */

static void
TdEcho(tPtr, c)
    register Terminal *tPtr;	/* Terminal for which to echo. */
    register char c;		/* Character to echo. */
{
    if (!(tPtr->sgttyb.sg_flags & ECHO)) {
	return;
    }
    if (isprint(c)) {
	TdPutChar(tPtr, c);
    } else if (c == '\n') {
	if (tPtr->sgttyb.sg_flags & CRMOD) {
	    TdPutChar(tPtr, '\r');
	}
	TdPutChar(tPtr, '\n');
    } else if (c == '\t') {
	TdPutChar(tPtr, c);
    } else if (c == 04) {
	/* Don't echo control-D's. */
    } else if (tPtr->localMode & LCTLECH) {
	TdPutChar(tPtr, '^');
	if (c == 0177) {
	    TdPutChar(tPtr, '?');
	} else {
	    TdPutChar(tPtr, c + 'A' - 1);
	}
    } else {
	TdPutChar(tPtr, c);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TdRetypeInput --
 *
 *	This procedure is called to re-echo all of the characters in
 *	the input buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters get added to the terminal's output buffer.
 *
 *----------------------------------------------------------------------
 */

static void
TdRetypeInput(tPtr, start)
    register Terminal *tPtr;	/* Which terminal to re-echo for. */
    int start;			/* Index within tPtr's buffer:  start
				 * echoing at the character AFTER this one. */
{
    tPtr->keyIndex = start;
    tPtr->keyColumn = tPtr->column;
    while (start != tPtr->lastAddedIn) {
	NEXT(start, tPtr->inBufSize, start);
	TdEcho(tPtr, tPtr->inputBuffer[start]);
    }
    tPtr->lastHidden = -1;
}

/*
 *----------------------------------------------------------------------
 *
 * TdBackspace --
 *
 *	Using mode information from tPtr, output the appropriate
 *	sequence to backspace over the most recently typed character
 *	in tPtr's input buffer.  Also remove the character from
 *	the input buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TPtr's input buffer ends up with less characters in it,
 *	and stuff gets added to the output buffer.
 *
 *----------------------------------------------------------------------
 */

static void
TdBackspace(tPtr)
    register Terminal *tPtr;		/* Terminal to backspace. */
{
    if (tPtr->sgttyb.sg_flags & ECHO) {
	if (tPtr->localMode & LCRTBS) {
	    int count;
	    char c;

	    /*
	     * CRT-style backspacing:  the character can actually be erased.
	     * Figure out how wide the character was, then back over it one
	     * space at a time.  If there's output intervening between us and
	     * the next character to erase, then first re-echo everything.
	     */

	    if (tPtr->lastAddedIn == tPtr->lastHidden) {
		c = tPtr->ltchars.t_rprntc;
		if ((c & 0377) == 0377) {
		    c = ltcharsDefault.t_rprntc;
		}
		TdEcho(tPtr, c);
		TdEcho(tPtr, '\n');
		TdRetypeInput(tPtr, tPtr->lastRemovedIn);
		tPtr->lastHidden = -1;
	    }
	    c = tPtr->inputBuffer[tPtr->lastAddedIn];
	    if (isprint(c)) {
		count = 1;
	    } else {
		int i, pos;
		char c2;

		/*
		 * Anything besides a normal printing character is tricky.  Tabs
		 * are particularly nasty.  To figure out how much to erase,
		 * work forwards from a known position, computing the position
		 * of the character just before the one being erased.
		 */

		i = tPtr->keyIndex;
		pos = tPtr->keyColumn;
		while (TRUE) {
		    NEXT(i, tPtr->inBufSize, i);
		    if (i == tPtr->lastAddedIn) {
			break;
		    }
		    c2 = tPtr->inputBuffer[i];
		    if (isprint(c2)) {
			pos++;
		    } else if (c2 == '\t') {
			pos = (pos + 8) & ~07;
		    } else if (((c2 == '\n') && !(tPtr->sgttyb.sg_flags & CRMOD))
			    || (c2 == 04)) {
			/* No change to position. */
		    } else if (tPtr->localMode & LCTLECH) {
			pos += 2;
		    } else if (c2 == '\b') {
			pos -= 1;
		    }
		}
		count = tPtr->column - pos;
	    }

	    for ( ; count > 0; count--) {
		if (tPtr->localMode & LCRTERA) {
		    TdPutChar(tPtr, '\b');
		    TdPutChar(tPtr, ' ');
		}
		TdPutChar(tPtr, '\b');
	    }
	} else if (tPtr->localMode & LPRTERA) {

	    /*
	     * Hardcopy terminal:  backspace by outputting erased characters
	     * between "\" and "/" delimiters.
	     */

	    if (!(tPtr->flags & BS_IN_PROGRESS)) {
		TdPutChar(tPtr, '\\');
		tPtr->flags |= BS_IN_PROGRESS;
	    }
	    TdEcho(tPtr, tPtr->inputBuffer[tPtr->lastAddedIn]);
	} else {

	    /*
	     * Old-style backspace:  just echo the backspace character.
	     */

	    TdEcho(tPtr, tPtr->sgttyb.sg_erase);
	}
    }
    PREV(tPtr->lastAddedIn, tPtr->inBufSize, tPtr->lastAddedIn);
}

/*
 *----------------------------------------------------------------------
 *
 * TdFlushInput --
 *
 *	Empty the input buffer associated with a terminal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TPtr's input buffer is cleared.
 *
 *----------------------------------------------------------------------
 */

static void
TdFlushInput(tPtr)
    register Terminal *tPtr;		/* Terminal to flush. */
{
    tPtr->lastAddedIn = tPtr->lastRemovedIn = 0;
    tPtr->lastBreak = tPtr->keyIndex = 0;
    tPtr->lastHidden = -1;
    tPtr->keyColumn = tPtr->column;
    tPtr->flags &= ~(LITERAL_NEXT|BS_IN_PROGRESS);
}

/*
 *----------------------------------------------------------------------
 *
 * TdFlushOutput --
 *
 *	Empty the output queue for a terminal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The output buffer for tPtr is emptied, and the terminal's raw
 *	control procedure is invoked to empty any other buffers down
 *	the line.  The cooked side gets notified that the terminal
 *	is now writable.
 *
 *----------------------------------------------------------------------
 */

static void
TdFlushOutput(tPtr)
    register Terminal *tPtr;		/* Terminal to flush. */
{
    tPtr->lastAddedOut = tPtr->lastRemovedOut = 0;
    tPtr->outCharsBuffered = 0;
    (*tPtr->rawProc)(tPtr->rawData, TD_RAW_FLUSH_OUTPUT, 0,
	    (char *) NULL, 0, (char *) NULL);
    (*tPtr->cookedProc)(tPtr->cookedData, TD_COOKED_WRITES_OK, 0,
	    (char *) NULL, 0, (char *) NULL);
}
