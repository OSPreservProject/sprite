/*
 * dbgAsm.h --
 *
 *     Termination type constants.  These are in a separate file
 *     because this file is included by the assembler file and assemblers 
 *     can't handle C types.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBGASM
#define _DBGASM

/*
 * Reasons for termination.  These are Unix signal numbers that dbx expects.  
 * Therefore don't change these numbers or kdbx will break.  The reasons are:
 *
 *	DBG_NOREASON_SIG	Unknown reason
 *	DBG_INTERRUPT_SIG	Interrupt
 *	DBG_TRACETRAP_SIG	Trace trap
 */

#define	DBG_NO_REASON_SIG	0
#define	DBG_INTERRUPT_SIG	2
#define	DBG_TRACE_TRAP_SIG	5

/*
 * The size of the hole to leave in the debuggers stack when it is called.
 * This hole is so that kdbx can play with the stack without ruining the 
 * the debuggers stack.  The size is in bytes. The size must be a mutiple
 * of 4 to allow the code in dbgTrap.s to conform to the gcc C calling
 * sequence.
 */

#define	DBG_STACK_HOLE	52

#endif _DBGASM
