/* 
 * brk.c --
 *
 *	Source code for the "brk" and "sbrk" library procedures, which
 *	emulate the UNIX kernel calls by the same names.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/brk.c,v 1.7 89/04/12 17:11:23 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <stdio.h>
#include <vm.h>
#include <sys/types.h>

/*
 * NextAddr is the user's idea of the location of the first
 * as-yet-unallocated byte in the heap.  RealNextAddr is the
 * real location.  We keep two pointers so we if a program
 * asks brk() to reduce the size of the heap, we can shrink
 * is without making any actual system calls.  The space is
 * still allocated, so subsequent requests to extend the heap
 * can also be met without system calls.
 */

extern int end;				/* Linker gives this variable an
					 * address equal to the location just
					 * above the top of the heap. */
#ifndef LINTLIB
static caddr_t nextAddr = (caddr_t) &end;
static caddr_t realNextAddr = (caddr_t) &end;
#else
static caddr_t nextAddr = 0;
static caddr_t realNextAddr = 0;
#endif /* LINTLIB */
static int pageSize;

/*
 * Macro to compute for any address, the first addresss of the page
 * it is on.
 */

#define FirstAddrOnPage(address)  (((int) (address)) & (~pageSize))


/*
 *----------------------------------------------------------------------
 *
 * brk --
 *
 *	Enlarge the heap, if necessary, so that it extends to at
 *	least addr-1.
 *
 * Results:
 *	0 is normally returned.  If the heap couldn't be extended
 *	to the given place, then -1 is returned.
 *
 * Side effects:
 *	The virtual address space of the process is extended.
 *
 *----------------------------------------------------------------------
 */

caddr_t
brk(addr)
    caddr_t addr;		/* Make this the new "first location just
				 * above top of heap".  */
{
    static int initialized = 0;
    ReturnStatus status;

    if (!initialized) {
        initialized = 1;

	/*
	 *  Get the system page size and calculate the address in the heap 
	 *  after the end of data to start allocating chunks from.
	 */

	if (Vm_PageSize(&pageSize) != SUCCESS) {
	    panic("brk couldn't get page size");
	    return 0;		/* should never get here */
	}
	pageSize -= 1;
    }

    /*
     * See if the new top-of-heap is already mapped;  if so, there's
     * nothing for us to do.
     */

    if (addr <= realNextAddr) {
	nextAddr = addr;
	return 0;
    }
    if(FirstAddrOnPage(realNextAddr - 1) != FirstAddrOnPage(addr)) {
	status = Vm_CreateVA(realNextAddr, addr-realNextAddr);
	if (status != SUCCESS) {
	    return (caddr_t) -1;
	}
    }
    realNextAddr = nextAddr = addr;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * sbrk --
 *
 *	Make "numBytes" more space available in the heap, and return
 *	a pointer to it.
 *
 * Results:
 *	The return value is a pointer to a new block of memory, which
 *	is numBytes bytes long.  If no space could be allocated, then
 *	-1 is returned.
 *
 * Side effects:
 *	The virtual address space of the process is extended.
 *
 *----------------------------------------------------------------------
 */

caddr_t
sbrk(numBytes)
    int numBytes;			/* Number of bytes desired.  */
{
    caddr_t result;

    result = nextAddr;
    if (brk(nextAddr+numBytes) == (caddr_t) -1) {
	return (caddr_t) -1;
    }
    return result;
}
