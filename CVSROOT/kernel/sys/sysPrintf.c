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
#include "mach.h"
#include "fs.h"
#include "sys.h"
#include "dbg.h"
#include "dev.h"

/*
 * Calls to panic and printf are protected.
 */
static	int	sysPrintMutex = 0;

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
    int		count;
    int		written;
    Boolean	status;


    count = stream->lastAccess + 1 - stream->buffer;

    if (count > 0) { 
	status = Dev_SyslogWrite((Fs_Device *) NIL, 0, count, 
				    (char *) stream->buffer, &written);
#ifdef NOTDEF
	/*
	 * What to do here?  This would cause deadlock and probably won't
	 * succeed anyway!
	 */
	if (status != SUCCESS) {
	    printf("sys printf: Dev_SyslogWrite failed\n");
	}
#endif /* NOTDEF */
	stream->lastAccess = stream->buffer - 1;
	stream->writeCount = stream->bufSize;
	bytesWritten += written;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * doVprintf --
 *
 *      Perform a C style vprintf to the monitor. 
 *
 * Results:
 *      Number of characters printed.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

static int
doVprintf(format, args)
    char	*format;
    va_list	*args;
{
    static Boolean	initialized = FALSE;
    static FILE		stream;

    if (!initialized) {
	Stdio_Setup(&stream, 0, 1, streamBuffer, STREAM_BUFFER_SIZE,
		(void (*)()) 0, writeProc,  (int (*)()) 0, (ClientData) 0);
	initialized = TRUE;
    }

    bytesWritten = 0;
    vfprintf(&stream, format, *args);
    fflush(&stream);
    return (bytesWritten);

}


#ifdef NOTDEF
/* No one calls this one anyway. */

/*
 * ----------------------------------------------------------------------------
 *
 * Sys_DoPrintf --
 *
 *      Perform a C style printf to the monitor. 
 *
 * Results:
 *      Number of characters printed.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */
/*VARARGS*/
int
Sys_DoPrintf(va_alist)
    va_dcl
{
	char *format;
	int  count;
	va_list	args;

	va_start(args);
	format = va_arg(args, char *);
	MASTER_LOCK(sysPrintMutex);
	count = doVprintf(format, &args);
	MASTER_UNLOCK(sysPrintMutex);
	va_end(args);

	return (count);
}
#endif /* NOTDEF */

Boolean	sysPanicing = FALSE;


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

/* VARARGS0 */
void
panic(va_alist)
    va_dcl			/* char *format, then any number of additional
				 * values to be printed under the control of
				 * format.  This is all just the same as you'd
				 * pass to printf. */
{
    char *format;
    va_list args;
    static int	recursiveCallP = 0;	/* prevent recursive calls to panic
					 * that could occur if doVprintf
					 * fails, etc.  */

    va_start(args);
    format = va_arg(args, char *);

    if (recursiveCallP != 0) {
	return;
    }
    recursiveCallP = 1;
	
    Dev_SyslogDebug(TRUE);
    printf("Fatal Error: ");
    MASTER_LOCK(sysPrintMutex);
    (void) doVprintf(format,&args);
    sysPanicing = TRUE;
    MASTER_UNLOCK(sysPrintMutex);
    recursiveCallP = 0;
    DBG_CALL;
    Dev_SyslogDebug(FALSE);
}



/*
 * ----------------------------------------------------------------------------
 *
 * Sys_Panic --
 *
 *	This should go away as soon as everyone converts their WARNING-level
 *	calls to printf and their FATAL-level calls to panic.
 *
 *      Print a formatted string to the monitor and then either abort to the
 *      debugger or continue depending on the panic level.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

/*VARARGS0*/
void
Sys_Panic(va_alist)
    va_dcl
{
    Sys_PanicLevel 	level;
    char 		*format;
    va_list 		args;
    static int	recursiveCallP = 0;	/* prevent recursive calls to panic
					 * that could occur if doVprintf
					 * fails, etc.  */

    va_start(args);

    level = va_arg(args, Sys_PanicLevel);
    format = va_arg(args, char *);

    if (recursiveCallP != 0) {
	return;
    }
    recursiveCallP = 1;
    if (level == SYS_WARNING) {
        printf("Warning: ");
    } else {
	Dev_SyslogDebug(TRUE);
        printf("Fatal Error: ");
    }

    MASTER_LOCK(sysPrintMutex);
    (void) doVprintf(format,&args);

    if (level != SYS_FATAL) {
	MASTER_UNLOCK(sysPrintMutex);
	recursiveCallP = 0;
	return;
    }

    sysPanicing = TRUE;
    MASTER_UNLOCK(sysPrintMutex);
    recursiveCallP = 0;
    DBG_CALL;
    Dev_SyslogDebug(FALSE);
    return;
}

#ifdef NOTDEF
/* No one calls this one, anyway. */

/*
 * ----------------------------------------------------------------------------
 *
 * Sys_UnSafePrintf --
 *
 *      Perform a C style printf without disabling interrupts.
 *	This routine does not get the sysPrintMutex and is thus truly
 *	unsafe.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * ----------------------------------------------------------------------------
 */

/*VARARGS*/
void
Sys_UnSafePrintf(va_alist)
    va_dcl
{
    va_list	args;
    char *format;

    va_start(args);
    format = va_arg(args, char *);
    (void) doVprintf(format,  &args);
    va_end(args);
}
#endif NOTDEF

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

/*VARARGS0*/
void
printf(va_alist)
    va_dcl
{
    char *format;
    va_list	args;
    static int	recursiveCallP = 0;	/* prevent recursive calls to panic
					 * that could occur if doVprintf
					 * fails, etc.  */

    va_start(args);
    format = va_arg(args, char *);

    if (recursiveCallP != 0) {
	return;
    }
    recursiveCallP = 1;
    MASTER_LOCK(sysPrintMutex);
    (void) doVprintf(format, &args);
    MASTER_UNLOCK(sysPrintMutex);
    recursiveCallP = 0;
    va_end(args);
}

/*
 * The following will go away after everyone has converted their calls
 * to Sys_Printf to calls to printf.
 */
/*VARARGS0*/
void
Sys_Printf(va_alist)
    va_dcl
{
    char *format;
    va_list	args;
    static int	recursiveCallP = 0;	/* prevent recursive calls to panic
					 * that could occur if doVprintf
					 * fails, etc.  */

    va_start(args);
    format = va_arg(args, char *);

    if (recursiveCallP != 0) {
	return;
    }
    recursiveCallP = 1;
    MASTER_LOCK(sysPrintMutex);
    (void) doVprintf(format, &args);
    MASTER_UNLOCK(sysPrintMutex);
    recursiveCallP = 0;
    va_end(args);
}
