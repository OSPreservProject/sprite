/*
 * tty.h --
 *
 *	This file contains declarations of the facilities provided by
 *	devTty.c for the rest of the dev module.  This consists of glue
 *	that holds together a generic terminal driver (ttyDriver), a
 *	device-specific interface to a serial line, and the generic
 *	Sprite kernel-call procedures for terminal devices.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVTTY
#define _DEVTTY

#ifndef _TD
#include <td.h>
#endif
#ifndef _SYNC
#include "sync.h"
#endif

/*
 * For each terminal or terminal-like device supported by the kernel,
 * there exists a record with the following structure.  The structure
 * (and the code in devTty.c) is complicated by the fact that the Td
 * library calls malloc, which means that some of its procedures can't
 * be invoked by interrupt handlers (or with interrupts disabled).  To
 * get around this problem, devTty.c introduces an extra level of buffering
 * between the interrupt handler and Td.  The Td structures are locked
 * using a sleep lock, and are ONLY accessed from process level.  The extra
 * buffers are handled without locks by using pointers carefully.  Transfers
 * between the Td buffers and the DevTty buffers are made only by procedures
 * running at process level.
 */

#define TTY_IN_BUF_SIZE 100
#define TTY_OUT_BUF_SIZE 10

typedef struct DevTty {
    /*
     * Intermediate buffers for terminal.  Note that the input buffer
     * contains int's, not chars, in order to allow for the special
     * non-ASCII values defined below, such as DEV_TTY_BREAK.  Both buffers
     * use circular pointers.
     */

    volatile int inBuffer[TTY_IN_BUF_SIZE];
    volatile int insertInput;		/* Index at which to place next value
					 * in inBuffer. */
    volatile int extractInput;		/* Index at which to remove next value
					 * from inBuffer.  If insert ==
					 * extract, buffer is empty. */
    volatile unsigned char outBuffer[TTY_OUT_BUF_SIZE];
    volatile int insertOutput;		/* Index at which to place next byte
					 * in outBuffer. */
    volatile int extractOutput;		/* Index at which to remove next byte
					 * from outputBuffer.  If insert ==
					 * extract, buffer is empty. */

    /*
     * Information used to communicate with the device-specific driver and
     * other device-specific parameterization.  This information is filled
     * in by the machine-specific initialization procedure DevTtyAttach.
     */

    int (*rawProc)();			/* Used as "rawProc" for the
					 * terminal by Td library. */
    void (*activateProc)();		/* Called to activate terminal
					 * (initialize, enable interrupts,
					 * etc.) after initialization is
					 * complete. */
    ClientData rawData;			/* Arbitrary value associated with
					 * the device driver;  passed to
					 * rawProc by Td library. */
    void (*inputProc)();		/* For most terminal-like devices
					 * this is NULL.  If non-NULL, it
					 * is a procedure to invoke to process
					 * each input character (e.g. to map
					 * keystroke identifiers to ASCII
					 * characters). */
    ClientData inputData;		/* Arbitrary value to pass to
					 * inputProc. */
    int consoleFlags;			/* Used to manage console device;
					 * see below for definitions. */

    /*
     * Information about the terminal and how to hook its cooked side up
     * to Sprite kernel calls.
     */

    Td_Terminal term;			/* Token returned by Td_Create. */
    int selectState;			/* Current select state for terminal:
					 * OR'ed combination of FS_READABLE,
					 * FS_WRITABLE, and FS_EXCEPTION. */
    ClientData notifyToken;		/* Token identifying device;  used to
					 * notify Fs when terminal becomes
					 * readable or writable. */
    int openCount;			/* Number of active opens of
					 * terminal. */
} DevTty;

/*
 * Definitions for consoleFlags bits:
 *
 * DEV_TTY_IS_CONSOLE:	1 means this terminal is the console, so handle
 *			special console commands like L1-A.
 * DEV_TTY_GOT_BREAK:	For consoles that are just serial lines, this bit
 *			means a break was just received on input, so the
 *			next character is a console command.
 */

#define DEV_TTY_IS_CONSOLE	1
#define DEV_TTY_GOT_BREAK	2

/*
 * Special values for "characters" placed in the buffer of a DevTty:
 *
 * DEV_TTY_BREAK:	A break condition just occurred on the device.
 * DEV_TTY_HANGUP:	The terminal just got hung up.
 */

#define DEV_TTY_BREAK	1000
#define DEV_TTY_HANGUP	1001

#define DEV_TTY_IS_CONTROL(x) ((x) & ~0xff)

/*
 * Variables exported by devTty.c:
 */

extern Sync_Lock	devTtyLock;

/*
 * Procedures exported by devTty.c:
 */

extern ReturnStatus	DevTtyClose();
extern void		DevTtyInputChar();
extern int		DevTtyOutputChar();
extern ReturnStatus	DevTtyIOControl();
extern ReturnStatus	DevTtyOpen();
extern ReturnStatus	DevTtyRead();
extern ReturnStatus	DevTtySelect();
extern ReturnStatus	DevTtyWrite();

#endif /* _DEVTTY */
