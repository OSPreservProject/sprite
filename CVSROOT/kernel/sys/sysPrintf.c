/*
 *  sysPrintf --
 *
 *      Perform all formatted printing to the console.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "stdio.h"
#include "varargs.h"
#include "sync.h"
#include "mach.h"
#include "fs.h"
#include "sys.h"
#include "dbg.h"
#include "dev.h"

/*
 * Calls to panic and printf are protected.
 */
static	Sync_Semaphore	sysPrintMutex = Sync_SemInitStatic("sysPrintMutex");

/*
 * Set during a panic to prevent recursion.
 */
Boolean	sysPanicing = FALSE;

/*
 * Used to keep track of bytes written.
 */
static int bytesWritten;

/*
 * vprintf buffer.
 */
#define	STREAM_BUFFER_SIZE	512
static char streamBuffer[STREAM_BUFFER_SIZE];

/*
 * ----------------------------------------------------------------------------
 *
 * writeProc --
 *
 *      Stream writeProc - flushes data to syslog. 
 *
 * Results:
 *	None
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
writeProc(stream, flush)
    FILE *stream;
    Boolean flush;
{
    Fs_IOParam	io;
    Fs_IOReply	reply;

    bzero((char *)&io, sizeof(io));
    io.buffer = (Address) stream->buffer;
    io.length = stream->lastAccess + 1 - stream->buffer;
    bzero((char *)&reply, sizeof(reply));

    if (io.length > 0) { 
	(void)Dev_SyslogWrite((Fs_Device *) NIL, &io, &reply);
	stream->lastAccess = stream->buffer - 1;
	stream->writeCount = stream->bufSize;
	bytesWritten += reply.length;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * vprintf --
 *
 *	Printing routine that is called from varargs procedures.  The
 *	caller should use to varargs macros to extract the format
 *	string and the va_list structure.  This also checks for
 *	recursion that can result from a panic and initializes
 *	the stream data structure needed by the standard vfprintf.
 *
 * Results:
 *      Number of characters printed.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

#ifdef lint
/* VARARGS1 */
/* ARGSUSED */
int vprintf(format)
    char *format;
{
    /*
     * Lint complains about unused variables...  This is all #ifdef'ed lint.
     * It's silly and can probably be cut down a bit....
     */
    char foo;
    Sync_Semaphore *barPtr;
    barPtr = &sysPrintMutex;
    sysPrintMutex = *barPtr;
    writeProc((FILE *) NULL, 0);
    streamBuffer[0] = '\0';
    foo = streamBuffer[0];
    streamBuffer[0] = foo;
}
#else
/*VARARGS1*/
int
vprintf(format, args)
    char	*format;
    va_list	args;
{
    static Boolean	initialized = FALSE;
    static FILE		stream;
    static int	recursiveCallP = 0;	/* prevent recursive calls
					 * that could occur if vprintf
					 * fails, etc.  */

    if (recursiveCallP != 0) {
	return 0;
    }
    recursiveCallP = 1;
    MASTER_LOCK(&sysPrintMutex);
    if (!initialized) {
	Stdio_Setup(&stream, 0, 1, streamBuffer, STREAM_BUFFER_SIZE,
		(void (*)()) 0, writeProc,  (int (*)()) 0, (ClientData) 0);
	initialized = TRUE;
    }

    bytesWritten = 0;
    vfprintf(&stream, format, args);
    fflush(&stream);
    MASTER_UNLOCK(&sysPrintMutex);
    recursiveCallP = 0;
    return (bytesWritten);

}
#endif

/* 
 *----------------------------------------------------------------------
 *
 * panic --
 *
 *	Print an error message and enter the debugger. This entry is 
 *	provided for libc.a routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The kernel dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

#ifdef lint
/* VARARGS1 */
/* ARGSUSED */
void panic(format)
    char *format;
{}
#else
void
panic(va_alist)
    va_dcl			/* char *format, then any number of additional
				 * values to be printed under the control of
				 * format.  This is all just the same as you'd
				 * pass to printf. */
{
    char *format;
    va_list args;

    va_start(args);
    format = va_arg(args, char *);

    Dev_VidEnable(TRUE);	/* unblank the screen */
    Dev_SyslogDebug(TRUE);	/* divert /dev/syslog output to the screen */
    if (!sysPanicing) {
	printf("Fatal Error: ");
	(void) vprintf(format, args);
	va_end(args);
    }
    sysPanicing = TRUE;
    DBG_CALL;
    Dev_SyslogDebug(FALSE);
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * printf --
 *
 *      Perform a C style printf with disabling of interrupts.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */


#ifdef lint
/* VARARGS1 */
/* ARGSUSED */
void printf(format)
    char *format;
{}
#else
void
printf(va_alist)
    va_dcl
{
    char *format;
    va_list	args;

    va_start(args);
    format = va_arg(args, char *);

    (void) vprintf(format, args);
    va_end(args);
}
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * fprintf --
 *
 *      Perform a C style fprintf with disabling of interrupts (output
 *	always goes to the console:  stream arg is ignored).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

#ifdef lint
/* VARARGS */
/* ARGSUSED */
int fprintf()
{}
#else
int
fprintf(va_alist)
    va_dcl
{
    char *format;
    va_list	args;
    int result;

    va_start(args);
    (void) va_arg(args, FILE *);
    format = va_arg(args, char *);

    result = vprintf(format, args);
    va_end(args);
    return result;
}
#endif
