/*
 * procDebugRegs.h --
 *
 *	Declarations of register conversion routines for ptrace and
 *	Proc_Debug system calls.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.2 89/01/07 04:12:44 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCDEBUGREGS
#define _PROCDEBUGREGS

extern void ptraceToProcDebugRegs();
extern void procDebugToPtraceRegs();

/* procedures */

#endif /* _PROCDEBUGREGS */

