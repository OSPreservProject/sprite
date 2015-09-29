/* 
 * mmap.c --
 *
 *	Procedure to map from Unix mmap system call to Sprite.
 *
 * Copyright 1989 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/mmap.c,v 1.4 91/08/12 17:10:47 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include <sys/types.h>

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * mmap --
 *
 *	Procedure to map from Unix mmap system call to Sprite
 *	Vm_Mmap.
 *
 * Results:
 *	Returns address of the mapped segment.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	Creates a mapped segment.
 *
 *----------------------------------------------------------------------
 */

Address
mmap(addr, len, prot, share, fd, pos)
    caddr_t	addr;	/* Starting virtual address of mapped segment. */
    int		len;	/* Length of segment to map. */
    int 	prot;	/* Protections for segment. */
    int		share;	/* Private/shared flag. */
    int		fd;	/* Descriptor of open file to map. */
    off_t	pos;	/* Offset into mapped file. */
{
    Address newAddr;		/* Address returned by Vm_Mmap. */
    ReturnStatus status;	/* result returned by Vm_Mmap. */

    status = Vm_Mmap((Address)addr, len, prot, share, fd, pos, &newAddr);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return((Address) -1);
    } else {
	return (newAddr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * munmap --
 *
 *	Procedure to map from Unix munmap system call to Sprite
 *	Vm_Munmap.
 *
 * Results:
 *	Returns 0 on success.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	Removes the mapping of the page.
 *
 *----------------------------------------------------------------------
 */

int
munmap(addr, len)
    caddr_t	addr;	/* Starting virtual address of mapped segment. */
    int		len;	/* Length of segment to unmap. */
{
    ReturnStatus status;	/* result returned by Vm_Mmap. */

    status = Vm_Munmap((Address)addr, len, 0);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * msync --
 *
 *	Procedure to map from Unix msync system call to Sprite
 *	Vm_Msync.
 *
 * Results:
 *	Returns 0 if success.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	Syncs pages to disk.
 *
 *----------------------------------------------------------------------
 */

int
msync(addr, len)
    caddr_t	addr;	/* Starting virtual address of region. */
    int		len;	/* Length of segment to sync. */
{
    ReturnStatus status;	/* result returned by Vm_Msync. */

    status = Vm_Msync((Address)addr, len);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (0);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * mlock --
 *
 *	Procedure to map from Unix mmap system call to Sprite
 *	Vm_Mlock.
 *
 * Results:
 *	Returns 0 if success.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	locks pages.
 *
 *----------------------------------------------------------------------
 */

int
mlock(addr, len)
    caddr_t	addr;	/* Starting virtual address of region. */
    int		len;	/* Length of region to lock. */
{
    ReturnStatus status;	/* result returned by Vm_Mlock. */

    status = Vm_Mlock((Address)addr, len);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (0);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * munlock --
 *
 *	Procedure to map from Unix mmap system call to Sprite
 *	Vm_Munlock.
 *
 * Results:
 *	Returns 0 if success.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	Unlocks pages.
 *
 *----------------------------------------------------------------------
 */

int
munlock(addr, len)
    caddr_t	addr;	/* Starting virtual address of region. */
    int		len;	/* Length of region to unlock. */
{
    ReturnStatus status;	/* result returned by Vm_Munlock. */

    status = Vm_Munlock((Address)addr, len);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (0);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * mincore --
 *
 *	Procedure to map from Unix mincore system call to Sprite
 *	Vm_Mincore.
 *
 * Results:
 *	Returns vector of values; one for each page.
 *		0 if page not virtually present.
 *		1 if page paged out.
 *		2 if page clean, unreferenced.
 *		3 if page clean, referenced.
 *		4 if page dirty.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
mincore(addr, len, vec)
    caddr_t	addr;	/* Starting virtual address. */
    int		len;	/* Length of segment to map. */
    char 	*vec;	/* Return value vector. */
{
    ReturnStatus status;	/* result returned by Vm_Mincore. */

    status = Vm_Mincore((Address)addr, len, vec);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * mprotect --
 *
 *	Procedure to map from Unix mprotect system call to Sprite
 *	Vm_Mprotect.
 *
 * Results:
 *	Returns 0 if success.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	Changes protection.
 *
 *----------------------------------------------------------------------
 */

Address
mprotect(addr, len, prot)
    caddr_t	addr;	/* Starting virtual address. */
    int		len;	/* Length of region. */
    int 	prot;	/* Protections for segment. */
{
    ReturnStatus status;	/* result returned by Vm_Mprotect. */

    status = Vm_Mprotect((Address)addr, len, prot);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (0);
    }
}
