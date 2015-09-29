/* 
 * vmFsCache.c --
 *
 *	Routines that deal with the FS block cache.
 *	
 *	In native Sprite the VM and FS modules trade pages back and forth, 
 *	depending on who has the oldest bits.  VM pages are given priority, 
 *	so there's not just one single cache.  We might do something 
 *	similar in the Sprite server, but for the time being just give the 
 *	file system a fixed size buffer which it can wire down.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmFsCache.c,v 1.2 92/04/16 11:22:00 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <limits.h>
#include <mach.h>
#include <mach_error.h>
#include <mach/mach_host.h>
#include <user/fs.h>

#include <sys.h>


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetRefTime --
 *
 *	Return the age of the LRU page (0 if is a free page).
 *
 * Results:
 *	Returns 0, so that the FS cache can grow to its allotted size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Vm_GetRefTime()
{
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_MapBlock --
 *
 *	Wire down enough pages at the given address to map one FS cache
 *	block.
 *
 * Results:
 *	The number of pages that were wired down.
 *	
 *	Note: the FS code assumes that the VM page size is assumed to be at 
 *	least as large as the FS block size.  If it's bigger, the FS code
 *	makes requests in terms of VM pages and then sticks multiple FS
 *	blocks into a single page.  This means that Vm_MapBlock will never
 *	return a number larger than 1.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Vm_MapBlock(addr)
    Address	addr;		/* Address where to map in pages. */
{
    kern_return_t kernStatus;

    kernStatus = vm_wire(sys_PrivHostPort, mach_task_self(),
			 (vm_address_t)addr, vm_page_size,
			 VM_PROT_READ | VM_PROT_WRITE);

    /* 
     * If the call failed, complain, but pretend that it succeeded.  Wiring 
     * the pages is only needed for performance, not correctness.
     */
    if (kernStatus != KERN_SUCCESS) {
	printf("Vm_MapBlock: can't wire page at 0x%x: %s\n",
	       addr, mach_error_string(kernStatus));
    }

    return 1;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_UnmapBlock --
 *
 *	Unwire enough pages at the given address to unmap one fs cache
 *	block.
 *
 * Results:
 *	The number of pages that were unwired.  See Vm_MapBlock.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Vm_UnmapBlock(addr)
    Address	addr;		/* Address where to start unwiring. */
{
    kern_return_t kernStatus;

    kernStatus = vm_wire(sys_PrivHostPort, mach_task_self(),
			 (vm_address_t)addr, vm_page_size,
			 VM_PROT_NONE);

    /* 
     * If the call failed, complain, but pretend that it succeeded.
     */
    if (kernStatus != KERN_SUCCESS) {
	printf("Vm_UnmapBlock: can't unwire page at 0x%x: %s\n",
	       addr, mach_error_string(kernStatus));
    }

    return 1;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_GetPageSize --
 *
 *      Return the page size.
 *
 * Results:
 *      The page size.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Vm_GetPageSize()
{
    return(vm_page_size);
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_FsCacheSize --
 *
 *	Return the virtual addresses of the start and end of the file systems
 *	cache.
 *
 * Results:
 *	Fills in the starting and last (not last+1) addresses of the file 
 *	system cache.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Vm_FsCacheSize(startAddrPtr, endAddrPtr)
    Address	*startAddrPtr;	/* OUT: Lowest virtual address. */
    Address	*endAddrPtr;	/* OUT: Highest virtual address. */
{
    kern_return_t kernStatus;
    vm_size_t cacheSize;	/* number of bytes in FS cache */

    /* 
     * The FS cache is allocated out of the default pager store.  Ideally 
     * we should probably pick a cache size based on the size of the 
     * pager's backing store.  For the time being, though, just go for a 
     * moderate number, say 4 MB, rounded to an integral number of pages.
     */

    cacheSize = (4 * 1024 * 1024 / vm_page_size) * vm_page_size;

    *startAddrPtr = 0;
    kernStatus = vm_allocate(mach_task_self(), (vm_address_t *)startAddrPtr,
			     cacheSize, TRUE);
    if (kernStatus != KERN_SUCCESS) {
	panic("Vm_FsCacheSize: couldn't allocate %d KB for FS cache: %s\n",
	      cacheSize/1024, mach_error_string(kernStatus));
    }

    *endAddrPtr = *startAddrPtr + cacheSize - 1;
}
