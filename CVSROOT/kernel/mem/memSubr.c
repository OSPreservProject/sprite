/*
 * memSubr.c --
 *
 *	This file contains user/kernel-dependent routines used by the
 *	dynamic memory allocation system. It provides procedures
 *	to allocate storage, and a panic routine to halt execution.
 *
 *	Every routine in this file assumes that the monitor lock is held.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <vm.h>
#include <stdlib.h>
#include <varargs.h>
#include <mem.h>
#include <memInt.h>

/*
 * memPrintProc is the routine called by the routines in memory.c
 * when they have something to print. It is set to PrintProc in
 * MemProcInit().
 */

static void 	PrintProc _ARGS_(());

void (*memPrintProc) _ARGS_(());

ClientData	memPrintData = (ClientData) 0;

/*
 * Flag to determine whether to panic when freeing free blocks. This
 * value is user/kernel dependent and is therefore placed in this file.
 */

Boolean memAllowFreeingFree = FALSE;



/*
 *----------------------------------------------------------------------
 *
 * MemPanic --
 *
 *	MemPanic is a procedure that's called by the memory allocator
 *	when it has uncovered a fatal error.  MemPanic prints the
 *	message and aborts.  It does NOT return.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The program is exited.
 *
 *----------------------------------------------------------------------
 */

void
MemPanic(message)
    char *message;
{
    panic(message);
}


/*
 *----------------------------------------------------------------------
 *
 * MemChunkAlloc --
 *
 *	Mem_Alloc will call MemChunkAlloc to get another region of storage
 *	from the system (i.e. whenever the storage it's gotten so far
 *	is insufficient to meet a request).  The actual size returned
 *	may be greater than size but not less.  This region now becomes
 *	the permanent property of Mem_Alloc, and will never be returned.
 *
 * Results:
 *	The actual size of the block allocated in bytes.
 *
 * Side effects:
 *	Memory is passed to the allocator and lost forever.
 *
 *----------------------------------------------------------------------
 */

int
MemChunkAlloc(size, addressPtr)
    int size;			/* Number of bytes desired.  */
    Address *addressPtr;	/* Address of the new region */
{
    *addressPtr = Vm_RawAlloc(size);
    return(size);
}

/*
 *----------------------------------------------------------------------
 *
 * Mem_DumpStats --
 *
 *	Call back routine used to print memory stats with
 *	a magic 'L1-m' keystroke on the console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed on the console.
 *
 *----------------------------------------------------------------------
 */
static	int	smallMinNum = 1;
static	int	largeMinNum = 1;
static	int	largeMaxSize = 10000;

void
Mem_DumpStats()
{
    Mem_PrintStatsSubrInt(PrintProc, (ClientData) 0, smallMinNum, largeMinNum,
	    largeMaxSize);
    Mem_DumpTrace(-1);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintProc --
 *
 *	The default printing routine for the kernel.
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
static void
PrintProc(va_alist)
    va_dcl		/* ClientData, then char *format, then any number
			 * of additional values to be printed. */
{
    ClientData clientData;
    char *format;
    va_list args;

    va_start(args);
    clientData = va_arg(args, ClientData);
#ifdef lint
    clientData = clientData;
#endif /* lint */
    format = va_arg(args, char *);
    (void)vprintf(format, args);
    va_end(args);
}


/*
 *----------------------------------------------------------------------
 *
 * MemPrintInit --
 *
 *	Initializes the default printing routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The default printing routine is initialized.
 *
 *----------------------------------------------------------------------
 */

void
MemPrintInit()
{
    memPrintProc = PrintProc;
    memPrintData = (ClientData) 0;
}
