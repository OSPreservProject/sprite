/* 
 * machMon.c --
 *
 *     Routines to access the SPUR console monitor. 
 * 
 * TODO: The console driver is supposed to use on the UARTs on the CPU board.
 *
 * The console monitor output is stored in a circular buffer. 
 * This buffer is primary for viewing devSyslog messages from the kernel
 * debugger. 
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
#include "machMon.h"
#include "dbg.h"



/*
 * The circular buffer definitions. 
 */

#define	BUFFER_SIZE	4*1024

char	machMonBuffer[BUFFER_SIZE];		/* The buffer itself. */
char	*machMonBufferPtr = machMonBuffer;  	/* First free character
						 * of buffer. */	
#ifndef lint
static	char *monBufStart = machMonBuffer;
#endif

/*
 * Forward declaration for stdio buffer write procedure used by _doprnt called
 * by Mach_MonPrintf.
 */

static void MonPrintfWriteProc();

/*
 * TRUE => send characters to the uart.
 */
static Boolean monUseUart;


#include "varargs.h"
#include "stdio.h"

/*
 *----------------------------------------------------------------------
 *
 * Mach_MonInit --
 *
 *	Initialized the monitor. All this does is check if we are using
 *	serial line debugging, and if we are don't send stuff to the
 *	uart. The uart has two channels, but the current uart implementation
 *	doesn't use channel B.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the monUseUart flag.
 *
 *----------------------------------------------------------------------
 */

void
Mach_MonInit()
{
    int switches;

    switches = read_physical_word(0x40000);
    if ((switches & 0x80) == 0) {
	monUseUart = TRUE;
    } else {
	monUseUart = FALSE;
    }
    if (monUseUart) {
	uart_init(1200);
    } else {
	uart_init(9600);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonPrintf --
 *
 *     Do a printf to the monitor.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonPrintf(va_alist)
    va_dcl			/* Variable number of values to be formatted. */
{

    FILE 	stream;
    char	*format;	/* Printf format argument. */
    va_list 	args;
    char	smallBuffer;	

    /*
     * MonPrintfWriteProc depends on the buffer only being one byte.
     */
    Stdio_Setup(&stream, 0, 1, &smallBuffer, sizeof(smallBuffer), 
		(void (*)()) 0, MonPrintfWriteProc, (int (*)()) 0, 
		(ClientData) 0);
    va_start(args);
    format = va_arg(args,char*);
    vfprintf(&stream, format, args);

}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonPutChar --
 *
 *     Output a character to the monitor.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonPutChar(ch)
    int		ch;
{
    if (machMonBufferPtr >= &(machMonBuffer[BUFFER_SIZE])) {
	    /*
	     * Wrap the buffer around.
	     */
	     machMonBufferPtr = machMonBuffer;
    }
    *machMonBufferPtr = ch;
    if (monUseUart) {
	writeUart(ch);
    }
    machMonBufferPtr++;

}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonMayPut --
 *
 *     	Output a character to the monitor.  This will return
 *	-1 if it couldn't put out the character.
 *
 * Results:
 *     -1 if couldn't emit the character.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

int
Mach_MonMayPut(ch)
    int		ch;
{

    if (machMonBufferPtr >= &(machMonBuffer[BUFFER_SIZE])) {
	/*
	 * No room for character, wrap the buffer.
	 */
	machMonBufferPtr = machMonBuffer;
    }
    *machMonBufferPtr = ch;
    machMonBufferPtr++;
    return (0);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonAbort --
 *
 *     	Abort to the monitor.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonAbort()
{
	DBG_CALL;
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonReboot --
 *
 *     	Reboot the system.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     System rebooted.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Mach_MonReboot(rebootString)
    char	*rebootString;
{
	DBG_CALL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonGetLine --
 *
 *     	Wait for someone to write a console for SPUR.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonGetLine()
{
	DBG_CALL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonGetNextChar --
 *
 *     	Wait for someone to write a console for SPUR.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonGetNextChar()
{
	DBG_CALL;
}




/*
 * ----------------------------------------------------------------------------
 *
 * MonPrintfWriteProc --
 *
 *     	Stdio WriteProc used for processing the printf command.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Character is "written" using the Mach_MonPutChar call.
 *
 * ----------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
MonPrintfWriteProc(stream, flush)
    FILE	*stream;	/* Stream handle for printf. */
    int		flush;		/* not used. */
{
    /*
     * This code assumes that the bufferSize is only one byte.
     */
    Mach_MonPutChar(*(char *) stream->buffer);
    stream->lastAccess = stream->buffer - 1;
    stream->writeCount = 1;
}
