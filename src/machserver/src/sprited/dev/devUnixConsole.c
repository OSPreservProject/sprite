/* 
 * devUnixConsole.c --
 *
 *	Provide console support using the UNIX tty that the Sprite server 
 *	was started up under.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/dev/RCS/devUnixConsole.c,v 1.2 92/04/02 21:19:39 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <errno.h>
#include <stdlib.h>
#include <sprite.h>
#include <sgtty.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include <devInt.h>
#include <main.h>
#include <proc.h>
#include <sync.h>
#include <tty.h>

/* These should go in a Mach/UX header file somewhere... */

extern int open _ARGS_((_CONST char *name, int flags, ...));
extern void	perror _ARGS_((_CONST char *msg));

#define BUFSIZE	512		/* size of output buffer for system printf's */

#define min(a,b) ((a) < (b) ? (a) : (b))

static Boolean initialized = FALSE;

/* 
 * Unix information about the console.
 */

#define STDOUT_FD	1	/* hack: file desc. for UNIX stdout */

static struct sgttyb consoleInfo;

static int consoleFd;		/* UNIX file descriptor for the console */

/* 
 * Sprite information about the console.
 */

static DevTty consoleTty;

/* 
 * This monitor lock is used to protect against concurrent attempts to 
 * write to the console.  I'm not sure this is really necessary, but it 
 * shouldn't hurt.
 */
static Sync_Lock consoleLock = Sync_LockInitStatic("consoleLock");
#define LOCKPTR &consoleLock

#ifdef DEBUG
/* 
 * Keep a buffer of the last few chunks written to the console.  This 
 * doesn't include system printf's.
 */
#define NUM_ROWS	10
typedef struct {
    char chars[100];		/* copy of what was written */
    Proc_PID pid;		/* the process that did the writing */
} DebugInfo;
static DebugInfo debugBuf[NUM_ROWS];
static int debugBufIndex;	/* index into the buffer */
#endif /* DEBUG */

/* Forward references */

static void DevConsoleActivateProc _ARGS_((void *clientData));
static int DevConsoleRawProc _ARGS_((void *clientData,
		int operation, int inputSize, Address inBuffer,
		int outputSize, Address outBuffer));
static void DevConsoleReadLoop _ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * DevTtyAttach --
 *
 *	Initialize a tty as part of the open sequence.
 *	
 *	For now the only tty that we support is the console.
 *	Note: this routine is in with the console-specific code because we 
 *	want to do the initialization early in the startup sequence, before 
 *	there are multiple threads running (which might cause a race on 
 *	errno).
 *
 * Results:
 *	Returns a partially filled-in structure describing the tty.  
 *	Returns NULL if the unit number is bad.
 *
 * Side effects:
 *	If the console wasn't already initialized, UNIX /dev/tty is opened 
 *	and put into a convenient state.  Sets initialized to TRUE.
 *
 *----------------------------------------------------------------------
 */
    
DevTty *
DevTtyAttach(unit)
    int unit;			/* unit number for the device */
{
    DevTty *ttyPtr;

    if (unit != DEV_CONSOLE_UNIT) {
	return NULL;
    }

    ttyPtr = &consoleTty;
    if (initialized) {
	return ttyPtr;
    }

    if (main_MultiThreaded) {
	printf("Warning: DevTtyAttach initializing the console ");
	printf("after the system is multithreaded.\n");
    }

    /*
     * Do the UNIX initialization.
     */

    consoleFd = open("/dev/tty", O_RDWR, 0666);
    if (consoleFd < 0) {
	perror("DevTtyAttach: can't open /dev/tty");
	exit(1);
    }
    if (ioctl(consoleFd, TIOCGETP, &consoleInfo) < 0) {
	perror("DevTtyAttach: can't get tty info");
	exit(1);
    }
    consoleInfo.sg_flags |= RAW;
    consoleInfo.sg_flags &= ~ECHO;
    if (ioctl(consoleFd, TIOCSETP, &consoleInfo) < 0) {
	perror("DevTtyAttach: can't set raw mode");
	exit(1);
    }

    /* 
     * Now do the Sprite initialization.  Don't start up the thread to
     * monitor the console yet, because this routine might get called very
     * early in the startup sequence.
     */
    
    ttyPtr->rawProc = DevConsoleRawProc;
    ttyPtr->rawData = (ClientData)NIL;
    ttyPtr->activateProc = DevConsoleActivateProc;
    ttyPtr->inputProc = (void (*)()) NIL;
    ttyPtr->inputData = (ClientData) 0;
    ttyPtr->consoleFlags = DEV_TTY_IS_CONSOLE;

    initialized = TRUE;
    return ttyPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * DevConsoleActivateProc --
 *
 *	Finish preparing the console for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forks a thread to read from the console.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
DevConsoleActivateProc(clientData)
    _VoidPtr clientData;
{
    static Boolean alreadyCalled = FALSE;

    if (alreadyCalled) {
	printf("DevConsoleActivateProc: warning: multiple calls.\n");
				/* DEBUG */
	return;
    }

    (void)Proc_NewProc((Address) UTILSMACH_MAGIC_CAST DevConsoleReadLoop,
		       (Address)0, PROC_KERNEL, FALSE, (Proc_PID *)NULL,
		       "console input process");
    alreadyCalled = TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * DevConsoleRawProc --
 *
 *	Raw output and control procedure for the console.
 * 
 * 	Note: there appears to be a bug in the Mach telnetd.  If a bare 
 * 	newline is sent and there is enough delay so that the newline isn't 
 * 	batched with a previous or following write(), the newline seems to 
 * 	get swallowed.  Lines can get broken up this way (with the carriage 
 * 	return in one chunk and the newline in the next) because of the way 
 * 	the higher-level tty code does buffering.  If this gets to be a 
 * 	problem, we can try putting in a hack, e.g., to detect lines that 
 * 	end in carriage return only and hold the carriage return until the 
 * 	newline is printed (or just insert a newline after the carriage 
 * 	return and figure that the next one will get eaten).
 * 	-mdk 24-Mar-92.
 *
 * Results:
 *	Returns the number of bytes places in the given output buffer.
 *
 * Side effects:
 *	Depends on the control operation.  Most likely effect is to
 *	write bytes to the console.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static ENTRY int
DevConsoleRawProc(clientData, operation, inputSize, inBuffer, outputSize,
		  outBuffer)
    _VoidPtr clientData;	/* unused */
    int operation;		/* what we're supposed to do */
    int inputSize;		/* bytes in inBuffer */
    Address inBuffer;		/* input buffer */
    int outputSize;		/* bytes in outBuffer */
    Address outBuffer;		/* output (result) buffer */
{
    char buf[TTY_OUT_BUF_SIZE]; /* temporary buffer */
    char *bufPtr;		/* pointer into it */
    int ch = -1;		/* one character */

    if (operation != TD_RAW_OUTPUT_READY) {
	return 0;
    }

    LOCK_MONITOR;

    /* 
     * Fill up the temporary buffer with whatever's available, then write 
     * it all at once.  We may end up doing this multiple times.
     */
    do {
	for (bufPtr = buf; bufPtr < buf + sizeof(buf); ++bufPtr) {
	    ch = DevTtyOutputChar(&consoleTty);
	    if (ch == -1) {
		break;
	    }
	    *bufPtr = ch & 0x7f; /* XXX why mask off the high-order bit? */
	}
	if (bufPtr > buf) {
	    (void)write(consoleFd, buf, bufPtr-buf);
#ifdef DEBUG
	    bcopy(buf, debugBuf[debugBufIndex].chars, bufPtr-buf);
	    debugBuf[debugBufIndex].chars[bufPtr-buf] = '\0';
	    debugBuf[debugBufIndex].pid = Proc_GetCurrentProc()->processID;
	    debugBuf[debugBufIndex].retVal = retVal;
	    if (++debugBufIndex == NUM_ROWS) {
		debugBufIndex = 0;
	    }
#endif
	}
    } while (ch != -1);

    UNLOCK_MONITOR;
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * DevConsoleReadLoop --
 *
 *	Wait in a loop for input from the "console".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Puts characters on the console input queue as they arrive.
 *
 *----------------------------------------------------------------------
 */

static void
DevConsoleReadLoop()
{
    char buf[10];
    int charsRead;
    char *bufPtr;

    /* 
     * Fortunately, there's no need to turn off raw mode when we're 
     * shutting down.  This means that the read loop can just do a read() 
     * and block until a character comes in.  Otherwise, we'd have to 
     * either (a) verify that an stty() in parallel with a read() works 
     * okay, or (b) use two threads, one that actually looks for data, and 
     * a second one that waits for either a signal from the first one or a 
     * signal to shutdown.
     */

    for (;;) {
	charsRead = read(consoleFd, buf, sizeof(buf));
	if (charsRead < 0) {
	    panic("Can't read from console: %s\n",
		  strerror(errno));
	}
	for (bufPtr = buf; bufPtr < buf + charsRead; ++bufPtr) {
	    DevTtyInputChar(&consoleTty, *bufPtr);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_ConsoleWrite --
 *
 *	Write a sprited (internal) stdio buffer to the console.
 *
 * Results:
 *	Returns the number of bytes written, not including carriage returns 
 *	that are inserted for newlines.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
Dev_ConsoleWrite(numBytes, buffer)
    int numBytes;		/* number of bytes to write */
    Address buffer;		/* the bytes to write */
{
    int fd;			/* actual file descriptor to write to */
    char expBuf[BUFSIZE];	/* copy of buffer, with LF expanded to LF/CR */
    int bufIndex;		/* index into given buffer */
    char *fromPtr, *toPtr;	/* pointers for actual copying */
    char *stopPtr;		/* last+1 for fromPtr in current chunk */

    LOCK_MONITOR;

    fd = (initialized ? consoleFd : STDOUT_FD);
    
    /* 
     * Copy the given buffer into the expansion buffer.  To simplify 
     * bookkeeping, only copy BUFSIZE/2 characters at a time, so that the
     * result will always fit.  Repeat until the entire given buffer has
     * been sent.
     */
    for (bufIndex = 0; bufIndex < numBytes; bufIndex += BUFSIZE/2) {
	stopPtr = buffer + min(numBytes, bufIndex + BUFSIZE/2);
	for (fromPtr = buffer + bufIndex, toPtr = expBuf;
	         fromPtr < stopPtr;
	         ++fromPtr, ++toPtr) {
	    *toPtr = *fromPtr;
	    if (*toPtr == '\n') {
		++toPtr;
		*toPtr = '\r';
	    }
	}
	write(fd, expBuf, toPtr - expBuf);
    }

    UNLOCK_MONITOR;
    return numBytes;
}
