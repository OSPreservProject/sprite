/*
 * ptrace.c --
 *
 *	Routines for creating a Unix like debugger interface to Sprite.
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
 * $Header: /sprite/src/kernel/mach/spur.md/RCS/machConfig.c,v 1.2 88/11/11 15:3
6:43 mendel Exp $ SPRITE (Berkeley)
 */

#include "machine/sys/ptrace.h"
#include <errno.h>
#include <signal.h>


/* 
 *----------------------------------------------------------------------
 *
 * ptrace --
 *
 *	Emulate Unix ptrace system call.
 *
 * Results:
 *	An integer.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

ptrace (request,pid,addr,data, addr2)
     enum ptracereq request;	/* Data
     int pid;			/* Process id of debugee. */
     char *addr;		
     int data;
     char *addr2;
{
     extern int	errno;
     errno = EINVAL;
    return -1;
}
