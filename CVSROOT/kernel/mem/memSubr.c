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
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "vm.h"



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
    Sys_Panic(SYS_FATAL, message);
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
 * Mem_PrintTrace --
 *
 *	Print out the given trace information about a memory trace record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Mem_Trace(allocated, blockAddr, pc, numBytes)
    Boolean	allocated;
    Address	blockAddr;
    Address	pc;
    int		numBytes;
{
    Sys_Printf("%s: PC=%x size=%d blockAddr=%d\n",
		allocated ? "Alloc" : "Free", pc, numBytes, blockAddr);
}

void	PrintProc();

static	int	smallMinNum = 50;
static	int	largeMinNum = 10;
static	int	largeMaxSize = 10000;


/*
 *----------------------------------------------------------------------
 *
 * Mem_DumpStats --
 *
 *	Print out memory statistics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Mem_DumpStats()
{
    Mem_PrintStats(PrintProc, 0, smallMinNum, largeMinNum, largeMaxSize);
}

void
PrintProc(clientData, format, args)
    ClientData	clientData;
    char	*format;
    int		args;
{
    Sys_DoPrintf(format, (char *) &args);
}
