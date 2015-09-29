/* 
 * procDebugRegs.c --
 *
 *	Convert registers between the format expected by the ptrace system
 *	call and that of the Sprite Proc_Debug call.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.2 89/01/07 04:12:18 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "status.h"
#include "sys/ptrace.h"
#include <errno.h>
#include <proc.h>
#include <signal.h>
#include <sys/wait.h>
#include "procDebugRegs.h"


/*
 *----------------------------------------------------------------------
 *
 * procDebugToPtraceRegs --
 *
 *	Convert registers from a proc debug format to a ptrace format.
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
procDebugToPtraceRegs(regStatePtr, ptraceRegsPtr)
    Mach_RegState	*regStatePtr; /* Register state as returned by 
				       * the PROC_GET_DBG_STATE argument
				       * to Proc_Debug. */
    char	*ptraceRegsPtr;	      /* Memory to put ptrace format of
				       * registers. */

{
	bcopy((char *)regStatePtr, ptraceRegsPtr, sizeof(Mach_RegState));
}


/*
 *----------------------------------------------------------------------
 *
 * ptraceToProcDebugRegs --
 *
 *	Convert registers from a ptrace format to a proc debug format.
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
ptraceToProcDebugRegs(ptraceRegsPtr, regStatePtr)
    char	*ptraceRegsPtr;	      /* Memory image of ptrace format of
				       * registers. */
    Mach_RegState	*regStatePtr; /* Register state as used by Proc_Debug */

{
	bcopy(ptraceRegsPtr, (char *)regStatePtr, sizeof(Mach_RegState));
}

