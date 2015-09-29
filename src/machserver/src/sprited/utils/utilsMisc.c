/* 
 * utilsMisc.c --
 *
 *	Various utility routines.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/utils/RCS/utilsMisc.c,v 1.12 92/04/29 21:46:11 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>

#include <main.h>
#include <user/sys.h>
#include <utils.h>
#include <vmTypes.h>

/* 
 * The Mach errno.h doesn't provide errno, sigh.
 */
extern int errno;

/* 
 * This should be in a Mach header file somewhere... 
 */
extern int syscall _ARGS_((int syscallNumber, ...));

#define PID_TO_TASK_SYSCALL	(-33) /* UX syscall number for pid_to_task */


/*
 *----------------------------------------------------------------------
 *
 * Utils_MapSpriteProtect --
 *
 *	Map a Sprite protection code to a Mach protection code.
 *
 * Results:
 *	Returns the Mach equivalent of the given Sprite protection code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

vm_prot_t
Utils_MapSpriteProtect(spriteAccess)
    int spriteAccess;
{
    vm_prot_t result = 0;

    switch (spriteAccess) {
    case VM_READONLY_ACCESS:
	result = VM_PROT_READ;
	break;
    case VM_OVERWRITE_ACCESS:
	result = VM_PROT_WRITE;
	break;
    case VM_READWRITE_ACCESS:
	result = VM_PROT_READ | VM_PROT_WRITE;
	break;
    default:
	panic("Utils_MapSpriteProtect: bogus protection code.\n");
	break;
    }

    result |= VM_PROT_EXECUTE;
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * Utils_UnixPidToTask --
 *
 *	Convert a Unix process ID to a Mach task.  The UX server provides 
 *	this back door for obtaining privileged system ports.
 *	
 *	Notes:
 *	1. This routine goes away when the Sprite server is the startup
 *	server.
 *	2. This routine should be called only during initialization, before 
 *	there are multiple threads running, so that we don't have to worry 
 *	about races accessing errno.
 *
 * Results:
 *	Returns a Unix errno code, or 0 if there wasn't an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Utils_UnixPidToTask(pid, taskPtr)
    pid_t pid;			/* Unix process ID to look up */
    task_t *taskPtr;		/* OUT: the corresponding task */
{
    if (main_MultiThreaded) {
	printf("Warning: %s called after system is multithreaded.\n",
	       "Utils_UnixPidToTask");
    }
    errno = 0;
    *taskPtr = (task_t)syscall(PID_TO_TASK_SYSCALL, pid);    
    return errno;
}
